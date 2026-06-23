#pragma once

#include <RobotKinematics/Core/Ids.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <string>
#include <vector>

namespace RobotKinematics {

// Lookup registry for tool/TCP definitions with a configurable default tool.
class ToolRegistry {
public:
    ToolRegistry() = default;

    static ToolRegistry fromConfig(const SerialRobotConfig& config)
    {
        ToolRegistry registry;
        for (const Tool& tool : config.tools) {
            registry.add(tool);
        }
        registry.setDefault(ToolId{config.defaultToolId});
        return registry;
    }

    void add(const Tool& tool) { tools_.push_back(tool); }
    void setDefault(const ToolId& id) { defaultId_ = id.value; }

    bool contains(const ToolId& id) const { return find(id) != nullptr; }

    Result<Tool> get(const ToolId& id) const
    {
        if (const Tool* tool = find(id)) {
            return Result<Tool>::success(*tool);
        }
        return Result<Tool>::failure(KinematicsStatus::ToolNotFound, "tool not found: " + id.value);
    }

    Result<Tool> getDefault() const
    {
        if (defaultId_.empty()) {
            return Result<Tool>::failure(KinematicsStatus::ToolNotFound, "no default tool configured");
        }
        return get(ToolId{defaultId_});
    }

    const std::vector<Tool>& tools() const { return tools_; }
    const std::string& defaultId() const { return defaultId_; }

private:
    const Tool* find(const ToolId& id) const
    {
        for (const Tool& tool : tools_) {
            if (tool.id == id.value) {
                return &tool;
            }
        }
        return nullptr;
    }

    std::vector<Tool> tools_;
    std::string defaultId_;
};

} // namespace RobotKinematics
