# ImageMatcher Class Documentation

## Overview

`ImageMatcher` is the core template matching engine for the NCR picking system. It performs edge-based template matching to locate patterns within images, returning match results with position, rotation, and confidence scores.

Features:
- **Edge-based matching**: Robust to lighting and texture variations
- **Multi-layer matching**: Different scale levels for robust detection
- **SIMD optimization**: Accelerated correlation computation on modern CPUs
- **Rotated rectangle matching**: Detects objects at arbitrary angles
- **Overlap filtering**: Removes duplicate detections
- **ROI support**: Match only within a region of interest

## Location

- **Header**: `src/matching/image_matcher.h`
- **Implementation**: `src/matching/image_matcher.cpp`
- **Namespace**: `mtc` (matching module)

## Dependencies

**External:**
- OpenCV (`cv::Mat`, `cv::Point`, `cv::Rect`, `cv::RotatedRect`)

**Project Internal:**
- `matching/match_params.h` — Match algorithm parameters
- `matching/match_pattern.h` — Pattern model
- `matching/match_group.h` — Pattern group container
- `matching/match_object.h` — Match result object
- `matching/edge_match_config.h` — Edge matching configuration
- `matching/utils_block_max.h` — Block maximum filter

## Structs & Types

### MatchResult

Contains the result of a single matching operation.

```cpp
struct MatchResult {
    std::vector<MatchedObject> Objects;     // Detected patterns
    cv::Point  cropOffsetPoint;              // ROI offset from image origin
    double     ExecutionTime{0.0};           // Matching execution time (ms)
    cv::Mat    Image;                        // Source image (optional)
    int        imageCols{0};                 // Image width
    int        imageRows{0};                 // Image height
    bool       isFoundMatchObject{false};    // At least one match found?
    int        totalPossiblePicking{0};      // Count of valid picks
    bool       isAreaLessThanLimits{false};  // Area constraint violation?
};
```

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| `Objects` | `std::vector<MatchedObject>` | List of matched patterns (empty if no matches) |
| `cropOffsetPoint` | `cv::Point` | Top-left offset of ROI from image origin (0,0 if no ROI) |
| `ExecutionTime` | `double` | Time in milliseconds for the matching operation |
| `Image` | `cv::Mat` | Copy of the source image (stored if needed for visualization) |
| `imageCols` | `int` | Width of the source image (pixels) |
| `imageRows` | `int` | Height of the source image (pixels) |
| `isFoundMatchObject` | `bool` | `true` if at least one pattern was detected |
| `totalPossiblePicking` | `int` | Number of valid, non-overlapping pick candidates |
| `isAreaLessThanLimits` | `bool` | `true` if image area is below minimum threshold |

### PossibleObject (Private)

Internal structure used during matching to track candidate objects.

## Public Members

#### #### int max_pos_num = 70
Maximum number of candidate positions to consider during search. Higher values increase detection quality but slow matching. Default: **70**.

#### #### double max_overlap = 0.2
Maximum allowed overlap (0.0–1.0) between detected objects. Overlapping detections are filtered; set lower to be more strict. Default: **0.2** (20%).

#### #### MatchResult match_result
Public result object populated after each `matching()` call. Contains positions, angles, and confidence scores of all detections.

## Constructor & Destructor

#### #### ImageMatcher()
Constructs an image matcher with default parameters.

**Initialization:**
- `max_pos_num` = 70
- `max_overlap` = 0.2
- Pattern group initialized to empty

## Public Methods

### Pattern Management

#### #### MatchGroup* getPatternGroup()
Returns the pattern group (container for all patterns).

**Returns:**
- Pointer to the internal `MatchGroup`

**Note:** Patterns are added/removed via the group, not directly on the matcher.

#### #### MatchGroup* getModel()
Alias for `getPatternGroup()`.

#### #### int getTemplateSourceSize()
Returns the number of patterns in the group.

**Returns:**
- Integer count of patterns

### Source Image Setup

#### #### void setImageSource(std::string path)
Loads a source image from a file path.

