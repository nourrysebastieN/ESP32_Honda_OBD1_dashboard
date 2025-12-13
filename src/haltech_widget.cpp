/**
 * @file haltech_widget.cpp
 * @brief Implementation of the Haltech-style dual gauge widget
 */

#include "haltech_widget.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

const lv_color_t COLOR_FRAME = lv_color_hex(0x050506);
const lv_color_t COLOR_PANEL = lv_color_hex(0x101114);
const lv_color_t COLOR_CARD = lv_color_hex(0x0A0B0D);
const lv_color_t COLOR_ACCENT = lv_color_hex(0xF0C44C);
const lv_color_t COLOR_WARNING = lv_color_hex(0xFF9F43);
const lv_color_t COLOR_DANGER = lv_color_hex(0xFF4D4F);
const lv_color_t COLOR_TEXT = lv_color_hex(0xF5F5F5);
const lv_color_t COLOR_TEXT_DIM = lv_color_hex(0x7A7B85);
const lv_color_t COLOR_NEEDLE = lv_color_hex(0xE44A3A);

static lv_style_t sectionNormalStyle;
static lv_style_t sectionCautionStyle;
static lv_style_t sectionWarningStyle;
static bool sectionStylesInitialized = false;

template <typename T>
T clamp_value(T value, T minValue, T maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

void init_section_styles(void) {
    if (sectionStylesInitialized) {
        return;
    }

    lv_style_init(&sectionNormalStyle);
    lv_style_set_arc_color(&sectionNormalStyle, COLOR_ACCENT);
    lv_style_set_arc_width(&sectionNormalStyle, 24);

    lv_style_init(&sectionCautionStyle);
    lv_style_set_arc_color(&sectionCautionStyle, COLOR_WARNING);
    lv_style_set_arc_width(&sectionCautionStyle, 24);

    lv_style_init(&sectionWarningStyle);
    lv_style_set_arc_color(&sectionWarningStyle, COLOR_DANGER);
    lv_style_set_arc_width(&sectionWarningStyle, 24);

    sectionStylesInitialized = true;
}

lv_obj_t* create_card_column(lv_obj_t* parent) {
    lv_obj_t* column = lv_obj_create(parent);
    lv_obj_set_width(column, 150);
    lv_obj_set_height(column, LV_PCT(100));
    lv_obj_set_style_bg_opa(column, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(column, 0, 0);
    lv_obj_set_style_pad_all(column, 0, 0);
    lv_obj_set_style_pad_row(column, 14, 0);
    lv_obj_set_style_pad_column(column, 0, 0);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(column, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(column, LV_OBJ_FLAG_SCROLLABLE);
    return column;
}

void build_side_cards(lv_obj_t* column,
                      const std::array<HaltechSideCardConfig, HALTECH_CARD_COUNT>& configs,
                      std::array<HaltechSideCard, HALTECH_CARD_COUNT>& outCards) {
    for (std::size_t i = 0; i < configs.size(); ++i) {
        const auto& cfg = configs[i];
        HaltechSideCard& card = outCards[i];

        card.container = lv_obj_create(column);
        lv_obj_set_size(card.container, LV_PCT(100), 78);
        lv_obj_set_style_bg_color(card.container, COLOR_CARD, 0);
        lv_obj_set_style_border_color(card.container, lv_color_hex(0x1D1E20), 0);
        lv_obj_set_style_border_width(card.container, 2, 0);
        lv_obj_set_style_radius(card.container, 10, 0);
        lv_obj_set_style_shadow_color(card.container, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(card.container, LV_OPA_40, 0);
        lv_obj_set_style_shadow_width(card.container, 10, 0);
        lv_obj_set_style_pad_all(card.container, 10, 0);
        lv_obj_clear_flag(card.container, LV_OBJ_FLAG_SCROLLABLE);

        card.label = lv_label_create(card.container);
        lv_label_set_text(card.label, cfg.title);
        lv_obj_set_style_text_font(card.label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(card.label, COLOR_TEXT_DIM, 0);
        lv_obj_align(card.label, LV_ALIGN_TOP_LEFT, 0, 0);

        card.valueLabel = lv_label_create(card.container);
        lv_label_set_text(card.valueLabel, "--");
        lv_obj_set_style_text_font(card.valueLabel, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(card.valueLabel, COLOR_ACCENT, 0);
        lv_obj_align(card.valueLabel, LV_ALIGN_TOP_RIGHT, 0, 0);

        card.unitLabel = lv_label_create(card.container);
        lv_label_set_text(card.unitLabel, cfg.unit);
        lv_obj_set_style_text_font(card.unitLabel, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(card.unitLabel, COLOR_TEXT_DIM, 0);
        lv_obj_align(card.unitLabel, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

        card.bar = lv_bar_create(card.container);
        lv_obj_set_width(card.bar, LV_PCT(100));
        lv_obj_set_height(card.bar, 10);
        lv_bar_set_range(card.bar, cfg.min, cfg.max);
        lv_bar_set_value(card.bar, cfg.min, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(card.bar, lv_color_hex(0x1E1F22), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(card.bar, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(card.bar, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(card.bar, 4, LV_PART_MAIN);
        lv_obj_set_style_bg_color(card.bar, COLOR_ACCENT, LV_PART_INDICATOR);
        lv_obj_set_style_radius(card.bar, 4, LV_PART_INDICATOR);
        lv_obj_align(card.bar, LV_ALIGN_BOTTOM_MID, 0, 0);

        card.min = cfg.min;
        card.max = cfg.max;
        card.warnOnLow = cfg.warnOnLow;
    }
}

static void build_center_hub(HaltechGaugeElements& gauge, const HaltechGaugeConfig& cfg) {
    lv_obj_t* hub = lv_obj_create(gauge.container);
    lv_obj_set_size(hub, 150, 150);
    lv_obj_center(hub);
    lv_obj_set_style_bg_color(hub, lv_color_hex(0x050505), 0);
    lv_obj_set_style_border_color(hub, lv_color_hex(0x28292B), 0);
    lv_obj_set_style_border_width(hub, 2, 0);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(hub, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* inner = lv_obj_create(hub);
    lv_obj_set_size(inner, 115, 115);
    lv_obj_center(inner);
    lv_obj_set_style_bg_color(inner, lv_color_hex(0x0E0E11), 0);
    lv_obj_set_style_border_width(inner, 0, 0);
    lv_obj_set_style_radius(inner, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(inner, LV_OBJ_FLAG_SCROLLABLE);

    gauge.centerLabel = lv_label_create(inner);
    lv_label_set_text(gauge.centerLabel, cfg.centerLabel != nullptr ? cfg.centerLabel : "");
    lv_obj_set_style_text_font(gauge.centerLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(gauge.centerLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(gauge.centerLabel, LV_ALIGN_TOP_MID, 0, 4);

    gauge.centerValue = lv_label_create(inner);
    lv_label_set_text(gauge.centerValue, "0");
    lv_obj_set_style_text_font(gauge.centerValue, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(gauge.centerValue, COLOR_TEXT, 0);
    lv_obj_align(gauge.centerValue, LV_ALIGN_CENTER, 0, -4);

    gauge.centerUnit = lv_label_create(inner);
    lv_label_set_text(gauge.centerUnit, cfg.centerUnit != nullptr ? cfg.centerUnit : "");
    lv_obj_set_style_text_font(gauge.centerUnit, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(gauge.centerUnit, COLOR_TEXT_DIM, 0);
    lv_obj_align(gauge.centerUnit, LV_ALIGN_BOTTOM_MID, 0, -6);
}

void build_gauge(HaltechGaugeElements& out,
                 lv_obj_t* parent,
                 const HaltechGaugeConfig& cfg,
                 bool alignRight) {
    out.container = lv_obj_create(parent);
    lv_obj_set_size(out.container, 340, 340);
    lv_obj_set_style_bg_color(out.container, COLOR_PANEL, 0);
    lv_obj_set_style_bg_grad_color(out.container, lv_color_hex(0x08090C), 0);
    lv_obj_set_style_bg_grad_dir(out.container, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(out.container, lv_color_hex(0x1B1C21), 0);
    lv_obj_set_style_border_width(out.container, 2, 0);
    lv_obj_set_style_radius(out.container, 170, 0);
    lv_obj_set_style_pad_all(out.container, 0, 0);
    lv_obj_set_style_shadow_color(out.container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(out.container, 20, 0);
    lv_obj_set_style_shadow_opa(out.container, LV_OPA_30, 0);
    lv_obj_clear_flag(out.container, LV_OBJ_FLAG_SCROLLABLE);
    if (alignRight) {
        lv_obj_set_style_margin_left(out.container, -50, 0);
    } else {
        lv_obj_set_style_margin_right(out.container, -50, 0);
    }

    out.scale = lv_scale_create(out.container);
    lv_obj_center(out.scale);
    lv_obj_set_size(out.scale, 300, 300);
    lv_obj_set_style_bg_opa(out.scale, LV_OPA_TRANSP, 0);
    lv_scale_set_mode(out.scale, LV_SCALE_MODE_ROUND_OUTER);
    lv_scale_set_angle_range(out.scale, 270);
    lv_scale_set_rotation(out.scale, 135);
    lv_scale_set_range(out.scale, cfg.min, cfg.max);
    lv_scale_set_total_tick_count(out.scale, cfg.totalTicks);
    lv_scale_set_major_tick_every(out.scale, cfg.majorTickEvery);
    lv_scale_set_label_show(out.scale, cfg.showLabels);
    lv_obj_set_style_line_width(out.scale, 3, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(out.scale, 2, LV_PART_ITEMS);
    lv_obj_set_style_length(out.scale, 18, LV_PART_INDICATOR);
    lv_obj_set_style_length(out.scale, 8, LV_PART_ITEMS);
    lv_obj_set_style_line_color(out.scale, COLOR_TEXT_DIM, LV_PART_ITEMS);
    if (cfg.showLabels) {
        lv_obj_set_style_text_font(out.scale, &lv_font_montserrat_12, LV_PART_ITEMS);
        lv_obj_set_style_text_color(out.scale, COLOR_TEXT, LV_PART_ITEMS);
    }

    init_section_styles();
    lv_scale_section_t* normal = lv_scale_add_section(out.scale);
    lv_scale_section_set_range(normal, cfg.min, std::min(cfg.cautionThreshold, cfg.max));
    lv_scale_section_set_style(normal, LV_PART_MAIN, &sectionNormalStyle);

    if (cfg.cautionThreshold < cfg.warningThreshold) {
        lv_scale_section_t* caution = lv_scale_add_section(out.scale);
        lv_scale_section_set_range(caution, cfg.cautionThreshold, std::min(cfg.warningThreshold, cfg.max));
        lv_scale_section_set_style(caution, LV_PART_MAIN, &sectionCautionStyle);
    }

    lv_scale_section_t* warning = lv_scale_add_section(out.scale);
    lv_scale_section_set_range(warning, cfg.warningThreshold, cfg.max);
    lv_scale_section_set_style(warning, LV_PART_MAIN, &sectionWarningStyle);

    out.needle = lv_line_create(out.scale);
    lv_obj_set_style_line_width(out.needle, 6, 0);
    lv_obj_set_style_line_color(out.needle, COLOR_NEEDLE, 0);
    lv_obj_set_style_line_rounded(out.needle, true, 0);
    out.needleOffset = cfg.needleOffset;
    lv_scale_set_line_needle_value(out.scale, out.needle, out.needleOffset, cfg.min);

    build_center_hub(out, cfg);

    out.titleLabel = lv_label_create(out.container);
    lv_label_set_text(out.titleLabel, cfg.title);
    lv_obj_set_style_text_font(out.titleLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(out.titleLabel, COLOR_TEXT, 0);
    lv_obj_align(out.titleLabel, LV_ALIGN_TOP_MID, 0, 12);

    out.digitalValue = lv_label_create(out.digitalCard);
    lv_label_set_text(out.digitalValue, "0");
    lv_obj_set_style_text_font(out.digitalValue, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(out.digitalValue, COLOR_TEXT, 0);
    lv_obj_align(out.digitalValue, LV_ALIGN_TOP_LEFT, 0, 0);

    out.digitalUnit = lv_label_create(out.digitalCard);
    lv_label_set_text(out.digitalUnit, cfg.unit);
    lv_obj_set_style_text_font(out.digitalUnit, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(out.digitalUnit, COLOR_TEXT_DIM, 0);
    lv_obj_align(out.digitalUnit, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /* Bottom stats (used for RPM gauge). */
    out.bottomValueLabel = lv_label_create(out.container);
    lv_label_set_text(out.bottomValueLabel, "0");
    lv_obj_set_style_text_font(out.bottomValueLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(out.bottomValueLabel, COLOR_TEXT, 0);
    lv_obj_align(out.bottomValueLabel, LV_ALIGN_BOTTOM_MID, 0, -10);

    out.bottomUnitLabel = lv_label_create(out.container);
    lv_label_set_text(out.bottomUnitLabel, cfg.unit);
    lv_obj_set_style_text_font(out.bottomUnitLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(out.bottomUnitLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align_to(out.bottomUnitLabel, out.bottomValueLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, -5);
}

void set_gauge_value(const HaltechGaugeConfig& cfg,
                     HaltechGaugeElements& gauge,
                     int32_t value) {
    if (gauge.scale == nullptr || gauge.needle == nullptr) {
        return;
    }

    const int32_t clamped = clamp_value(value, cfg.min, cfg.max);
    lv_scale_set_line_needle_value(gauge.scale, gauge.needle, gauge.needleOffset, clamped);

    if (cfg.syncDigital) {
        if (gauge.digitalValue != nullptr) {
            lv_label_set_text_fmt(gauge.digitalValue, "%d", clamped);
        }
        if (gauge.bottomValueLabel != nullptr) {
            lv_label_set_text_fmt(gauge.bottomValueLabel, "%d", clamped);
        }

        lv_color_t color = COLOR_ACCENT;
        if (clamped >= cfg.warningThreshold) {
            color = COLOR_DANGER;
        } else if (clamped >= cfg.cautionThreshold) {
            color = COLOR_WARNING;
        }

        if (gauge.digitalValue != nullptr) {
            lv_obj_set_style_text_color(gauge.digitalValue, color, 0);
        }
        if (gauge.bottomValueLabel != nullptr) {
            lv_obj_set_style_text_color(gauge.bottomValueLabel, color, 0);
        }
    }
}

void update_side_card(HaltechSideCard& card, float value) {
    if (card.bar == nullptr || card.valueLabel == nullptr) {
        return;
    }

    int32_t barValue = static_cast<int32_t>(std::lround(value));
    barValue = clamp_value(barValue, card.min, card.max);
    lv_bar_set_value(card.bar, barValue, LV_ANIM_ON);

    float normalized = 0.0f;
    if (card.max != card.min) {
        normalized = (static_cast<float>(barValue - card.min)) / static_cast<float>(card.max - card.min);
        normalized = clamp_value(normalized, 0.0f, 1.0f);
    }

    float risk = card.warnOnLow ? (1.0f - normalized) : normalized;
    lv_color_t color = COLOR_ACCENT;
    if (risk > 0.85f) {
        color = COLOR_DANGER;
    } else if (risk > 0.65f) {
        color = COLOR_WARNING;
    }

    char buffer[16];
    if (std::fabs(value) < 10.0f) {
        std::snprintf(buffer, sizeof(buffer), "%.1f", value);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.0f", value);
    }

    lv_label_set_text(card.valueLabel, buffer);
    lv_obj_set_style_text_color(card.valueLabel, color, 0);
    lv_obj_set_style_bg_color(card.bar, color, LV_PART_INDICATOR);
}

} // namespace

bool haltech_cluster_init(HaltechClusterWidget& widget,
                          lv_obj_t* parent,
                          const HaltechClusterConfig& config) {
    widget.config = config;

    widget.root = lv_obj_create(parent);
    if (widget.root == nullptr) {
        return false;
    }

    lv_obj_set_size(widget.root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(widget.root, COLOR_FRAME, 0);
    lv_obj_set_style_border_color(widget.root, lv_color_hex(0x1D1E22), 0);
    lv_obj_set_style_border_width(widget.root, 2, 0);
    lv_obj_set_style_radius(widget.root, 26, 0);
    lv_obj_set_style_pad_all(widget.root, 18, 0);
    lv_obj_set_style_shadow_color(widget.root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(widget.root, 18, 0);
    lv_obj_set_style_shadow_opa(widget.root, LV_OPA_30, 0);
    lv_obj_set_flex_flow(widget.root, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget.root, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(widget.root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* leftColumn = create_card_column(widget.root);
    build_side_cards(leftColumn, config.leftCards, widget.leftCards);

    lv_obj_t* centerArea = lv_obj_create(widget.root);
    lv_obj_set_flex_grow(centerArea, 1);
    lv_obj_set_height(centerArea, LV_PCT(100));
    lv_obj_set_style_bg_opa(centerArea, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(centerArea, 0, 0);
    lv_obj_set_flex_flow(centerArea, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(centerArea, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(centerArea, 0, 0);
    lv_obj_clear_flag(centerArea, LV_OBJ_FLAG_SCROLLABLE);

    build_gauge(widget.leftGauge, centerArea, config.leftGauge, false);
    if (config.enableRightGauge) {
        build_gauge(widget.rightGauge, centerArea, config.rightGauge, true);
    } else {
        widget.rightGauge.container = nullptr;
    }

    lv_obj_t* rightColumn = create_card_column(widget.root);
    build_side_cards(rightColumn, config.rightCards, widget.rightCards);

    return true;
}

void haltech_cluster_set_left_value(HaltechClusterWidget& widget, int32_t value) {
    set_gauge_value(widget.config.leftGauge, widget.leftGauge, value);
}

void haltech_cluster_set_right_value(HaltechClusterWidget& widget, int32_t value) {
    if (widget.config.enableRightGauge && widget.rightGauge.scale != nullptr) {
        set_gauge_value(widget.config.rightGauge, widget.rightGauge, value);
    }
}

void haltech_cluster_set_card_value(HaltechClusterWidget& widget,
                                    bool leftColumn,
                                    uint8_t index,
                                    float value) {
    auto& cards = leftColumn ? widget.leftCards : widget.rightCards;
    if (index >= cards.size()) {
        return;
    }
    update_side_card(cards[index], value);
}

void haltech_cluster_set_gauge_unit(HaltechClusterWidget& widget, bool leftGauge, const char* unit) {
    HaltechGaugeElements& gauge = leftGauge ? widget.leftGauge : widget.rightGauge;
    if (gauge.digitalUnit != nullptr && unit != nullptr) {
        lv_label_set_text(gauge.digitalUnit, unit);
    }
    if (leftGauge) {
        widget.config.leftGauge.unit = unit;
    } else if (widget.config.enableRightGauge) {
        widget.config.rightGauge.unit = unit;
    }
}

void haltech_cluster_set_center_value(HaltechClusterWidget& widget, bool leftGauge, float value) {
    HaltechGaugeElements& gauge = leftGauge ? widget.leftGauge : widget.rightGauge;
    if (gauge.centerValue == nullptr) {
        return;
    }

    char buffer[16];
    if (std::fabs(value) < 10.0f) {
        std::snprintf(buffer, sizeof(buffer), "%.1f", value);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.0f", value);
    }
    lv_label_set_text(gauge.centerValue, buffer);
}

void haltech_cluster_set_center_unit(HaltechClusterWidget& widget, bool leftGauge, const char* unit) {
    HaltechGaugeElements& gauge = leftGauge ? widget.leftGauge : widget.rightGauge;
    if (gauge.centerUnit != nullptr && unit != nullptr) {
        lv_label_set_text(gauge.centerUnit, unit);
    }
    if (leftGauge) {
        widget.config.leftGauge.centerUnit = unit;
    } else if (widget.config.enableRightGauge) {
        widget.config.rightGauge.centerUnit = unit;
    }
}

void haltech_cluster_set_digital_value(HaltechClusterWidget& widget, bool leftGauge, int32_t value,
                                       int32_t cautionThreshold, int32_t warningThreshold) {
    HaltechGaugeElements& gauge = leftGauge ? widget.leftGauge : widget.rightGauge;
    if (gauge.digitalValue == nullptr) {
        return;
    }

    lv_label_set_text_fmt(gauge.digitalValue, "%d", value);
    if (gauge.bottomValueLabel != nullptr) {
        lv_label_set_text_fmt(gauge.bottomValueLabel, "%d", value);
    }

    lv_color_t color = COLOR_ACCENT;
    if (value >= warningThreshold) {
        color = COLOR_DANGER;
    } else if (value >= cautionThreshold) {
        color = COLOR_WARNING;
    }

    lv_obj_set_style_text_color(gauge.digitalValue, color, 0);
    if (gauge.bottomValueLabel != nullptr) {
        lv_obj_set_style_text_color(gauge.bottomValueLabel, color, 0);
    }
}

lv_obj_t* haltech_cluster_get_root(HaltechClusterWidget& widget) {
    return widget.root;
}
