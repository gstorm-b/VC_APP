#include <RobotKinematics/Kinematics/SerialRobotKinematics.h>

#include <RobotKinematics/Model/FrameRegistry.h>
#include <RobotKinematics/Model/RobotModelValidator.h>
#include <RobotKinematics/Model/ToolRegistry.h>
#include <RobotKinematics/Solvers/Analytic6DofSphericalWristSolver.h>
#include <RobotKinematics/Solvers/NumericalIKSolver.h>

#include <Eigen/Geometry>

#include <utility>

namespace RobotKinematics {

namespace {
Result<Pose> referenceFrameInBase(const SerialRobotConfig& config, const FrameId& id)
{
    if (id.empty() || id.value == config.frames.baseLinkId || id.value == "base") {
        return Result<Pose>::success(Pose::identity());
    }

    const FrameRegistry frames = FrameRegistry::fromConfig(config);
    Result<UserFrame> frame = frames.get(id);
    if (!frame.ok()) {
        return Result<Pose>::failure(frame.status, frame.message);
    }
    if (frame.value.parentLinkId != config.frames.baseLinkId) {
        return Result<Pose>::failure(KinematicsStatus::InvalidRequest,
                                     "IK reference frame must be fixed to the base link");
    }
    return Result<Pose>::success(frame.value.transform);
}

Result<Tool> resolveTool(const SerialRobotConfig& config, const ToolId& id)
{
    const ToolRegistry tools = ToolRegistry::fromConfig(config);
    return id.empty() ? tools.getDefault() : tools.get(id);
}
}

SerialRobotKinematics::SerialRobotKinematics(SerialRobotConfig config)
    : config_(std::move(config))
{
}

const SerialRobotConfig& SerialRobotKinematics::config() const
{
    return config_;
}

IKResult SerialRobotKinematics::solve(const IKRequest& request) const
{
    return run(request, false);
}

IKResult SerialRobotKinematics::solveAll(const IKRequest& request) const
{
    return run(request, true);
}

IKResult SerialRobotKinematics::run(const IKRequest& request, bool allSolutions) const
{
    const ModelValidationResult model = RobotModelValidator::validateSerialRobotConfig(config_);
    if (!model.ok()) {
        return IKResult{model.status(), {}, model.issues.front().message};
    }

    const Result<Tool> tool = resolveTool(config_, request.tool);
    if (!tool.ok()) {
        return IKResult{tool.status, {}, tool.message};
    }

    const Result<Pose> baseFromReference = referenceFrameInBase(config_, request.referenceFrame);
    if (!baseFromReference.ok()) {
        return IKResult{baseFromReference.status, {}, baseFromReference.message};
    }

    IKSolveContext context;
    context.request = request;
    context.targetFlangeInBase = baseFromReference.value * request.targetPose * tool.value.flangeToTcp.inverse();

    // Hybrid policy: use the analytic plugin when it supports the model and returns a
    // solution; otherwise fall back to the numerical solver.
    const Analytic6DofSphericalWristSolver analytic;
    if (analytic.supportsModel(config_)) {
        const IKResult analyticResult =
            allSolutions ? analytic.solveAll(config_, context) : analytic.solve(config_, context);
        if (analyticResult.ok()) {
            return analyticResult;
        }
    }

    const NumericalIKSolver solver;
    return allSolutions ? solver.solveAll(config_, context) : solver.solve(config_, context);
}

} // namespace RobotKinematics