**Parameters:**
- `path` — Absolute or relative file path to an image file

**Side Effects:**
- Reads image using OpenCV
- Stores in `m_img_source`

#### #### void setImageSource(cv::Mat img)
Sets the source image directly from an OpenCV matrix.

**Parameters:**
- `img` — Image matrix (any supported OpenCV format)

**Side Effects:**
- Stores reference in `m_img_source`

#### #### void setMatchSourceImage(cv::Mat& img)
Alternative method to set the source image.

**Parameters:**
- `img` — Image matrix

### Region of Interest (ROI)

#### #### void setMatchingROI(cv::Point tl, cv::Point br)
Restricts matching to a rectangular region of interest.

**Parameters:**
- `tl` — Top-left corner of ROI
- `br` — Bottom-right corner of ROI

**Side Effects:**
- Stores ROI bounds in `ROI_tl` and `ROI_br`
- Subsequent matching operations only consider pixels within this rectangle

**Example:**
```cpp
matcher.setMatchingROI(cv::Point(100, 100), cv::Point(500, 400));
```

### Matching

#### #### void matching(cv::Mat& image, bool boundingBoxChecking, int objectsNum, bool usingRoi)
Performs template matching on a provided image.

**Parameters:**
- `image` — Source image to search
- `boundingBoxChecking` — If `true`, filter results by bounding box constraints
- `objectsNum` — Maximum number of results to return
- `usingRoi` — If `true`, use the ROI set by `setMatchingROI()`

**Side Effects:**
- Populates `match_result` with detected patterns
- Updates `match_result.Objects` with match positions, angles, scores

**Example:**
```cpp
cv::Mat image = cv::imread("scene.jpg");
matcher.matching(image, true, 10, false);
if (matcher.match_result.isFoundMatchObject) {
    for (const auto& obj : matcher.match_result.Objects) {
        qDebug() << "Match at" << obj.Point.x << "," << obj.Point.y;
    }
}
```

#### #### void matching(bool boundingBoxChecking, int objectsNum, bool usingRoi)
Performs template matching using the source image set via `setImageSource()`.

**Parameters:**
- `boundingBoxChecking` — Filter results by bounding box
- `objectsNum` — Max results to return
- `usingRoi` — Use ROI if set

**Side Effects:**
- Same as above; operates on `m_img_source` instead of a provided image

## Private Methods (Internal Algorithm)

#### #### bool MatchEdgePattern(cv::Mat& matSrc, MatchPattern* model, cv::Mat& matResult, int layer, double minScore, const EdgeMatchConfig& cfg)
Matches a single pattern at a specific scale layer using edge detection.

**Parameters:**
- `matSrc` — Source image
- `model` — Pattern to match
- `matResult` — Correlation result map (filled by the method)
- `layer` — Scale layer index
- `minScore` — Minimum correlation threshold
- `cfg` — Edge matching configuration (edge type, parameters)

**Returns:**
- `true` if match was found
- `false` otherwise

#### #### bool MatchEdgePattern_SIMD(cv::Mat& matSrc, MatchPattern* model, cv::Mat& matResult, int layer, double minScore)
SIMD-optimized variant of `MatchEdgePattern` for faster correlation computation.

#### #### cv::Point GetNextMaxLoc(cv::Mat& matResult, cv::Point ptMaxLoc, cv::Size sizeTemplate, double& maxValue, double maxOverlap)
Finds the next local maximum in the correlation map, excluding previously found matches.

#### #### bool SubPixEsimation(std::vector<MatchParams>* vec, double* dNewX, double* dNewY, double* dNewAngle, double dAngleStep, int iMaxScoreIndex)
Refines match position and angle to sub-pixel accuracy using quadratic interpolation.

#### #### void FilterWithScore(std::vector<MatchParams>* vec, double dScore)
Removes matches below a confidence threshold.

#### #### void FilterWithRotatedRect(std::vector<MatchParams>* vec, int iMethod, double dMaxOverLap, std::vector<int>* del_indexes = nullptr)
Removes overlapping matches, keeping only the highest-scoring ones.

