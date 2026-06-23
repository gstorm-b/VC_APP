#include <RobotKinematics/Solvers/NumericalIKSolver.h>

#include <RobotKinematics/Kinematics/ForwardKinematics.h>
#include <RobotKinematics/Kinematics/JointLimitValidator.h>
#include <RobotKinematics/Model/RobotModelValidator.h>
#include <RobotKinematics/Solvers/IKSolutionRanker.h>

#include <Eigen/Cholesky>
#include <Eigen/Geometry>

#include <algorithm>
#include <cmath>
#include <limits>

namespace RobotKinematics {

namespace {
struct Residual {
    Eigen::Matrix<double, 6, 1> vector = Eigen::Matrix<double, 6, 1>::Zero();
    double positionNorm = 0.0;
    double orientationNorm = 0.0;
};

bool finitePose(const Pose& pose)
{
    return pose.isometry().matrix().allFinite();
}

double clamp(double value, double lower, double upper)
{
    return std::max(lower, std::min(upper, value));
}

Eigen::VectorXd clampToLimits(const SerialRobotConfig& config, const Eigen::VectorXd& q)
{
    Eigen::VectorXd clamped = q;
    int index = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type != JointType::Revolute && joint.type != JointType::Prismatic) {
            continue;
        }
        if (joint.limits.has_value()) {
            clamped[index] = clamp(clamped[index], joint.limits->lower, joint.limits->upper);
        }
        ++index;
    }
    return clamped;
}

JointVector homeJoint(const SerialRobotConfig& config)
{
    Eigen::VectorXd q(ForwardKinematics::movableJointCount(config));
    int index = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type == JointType::Revolute || joint.type == JointType::Prismatic) {
            q[index++] = joint.home;
        }
    }
    return JointVector(q);
}

JointVector midpointJoint(const SerialRobotConfig& config)
{
    Eigen::VectorXd q(ForwardKinematics::movableJointCount(config));
    int index = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type != JointType::Revolute && joint.type != JointType::Prismatic) {
            continue;
        }
        q[index++] = joint.limits.has_value() ? 0.5 * (joint.limits->lower + joint.limits->upper) : joint.home;
    }
    return JointVector(q);
}

bool sameJointVector(const JointVector& a, const JointVector& b, double tolerance)
{
    return a.size() == b.size() && (a.values() - b.values()).norm() <= tolerance;
}

void appendUniqueSeed(std::vector<JointVector>& seeds, const JointVector& seed, double duplicateTolerance)
{
    for (const JointVector& existing : seeds) {
        if (sameJointVector(existing, seed, duplicateTolerance)) {
            return;
        }
    }
    seeds.push_back(seed);
}

double signedHalfRangeValue(const Joint& joint, int sign)
{
    if (!joint.limits.has_value()) {
        return joint.home;
    }
    if (sign < 0) {
        return 0.5 * (joint.limits->lower + std::min(0.0, joint.limits->upper));
    }
    return 0.5 * (std::max(0.0, joint.limits->lower) + joint.limits->upper);
}

void applyPostureBranchSeedValue(Eigen::VectorXd& q,
                                 const SerialRobotConfig& config,
                                 int movableIndex,
                                 const std::optional<int>& branch)
{
    if (!branch.has_value() || movableIndex < 0 || movableIndex >= q.size()) {
        return;
    }

    int index = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type != JointType::Revolute && joint.type != JointType::Prismatic) {
            continue;
        }
        if (index == movableIndex) {
            q[index] = signedHalfRangeValue(joint, *branch);
            return;
        }
        ++index;
    }
}

std::optional<JointVector> postureSeed(const SerialRobotConfig& config, const ArmPosture& posture)
{
    if (config.posture.resolver != "serial_6dof_shoulder_elbow_wrist"
        || ForwardKinematics::movableJointCount(config) < 6) {
        return std::nullopt;
    }

    Eigen::VectorXd q = midpointJoint(config).values();
    applyPostureBranchSeedValue(q, config, 0, posture.shoulder);
    applyPostureBranchSeedValue(q, config, 2, posture.elbow);
    applyPostureBranchSeedValue(q, config, 4, posture.wrist);
    return JointVector(q);
}

