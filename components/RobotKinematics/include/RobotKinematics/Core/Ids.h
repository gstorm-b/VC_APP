#pragma once

#include <string>
#include <utility>

namespace RobotKinematics {

// Strongly-typed identifier for a reference frame (base, flange, or a user frame).
struct FrameId {
    std::string value;

    FrameId() = default;
    FrameId(std::string v) : value(std::move(v)) {}
    FrameId(const char* v) : value(v) {}

    bool empty() const { return value.empty(); }
};

inline bool operator==(const FrameId& a, const FrameId& b) { return a.value == b.value; }
inline bool operator!=(const FrameId& a, const FrameId& b) { return !(a == b); }

// Strongly-typed identifier for a tool / TCP definition.
struct ToolId {
    std::string value;

    ToolId() = default;
    ToolId(std::string v) : value(std::move(v)) {}
    ToolId(const char* v) : value(v) {}

    bool empty() const { return value.empty(); }
};

inline bool operator==(const ToolId& a, const ToolId& b) { return a.value == b.value; }
inline bool operator!=(const ToolId& a, const ToolId& b) { return !(a == b); }

} // namespace RobotKinematics
