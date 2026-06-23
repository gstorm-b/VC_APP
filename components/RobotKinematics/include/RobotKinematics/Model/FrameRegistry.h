#pragma once

#include <RobotKinematics/Core/Ids.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <vector>

namespace RobotKinematics {

// Lookup registry for user-defined frames. Base and flange frames are intrinsic to
// the kinematic chain and are resolved by ForwardKinematics, not stored here.
class FrameRegistry {
public:
    FrameRegistry() = default;

    static FrameRegistry fromConfig(const SerialRobotConfig& config)
    {
        FrameRegistry registry;
        for (const UserFrame& frame : config.frames.userFrames) {
            registry.add(frame);
        }
        return registry;
    }

    void add(const UserFrame& frame) { frames_.push_back(frame); }

    bool contains(const FrameId& id) const { return find(id) != nullptr; }

    Result<UserFrame> get(const FrameId& id) const
    {
        if (const UserFrame* frame = find(id)) {
            return Result<UserFrame>::success(*frame);
        }
        return Result<UserFrame>::failure(KinematicsStatus::FrameNotFound, "frame not found: " + id.value);
    }

    const std::vector<UserFrame>& frames() const { return frames_; }

private:
    const UserFrame* find(const FrameId& id) const
    {
        for (const UserFrame& frame : frames_) {
            if (frame.id == id.value) {
                return &frame;
            }
        }
        return nullptr;
    }

    std::vector<UserFrame> frames_;
};

} // namespace RobotKinematics