std::vector<JointVector> buildSeeds(const SerialRobotConfig& config,
                                    const IKRequest& request,
                                    const NumericalIKDefaults& defaults)
{
    std::vector<JointVector> seeds;
    if (request.previousJoint.has_value()) {
        appendUniqueSeed(seeds, *request.previousJoint, defaults.duplicateJointTolerance);
    }
    if (request.seedJoint.has_value()) {
        appendUniqueSeed(seeds, *request.seedJoint, defaults.duplicateJointTolerance);
    }
    if (request.posture.has_value()) {
        const std::optional<JointVector> seed = postureSeed(config, *request.posture);
        if (seed.has_value()) {
            appendUniqueSeed(seeds, *seed, defaults.duplicateJointTolerance);
        }
    }

    appendUniqueSeed(seeds, homeJoint(config), defaults.duplicateJointTolerance);
    appendUniqueSeed(seeds, midpointJoint(config), defaults.duplicateJointTolerance);

    const int dof = ForwardKinematics::movableJointCount(config);
    for (int s = 0; static_cast<int>(seeds.size()) < defaults.maxSeeds && s < defaults.maxSeeds * 4; ++s) {
        Eigen::VectorXd q(dof);
        int index = 0;
        for (const Joint& joint : config.joints) {
            if (joint.type != JointType::Revolute && joint.type != JointType::Prismatic) {
                continue;
            }
            const double lower = joint.limits.has_value() ? joint.limits->lower : -1.0;
            const double upper = joint.limits.has_value() ? joint.limits->upper : 1.0;
            const double fraction = std::fmod((s + 1) * (index + 1) * 0.6180339887498948, 1.0);
            q[index] = lower + fraction * (upper - lower);
            ++index;
        }
        appendUniqueSeed(seeds, JointVector(q), defaults.duplicateJointTolerance);
    }
    return seeds;
}

Residual computeResidual(const Pose& current, const Pose& target, const NumericalIKDefaults& defaults)
{
    Residual residual;
    const Eigen::Vector3d position = target.translation_m() - current.translation_m();
    const Eigen::Matrix3d rotationError = target.isometry().linear() * current.isometry().linear().transpose();
    const Eigen::AngleAxisd angleAxis(rotationError);
    Eigen::Vector3d orientation = Eigen::Vector3d::Zero();
    if (std::abs(angleAxis.angle()) > 1e-14) {
        orientation = angleAxis.axis() * angleAxis.angle();
    }

    residual.positionNorm = position.norm();
    residual.orientationNorm = std::abs(angleAxis.angle());
    residual.vector.template segment<3>(0) = defaults.positionResidualWeight * position;
    residual.vector.template segment<3>(3) = defaults.orientationResidualWeight * orientation;
    return residual;
}

Eigen::MatrixXd geometricJacobian(const FkChain& chain, const Pose& currentTool)
{
    Eigen::MatrixXd jacobian(6, static_cast<Eigen::Index>(chain.joints.size()));
    const Eigen::Vector3d end = currentTool.translation_m();

    for (Eigen::Index i = 0; i < static_cast<Eigen::Index>(chain.joints.size()); ++i) {
        const JointFrameData& joint = chain.joints[static_cast<std::size_t>(i)];
        if (joint.type == JointType::Revolute) {
            jacobian.block<3, 1>(0, i) = joint.axisInBase.cross(end - joint.originInBase);
            jacobian.block<3, 1>(3, i) = joint.axisInBase;
        } else if (joint.type == JointType::Prismatic) {
            jacobian.block<3, 1>(0, i) = joint.axisInBase;
            jacobian.block<3, 1>(3, i).setZero();
        } else {
            jacobian.block<6, 1>(0, i).setZero();
        }
    }
    return jacobian;
}

IKSolution scoreSolution(const SerialRobotConfig& config,
                         const IKRequest& request,
                         JointVector joints,
                         double positionError,
                         double orientationError,
                         const NumericalIKDefaults& defaults)
{
    return IKSolutionRanker::score(config, request, std::move(joints), positionError, orientationError,
                                   defaults.jointLimitAvoidanceWeight);
}

struct SolveAttempt {
    bool converged = false;
    IKSolution solution;
    KinematicsStatus failureStatus = KinematicsStatus::MaxIterationsReached;
};

