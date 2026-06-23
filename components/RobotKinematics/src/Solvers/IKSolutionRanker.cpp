#include <RobotKinematics/Solvers/IKSolutionRanker.h>

#include <RobotKinematics/Posture/PostureResolver.h>

#include <algorithm>

namespace RobotKinematics {

namespace {
bool optionalBranchMatches(const std::optional<int>& candidate, const std::optional<int>& requested)
{
    return !requested.has_value() || (candidate.has_value() && *candidate == *requested);
}

double jointLimitMarginCost(const SerialRobotConfig& config, const JointVector& joints)
{
    double cost = 0.0;
    int index = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type != JointType::Revolute && joint.type != JointType::Prismatic) {
            continue;
        }
        if (joint.limits.has_value()) {
            const double range = joint.limits->upper - joint.limits->lower;
            const double margin = std::min(joints[index] - joint.limits->lower, joint.limits->upper - joints[index]);
            if (range > 0.0) {
                cost += 1.0 / std::max(margin / range, 1e-6);
            }
        }
        ++index;
    }
    return cost;
}

double postureMismatchCost(const ArmPosture& candidate, const ArmPosture& requested)
{
    double cost = 0.0;
    if (!optionalBranchMatches(candidate.shoulder, requested.shoulder)) {
        cost += 1.0;
    }
    if (!optionalBranchMatches(candidate.elbow, requested.elbow)) {
        cost += 1.0;
    }
    if (!optionalBranchMatches(candidate.wrist, requested.wrist)) {
        cost += 1.0;
    }
    for (const auto& requestedVendor : requested.vendorSpecific) {
        const auto candidateVendor = candidate.vendorSpecific.find(requestedVendor.first);
        if (candidateVendor == candidate.vendorSpecific.end() || candidateVendor->second != requestedVendor.second) {
            cost += 1.0;
        }
    }
    return cost;
}
}

bool IKSolutionRanker::postureMatches(const ArmPosture& candidate, const ArmPosture& requested)
{
    return postureMismatchCost(candidate, requested) == 0.0;
}

IKSolution IKSolutionRanker::score(const SerialRobotConfig& config,
                                   const IKRequest& request,
                                   JointVector joints,
                                   double positionError_m,
                                   double orientationError_rad,
                                   double jointLimitAvoidanceWeight)
{
    IKSolution solution;
    solution.joints = std::move(joints);
    solution.positionError_m = positionError_m;
    solution.orientationError_rad = orientationError_rad;

    if (std::unique_ptr<PostureResolver> resolver = PostureResolverFactory::create(config)) {
        const Result<ArmPosture> posture = resolver->classify(config, solution.joints);
        if (posture.ok()) {
            solution.posture = posture.value;
        }
    }

    solution.score.jointLimitMarginCost = jointLimitAvoidanceWeight * jointLimitMarginCost(config, solution.joints);
    if (request.seedJoint.has_value() && request.seedJoint->size() == solution.joints.size()) {
        solution.score.seedDistanceCost = (solution.joints.values() - request.seedJoint->values()).norm();
    }
    if (request.previousJoint.has_value() && request.previousJoint->size() == solution.joints.size()) {
        solution.score.motionContinuityCost = (solution.joints.values() - request.previousJoint->values()).norm();
    }
    if (request.posture.has_value()) {
        solution.score.postureMismatchCost = 100.0 * postureMismatchCost(solution.posture, *request.posture);
    }
    solution.score.totalCost = solution.positionError_m + solution.orientationError_rad
                               + solution.score.seedDistanceCost + solution.score.motionContinuityCost
                               + solution.score.jointLimitMarginCost + solution.score.postureMismatchCost;
    return solution;
}

} // namespace RobotKinematics