#### #### void SortPtWithCenter(std::vector<cv::Point2f>& vecSort)
Sorts match points by distance from the image center.

#### #### void DrawMatchResult(cv::Mat& drawImage, std::vector<MatchedObject>& matchedResults)
(Optional) Draws match results on an image for visualization/debugging.

#### #### cv::Size GetBestRotationSize(cv::Size sizeSrc, cv::Size sizeDst, double rAngle)
Computes the rotated bounding box size for a rotated template.

#### #### cv::Point2f ptRotatePt2f(cv::Point2f ptInput, cv::Point2f ptOrg, double angle)
Rotates a 2D point around an origin by a given angle.

#### #### void GetRotatedROI(cv::Mat& matSrc, cv::Size size, cv::Point2f ptLT, double dAngle, cv::Mat& matROI)
Extracts a rotated rectangular region from an image.

## Private Members

| Member | Type | Purpose |
|--------|------|---------|
| `m_img_source` | `cv::Mat` | Current source image |
| `m_model_src` | `MatchGroup` | Container for all patterns to match |
| `ROI_tl` | `cv::Point` | ROI top-left corner |
| `ROI_br` | `cv::Point` | ROI bottom-right corner |
| `m_final_overlap_result` | `std::vector<MatchParams>` | Final deduplicated matches |

## Algorithm Overview

The edge-based matching algorithm proceeds as follows:

1. **Edge extraction**: Compute edge maps from both source image and patterns
2. **Multi-scale search**: Perform correlation at multiple scale layers
3. **Correlation**: Compute cross-correlation between pattern edges and image edges
4. **Peak detection**: Find local maxima in the correlation map
5. **Sub-pixel refinement**: Refine position and angle using quadratic interpolation
6. **Overlap filtering**: Remove duplicate detections based on `max_overlap`
7. **Ranking**: Sort results by confidence score
8. **Bounding box filtering**: Optional filtering based on object size constraints

## Constants

#### #### double SOURCE_THRESHOLD_TOLERANCE = 0.3
Tolerance factor for source edge strength threshold. Used to adjust edge sensitivity.

## Usage Example

```cpp
// Create matcher
mtc::ImageMatcher matcher;

// Add patterns to matcher's group
auto group = matcher.getPatternGroup();
cv::Mat pattern1 = cv::imread("pattern1.png", cv::IMREAD_GRAYSCALE);
auto matchPattern = new mtc::MatchPattern("Part A", pattern1);
group->addPattern(matchPattern);

// Load source image
cv::Mat scene = cv::imread("scene.jpg", cv::IMREAD_GRAYSCALE);

// Optional: Set ROI
matcher.setMatchingROI(cv::Point(50, 50), cv::Point(800, 600));

// Perform matching
matcher.matching(scene, true, 5, true);

// Process results
if (matcher.match_result.isFoundMatchObject) {
    qDebug() << "Found" << matcher.match_result.Objects.size() << "matches";
    qDebug() << "Execution time:" << matcher.match_result.ExecutionTime << "ms";
    
    for (const auto& obj : matcher.match_result.Objects) {
        qDebug() << "Match:" 
                 << "(" << obj.Point.x << "," << obj.Point.y << ")"
                 << "Angle:" << obj.Angle 
                 << "Score:" << obj.Score;
    }
}
```

## Performance Considerations

- **SIMD optimization**: Automatically used on CPUs with SSE/AVX support
- **Multi-layer matching**: More layers improve robustness but increase time
- **ROI usage**: Restricting to ROI significantly speeds up matching
- **Max overlap**: Stricter values (lower) reduce output count but increase precision

## Related Classes

- **MatchPattern** (`matching/match_pattern.h`) — Individual pattern model
- **MatchGroup** (`matching/match_group.h`) — Container for multiple patterns
- **MatchedObject** (`matching/match_object.h`) — Single match result
- **EdgeMatchConfig** (`matching/edge_match_config.h`) — Algorithm configuration
- **PatternGroupManager** (`matching/pattern_group_manager.h`) — UI-facing pattern manager