SolveAttempt solveFromSeed(const SerialRobotConfig& config,
                           const IKSolveContext& context,
                           const JointVector& seed,
                           const NumericalIKDefaults& defaults)
{
    const int dof = ForwardKinematics::movableJointCount(config);
    Eigen::VectorXd q = clampToLimits(config, seed.values());
    double damping = defaults.initialDamping;
    double previousCost = std::numeric_limits<double>::infinity();

    for (int iteration = 0; iteration < defaults.maxIterations; ++iteration) {
        const JointVector currentJoints(q);
        const FkChain chain = ForwardKinematics::computeChain(config, currentJoints);
        const Pose current = chain.flangeInBase;
        const Residual residual = computeResidual(current, context.targetFlangeInBase, defaults);

        if (residual.positionNorm <= context.request.options.maxPositionError_m
            && residual.orientationNorm <= context.request.options.maxOrientationError_rad) {
            IKSolution solution = scoreSolution(config, context.request, currentJoints,
                                                residual.positionNorm, residual.orientationNorm, defaults);
            if (context.request.options.requirePosture && context.request.posture.has_value()
                && !IKSolutionRanker::postureMatches(solution.posture, *context.request.posture)) {
                return SolveAttempt{false, {}, KinematicsStatus::PostureConstraintUnsatisfied};
            }

            SolveAttempt attempt;
            attempt.converged = true;
            attempt.solution = std::move(solution);
            return attempt;
        }

        const double cost = residual.vector.squaredNorm();
        if (std::abs(previousCost - cost) <= defaults.costTolerance) {
            damping = std::min(defaults.maxDamping, damping * defaults.dampingIncreaseFactor);
        }
        previousCost = cost;

        const Eigen::MatrixXd jacobian = geometricJacobian(chain, current);
        const Eigen::Matrix<double, 6, 6> normal =
            jacobian * jacobian.transpose()
            + Eigen::Matrix<double, 6, 6>::Identity() * damping * damping;
        Eigen::VectorXd step = jacobian.transpose() * normal.ldlt().solve(residual.vector);
        if (!step.allFinite()) {
            return SolveAttempt{false, {}, KinematicsStatus::NumericalError};
        }
        const double stepNorm = step.norm();
        if (stepNorm <= defaults.stepTolerance) {
            return SolveAttempt{false, {}, KinematicsStatus::NoConvergedSolution};
        }
        if (stepNorm > defaults.maxJointStepNorm) {
            step *= defaults.maxJointStepNorm / stepNorm;
        }

        const Eigen::VectorXd candidate = clampToLimits(config, q + step);
        const Pose candidatePose = ForwardKinematics::flangePose(config, JointVector(candidate));
        const Residual candidateResidual = computeResidual(candidatePose, context.targetFlangeInBase, defaults);
        if (candidateResidual.vector.squaredNorm() < cost) {
            q = candidate;
            damping = std::max(defaults.minDamping, damping * defaults.dampingDecreaseFactor);
        } else {
            damping = std::min(defaults.maxDamping, damping * defaults.dampingIncreaseFactor);
        }
    }

    if (dof == 0) {
        return SolveAttempt{false, {}, KinematicsStatus::InvalidRobotConfig};
    }
    return SolveAttempt{false, {}, KinematicsStatus::MaxIterationsReached};
}

bool duplicateSolution(const std::vector<IKSolution>& solutions, const IKSolution& candidate, double tolerance)
{
    for (const IKSolution& solution : solutions) {
        if (sameJointVector(solution.joints, candidate.joints, tolerance)) {
            return true;
        }
    }
    return false;
}
}

NumericalIKSolver::NumericalIKSolver(NumericalIKDefaults defaults)
    : defaults_(defaults)
{
}

const char* NumericalIKSolver::name() const
{
    return "adaptive_damped_least_squares";
}

IKResult NumericalIKSolver::solve(const SerialRobotConfig& config, const IKSolveContext& context) const
{
    IKResult result = solveAll(config, context);
    if (result.solutions.size() > 1) {
        result.solutions.resize(1);
    }
    return result;
}

IKResult NumericalIKSolver::solveAll(const SerialRobotConfig& config, const IKSolveContext& context) const
{
    const ModelValidationResult model = RobotModelValidator::validateSerialRobotConfig(config);
    if (!model.ok()) {
        return IKResult{model.status(), {}, model.issues.front().message};
    }
    if (!finitePose(context.targetFlangeInBase)) {
        return IKResult{IKStatus::InvalidRequest, {}, "target pose contains non-finite values"};
    }

    const int dof = ForwardKinematics::movableJointCount(config);
    if ((context.request.seedJoint.has_value() && context.request.seedJoint->size() != dof)
        || (context.request.previousJoint.has_value() && context.request.previousJoint->size() != dof)) {
        return IKResult{IKStatus::JointDimensionMismatch, {}, "seed or previous joint dimension mismatch"};
    }

    IKResult result;
    result.status = IKStatus::NoConvergedSolution;
    KinematicsStatus lastFailure = KinematicsStatus::NoConvergedSolution;

    const std::vector<JointVector> seeds = buildSeeds(config, context.request, defaults_);
    for (const JointVector& seed : seeds) {
        const JointLimitCheck seedLimits = JointLimitValidator::validate(config, seed);
        if (seedLimits.status == KinematicsStatus::JointDimensionMismatch) {
            return IKResult{IKStatus::JointDimensionMismatch, {}, "generated seed dimension mismatch"};
        }

        const SolveAttempt attempt = solveFromSeed(config, context, seed, defaults_);
        if (attempt.converged) {
            if (!duplicateSolution(result.solutions, attempt.solution, defaults_.duplicateJointTolerance)) {
                result.solutions.push_back(attempt.solution);
            }
            if (context.request.options.maxSolutions > 0
                && static_cast<int>(result.solutions.size()) >= context.request.options.maxSolutions) {
                break;
            }
        } else {
            lastFailure = attempt.failureStatus;
        }
    }

    if (!result.solutions.empty()) {
        std::sort(result.solutions.begin(), result.solutions.end(), [](const IKSolution& a, const IKSolution& b) {
            return a.score.totalCost < b.score.totalCost;
        });
        result.status = IKStatus::Ok;
        return result;
    }

    result.status = lastFailure;
    result.message = "no converged numerical IK solution";
    return result;
}

} // namespace RobotKinematics
