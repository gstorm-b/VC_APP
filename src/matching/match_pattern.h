#ifndef MATCH_PATTERN_H
#define MATCH_PATTERN_H

#include <vector>
#include <opencv2/core.hpp>
#include "match_pattern_config.h"
#include "match_pattern_layer.h"
#include "manager_result.h"

namespace mtc {

class MatchGroup;
class EdgeMatchConfig;

// ---------------------------------------------------------------------------
// MatchPattern — a single learned template.
//
// Carries a MatchPatternConfig (identity + per-pattern search params) plus the
// data computed by learnPattern(): image pyramids, edge point arrays, contour
// masks, etc.  The algorithm (Edge-Based) config is shared at the group level
// (MatchGroupConfig::typeConfig) and fetched via the parent group.
//
// To change per-pattern settings after construction, retrieve a copy of the
// config, modify the relevant fields, then call setConfig().
// ---------------------------------------------------------------------------
class MatchPattern {
public:
    MatchPattern();
    MatchPattern(const MatchPatternConfig& config, MatchGroup* parent);

    std::wstring name()   const { return m_config.m_patternName; }
    int          number() const { return m_config.m_patternIndex; }
    cv::Mat      image()        { return m_config.m_rawImage.clone(); }

    const MatchPatternConfig& config() const { return m_config; }

    // ── Config update API ─────────────────────────────────────────────────
    void setConfig(MatchPatternConfig& cfg);
    MatchPatternConfig        patternConfig()    const;
    const MatchPatternConfig* patternConfigPtr() const;

    // ── Image API ─────────────────────────────────────────────────────────
    void setRawImage(std::wstring path);
    void setRawImage(cv::Mat& imgPattern);
    cv::Mat      getRawImage()              const;
    cv::MatSize  getRawImageSize();
    cv::Mat      getImageWithPickPosition();
    cv::Mat      getImageWithCannyThreshold();

    // ── Learn API ─────────────────────────────────────────────────────────
    bool learnPattern(std::wstring path);
    bool learnPattern(cv::Mat& imgPattern);
    bool learnPattern();

    // ── State queries ─────────────────────────────────────────────────────
    bool isImageEmpty()     const { return m_config.m_rawImage.empty(); }
    bool isPatternLearned() const { return m_hasPatternLearned; }

    // ── Learned-data accessors (used by ImageMatcher) ─────────────────────
    const std::vector<cv::Mat>*         getPyramid()  const { return &m_pyramids; }
    const std::vector<PatternLayer>*    getPatterns() const { return &m_patterns; }
    cv::Mat getPatternImage() const { return m_config.m_rawImage.clone(); }
    int     getTopLayer()     const { return m_topLayer; }
    int     borderColor()     const { return m_borderColor; }

    int getModelContoursSelectedIndex()    const { return m_contoursSelectIndex; }
    int getModelContoursSelectedArea()     const { return m_contoursSelectArea; }
    int getModelContoursSelectedRectArea() const { return m_contoursSelectRectArea; }

    cv::Mat getMaskImage()   const { return m_maskImage.clone(); }
    cv::Mat getPatternMask() const { return m_maskPatternPoint.clone(); }

    void        setImageSaveName(std::string name) { m_iamgeSaveName = name; }
    std::string imageSaveName()                    { return m_iamgeSaveName; }

private:
    void clearPatternData();
    void findTopLayer();

    // Edge algorithm config is shared at the group level — fetch it from the
    // parent group (nullptr if no parent or the group is not EdgeBased).
    const EdgeMatchConfig* groupEdgeConfig() const;

    friend class MatchGroup;
    ManagerResult setConfig(const MatchPatternConfig& cfg);
    ManagerResult setName(const std::wstring& name);
    ManagerResult setNumber(int number);
    void          setImage(const cv::Mat& image);

private:
    MatchPatternConfig m_config;
    MatchGroup*        m_parentGroup{nullptr};

    cv::Mat m_image;
    cv::Mat m_maskImage;
    cv::Mat m_maskPatternPoint;
    bool    m_hasPatternLearned{false};
    int     m_minReduceArea{0};
    int     m_topLayer{0};
    int     m_borderColor{0};

    std::vector<cv::Mat>       m_pyramids;
    std::vector<PatternLayer>  m_patterns;

    int             m_contoursSelectIndex{0};
    int             m_contoursSelectArea{0};
    cv::RotatedRect m_contoursSelectRect;
    int             m_contoursSelectRectArea{0};

    std::string m_iamgeSaveName;
};

} // namespace mtc
#endif // MATCH_PATTERN_H
