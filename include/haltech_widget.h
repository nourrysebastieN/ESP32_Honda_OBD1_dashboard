/**
 * @file haltech_widget.h
 * @brief Reusable Haltech-style dual gauge widget for LVGL
 *
 * This widget recreates the look of the Haltech IC-10 cluster with
 * two analog gauges and stacked side indicator cards.
 */

#ifndef HALTECH_WIDGET_H
#define HALTECH_WIDGET_H

#include <array>
#include <cstdint>

#include <lvgl.h>

constexpr std::size_t HALTECH_CARD_COUNT = 4;

/**
 * @brief Configuration for an individual gauge inside the cluster
 */
struct HaltechGaugeConfig {
    const char* title;
    const char* unit;
    const char* centerLabel;
    const char* centerUnit;
    int32_t min;
    int32_t max;
    int32_t cautionThreshold;
    int32_t warningThreshold;
    uint16_t totalTicks;
    uint8_t majorTickEvery;
    int16_t needleOffset;
    bool syncDigital;
    bool showLabels;
};

/**
 * @brief Configuration for the stacked indicator cards
 */
struct HaltechSideCardConfig {
    const char* title;
    const char* unit;
    int32_t min;
    int32_t max;
    bool warnOnLow;
};

/**
 * @brief Complete widget configuration
 */
struct HaltechClusterConfig {
    bool enableRightGauge;
    HaltechGaugeConfig leftGauge;
    HaltechGaugeConfig rightGauge;
    std::array<HaltechSideCardConfig, HALTECH_CARD_COUNT> leftCards;
    std::array<HaltechSideCardConfig, HALTECH_CARD_COUNT> rightCards;
};

struct HaltechGaugeElements {
    lv_obj_t* container = nullptr;
    lv_obj_t* scale = nullptr;
    lv_obj_t* needle = nullptr;
    lv_obj_t* titleLabel = nullptr;
    lv_obj_t* centerLabel = nullptr;
    lv_obj_t* centerValue = nullptr;
    lv_obj_t* centerUnit = nullptr;
    lv_obj_t* bottomValueLabel = nullptr;
    lv_obj_t* bottomUnitLabel = nullptr;
    lv_obj_t* digitalCard = nullptr;
    lv_obj_t* digitalValue = nullptr;
    lv_obj_t* digitalUnit = nullptr;
    int16_t needleOffset = -30;
};

struct HaltechSideCard {
    lv_obj_t* container = nullptr;
    lv_obj_t* label = nullptr;
    lv_obj_t* valueLabel = nullptr;
    lv_obj_t* unitLabel = nullptr;
    lv_obj_t* bar = nullptr;
    int32_t min = 0;
    int32_t max = 100;
    bool warnOnLow = false;
};

/**
 * @brief Widget instance containing all LVGL objects
 */
struct HaltechClusterWidget {
    HaltechClusterConfig config;
    lv_obj_t* root = nullptr;
    HaltechGaugeElements leftGauge;
    HaltechGaugeElements rightGauge;
    std::array<HaltechSideCard, HALTECH_CARD_COUNT> leftCards;
    std::array<HaltechSideCard, HALTECH_CARD_COUNT> rightCards;
};

bool haltech_cluster_init(HaltechClusterWidget& widget, lv_obj_t* parent, const HaltechClusterConfig& config);
void haltech_cluster_set_left_value(HaltechClusterWidget& widget, int32_t value);
void haltech_cluster_set_right_value(HaltechClusterWidget& widget, int32_t value);
void haltech_cluster_set_card_value(HaltechClusterWidget& widget, bool leftColumn, uint8_t index, float value);
void haltech_cluster_set_gauge_unit(HaltechClusterWidget& widget, bool leftGauge, const char* unit);
void haltech_cluster_set_center_value(HaltechClusterWidget& widget, bool leftGauge, float value);
void haltech_cluster_set_center_unit(HaltechClusterWidget& widget, bool leftGauge, const char* unit);
void haltech_cluster_set_digital_value(HaltechClusterWidget& widget, bool leftGauge, int32_t value,
                                       int32_t cautionThreshold, int32_t warningThreshold);
lv_obj_t* haltech_cluster_get_root(HaltechClusterWidget& widget);

#endif /* HALTECH_WIDGET_H */
