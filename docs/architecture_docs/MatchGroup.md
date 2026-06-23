# MatchGroup Class Documentation

## Overview

`MatchGroup` is a container for multiple template patterns and shared algorithm configuration. Each group represents a logical set of related patterns (e.g., all views of a single part) that are matched together using the same algorithm settings.

Responsibilities:
- Store multiple patterns with unique names and numbers
- Manage group metadata (name, index, picking box geometry)
- Store algorithm-specific configuration shared by all patterns
- Provide pattern lookup and mutation operations

A `MatchGroup` is owned by `PatternGroupManager` (UI-facing) or used directly by `ImageMatcher` (algorithm).

## Location

- **Header**: `src/matching/match_group.h`
- **Implementation**: `src/matching/match_group.cpp`
- **Namespace**: `mtc` (matching module)

## Related Structures

### MatchGroupConfig

Container for all group-level configuration.

```cpp
class MatchGroupConfig {
    std::wstring m_groupName;              // Group display name
    int m_groupIndex{0};                   // Group number (0-based)
    
    cv::Size2f m_pickingBoxSize{0.0, 0.0}; // Picking box dimensions (mm)
    double m_pickingBoxDistance{0.0};      // Distance parameter (mm)
    double m_pickingBoxAngle{0.0};         // Angle offset (degrees)
    
    cv::Point3f m_pickingOffset{0.0, 0.0, 0.0}; // 3D offset (X, Y, Z)
    
    // Algorithm config shared by all patterns in this group
    std::shared_ptr<IMatchTypeConfig> typeConfig;  // EdgeMatchConfig, etc.
    
    // Individual pattern configs
    std::vector<MatchPatternConfig> m_patterns;
};
```

| Field | Type | Purpose |
|-------|------|---------|
| `m_groupName` | `std::wstring` | Human-readable group name |
| `m_groupIndex` | `int` | Numeric group identifier |
| `m_pickingBoxSize` | `cv::Size2f` | Dimensions of the picking gripper box (mm) |
| `m_pickingBoxDistance` | `double` | Distance from match point (mm) |
| `m_pickingBoxAngle` | `double` | Angle offset for picking box (degrees) |
| `m_pickingOffset` | `cv::Point3f` | 3D offset vector (X, Y, Z) for picking position |
| `typeConfig` | `std::shared_ptr<IMatchTypeConfig>` | Algorithm-specific config (shared by all patterns) |
| `m_patterns` | `std::vector<MatchPatternConfig>` | Vector of pattern configs |

#### MatchGroupConfig Constructors

#### #### MatchGroupConfig()
Default constructor with all zeros/defaults.

#### #### MatchGroupConfig(const MatchGroupConfig& other)
Deep copy constructor; clones `typeConfig` via `clone()`.

#### #### MatchGroupConfig& operator=(const MatchGroupConfig& other)
Deep copy assignment operator.

#### #### MatchGroupConfig(MatchGroupConfig&&) = default
Move constructor (default).

#### #### MatchGroupConfig& operator=(MatchGroupConfig&&) = default
Move assignment operator (default).

#### MatchGroupConfig Typed Helpers

#### #### MatchingType matchingType() const noexcept
Returns the current algorithm type.

**Returns:**
- `MatchingType::EdgeBased` (default) or other types

#### #### void setMatchingType(MatchingType t)
Switches to a different matching algorithm.

**Parameters:**
- `t` — New algorithm type

**Side Effects:**
- Replaces `typeConfig` with a default instance of the new type
- Previous config is discarded

#### #### EdgeMatchConfig* edgeConfig() noexcept
Type-safe cast to edge-based configuration.

**Returns:**
- Pointer to `EdgeMatchConfig`, or `nullptr` if current type is not edge-based

#### #### template<typename T> T* configAs() noexcept
Generic typed cast for future algorithm types.

**Returns:**
- Pointer to config as type `T`, or `nullptr` if cast fails

---

## MatchGroup Class

Container and manager for pattern templates.

## Constructor & Destructor

#### #### MatchGroup()
Default constructor with empty name and index 0.

#### #### MatchGroup(std::wstring name, int index)
Constructs with specified name and index.

**Parameters:**
- `name` — Group name (e.g., "Front View Patterns")
- `index` — Group number (must be within valid range)

#### #### ~MatchGroup()
Destructor; releases all patterns.

## Public Methods

### Configuration

#### #### const MatchGroupConfig& config() const
Returns the group configuration.

**Returns:**
- Reference to the internal `MatchGroupConfig`

#### #### const std::wstring& name() const
Returns the group name.

#### #### int number() const
Returns the group index number.

#### #### void cloneConfigTo(MatchGroup& group)
Deep-copies this group's config to another group.

**Parameters:**
- `group` — Destination group (modified in-place)

**Side Effects:**
- All config fields copied to destination
- Destination `typeConfig` is cloned

### Pattern Lookup

#### #### std::vector<std::shared_ptr<MatchPattern>> patterns() const
Returns all patterns in the group.

**Returns:**
- Vector of shared pointers to `MatchPattern` objects

#### #### MatchPattern* findPatternByName(const std::wstring& name) const
Finds a pattern by name.

**Parameters:**
- `name` — Pattern name to search for

**Returns:**
- Raw pointer to pattern, or `nullptr` if not found

#### #### MatchPattern* findPatternByNumber(int number) const
Finds a pattern by index number.

**Parameters:**
- `number` — Pattern index to search for

**Returns:**
- Raw pointer to pattern, or `nullptr` if not found

#### #### bool containsPatternName(const std::wstring& name) const
Checks if a pattern name exists.

**Returns:**
- `true` if pattern with this name exists

#### #### bool containsPatternNumber(int number) const
Checks if a pattern number exists.

**Returns:**
- `true` if pattern with this number exists

#### #### int patternCount() const
Returns the number of patterns in the group.

#### #### bool isEmpty() const
Checks if the group has no patterns.

**Returns:**
- `true` if no patterns

### Pattern Mutations

#### #### ManagerResult addPattern(const MatchPatternConfig& config)
Creates a new pattern with the given configuration.

**Parameters:**
- `config` — Pattern configuration (name, image, number)

**Returns:**
- `ManagerResult` with success flag and error message

**Validation:**
- Pattern name must be unique within the group
- Pattern number must be unique within the group
- Number must be within valid range (see `validateIndexRange`)

**Failure reasons:**
- Name collision
- Number collision
- Invalid number range

#### #### ManagerResult removePattern(const std::wstring& patternName)
Removes a pattern by name.

**Parameters:**
- `patternName` — Name of pattern to remove

**Returns:**
- `ManagerResult` with success flag

#### #### ManagerResult removePatternByNumber(int number)
Convenience method to remove by number.

**Parameters:**
- `number` — Pattern number to remove

#### #### ManagerResult setPatternConfig(const std::wstring& currentName, const MatchPatternConfig& newConfig)
Replaces the entire configuration of a pattern.

**Parameters:**
- `currentName` — Current pattern name (used to locate)
- `newConfig` — New configuration (with potentially different name/number)

**Returns:**
- `ManagerResult` with validation errors (name/number collision)

#### #### ManagerResult renamePattern(const std::wstring& currentName, const std::wstring& newName)
Renames a pattern, checking for collisions.

**Parameters:**
- `currentName` — Current pattern name
- `newName` — New pattern name

**Returns:**
- `ManagerResult` with success/failure

#### #### ManagerResult renumberPattern(const std::wstring& patternName, int newNumber)
Changes a pattern's number.

**Parameters:**
- `patternName` — Pattern name
- `newNumber` — New pattern number

**Returns:**
- `ManagerResult` with validation results

#### #### ManagerResult setPatternImage(const std::wstring& patternName, const cv::Mat& image)
Updates a pattern's template image.

**Parameters:**
- `patternName` — Pattern to update
- `image` — New template image (OpenCV matrix)

**Returns:**
- `ManagerResult` with success flag

### Pattern Access

#### #### std::shared_ptr<MatchPattern> lastPatternAccess() const
Returns the most recently accessed or created pattern.

**Returns:**
- Shared pointer to pattern, or `nullptr` if no patterns

## Static Methods (Range Validation)

#### #### static bool validateIndexRange(int index)
Checks if an index is within the valid range.

**Parameters:**
- `index` — Index to validate

**Returns:**
- `true` if valid

**Default range:** 0 to 9 (10 groups/patterns max)

#### #### static bool setIndexRange(int min, int max)
Sets the valid range for group/pattern indices.

**Parameters:**
- `min` — Minimum valid index
- `max` — Maximum valid index

**Returns:**
- `true` if range set successfully

#### #### static int getMaxGroupRange()
Returns the maximum valid group index.

#### #### static int getMinGroupRange()
Returns the minimum valid group index.

#### #### static int getMaxPatternRange()
Returns the maximum valid pattern index within a group.

#### #### static int getMinPatternRange()
Returns the minimum valid pattern index.

## Static Members

#### #### static int m_min_index_range
Minimum allowed index (default: 0).

#### #### static int m_max_index_range
Maximum allowed index (default: 9).

## Usage Example

```cpp
// Create a group for "Part A" views
mtc::MatchGroup group(L"Part A Views", 0);

// Load template images
cv::Mat front = cv::imread("part_a_front.png", cv::IMREAD_GRAYSCALE);
cv::Mat side = cv::imread("part_a_side.png", cv::IMREAD_GRAYSCALE);

// Configure patterns
mtc::MatchPatternConfig patternCfg1;
patternCfg1.name = L"Front View";
patternCfg1.number = 0;
patternCfg1.image = front;

mtc::MatchPatternConfig patternCfg2;
patternCfg2.name = L"Side View";
patternCfg2.number = 1;
patternCfg2.image = side;

// Add patterns
auto result1 = group.addPattern(patternCfg1);
auto result2 = group.addPattern(patternCfg2);

if (!result1.success || !result2.success) {
    qDebug() << "Failed to add pattern:" << result1.error.c_str();
    return;
}

// Configure group picking box
group.config().m_pickingBoxSize = cv::Size2f(50.0, 50.0);
group.config().m_pickingBoxDistance = 100.0;

// Set algorithm config to edge-based
group.config().setMatchingType(mtc::MatchingType::EdgeBased);
auto edgeCfg = group.config().edgeConfig();
if (edgeCfg) {
    edgeCfg->minScore = 0.75;
}

// Later: find a pattern
auto pattern = group.findPatternByName(L"Front View");
if (pattern) {
    qDebug() << "Found pattern:" << pattern->name();
}

// Rename pattern
auto renameResult = group.renamePattern(L"Front View", L"Front Image");

// Count patterns
qDebug() << "Total patterns in group:" << group.patternCount();
```

## Related Classes

- **MatchPattern** (`matching/match_pattern.h`) — Individual template pattern
- **PatternGroupManager** (`matching/pattern_group_manager.h`) — UI-facing container
- **ImageMatcher** (`matching/image_matcher.h`) — Uses groups for matching
- **MatchGroupConfig** (this file) — Configuration data structure

