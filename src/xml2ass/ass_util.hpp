#ifndef BILIBILI_DANMAKU_ASS_UTIL_HPP
#define BILIBILI_DANMAKU_ASS_UTIL_HPP

#include <string>
#include <vector>
#include <array>
#include <bit>

#include "ass_render_utils.h"
#include "sc_item.h"
#include "ass_danmaku.h"

#include "thirdparty/fmt/include/fmt/format.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/libass/include/ass.h"

typedef struct ass_library ASS_Library;
typedef struct ass_renderer ASS_Renderer;
typedef struct ass_track ASS_Track;

extern "C" {
int ass_get_text_width(ASS_Renderer *render_priv, ASS_Track *track, int style_index,
                       char *text);
};

namespace ass {

template <typename Tp> class Point_ {
  public:
    Tp x_; //!< x coordinate of the point
    Tp y_; //!< y coordinate of the point

    Point_() = default;
    Point_(Tp x, Tp y) : x_(x), y_(y){};

    auto x() {
        return x_;
    }

    auto y() {
        return y_;
    }
};

typedef Point_<int> Point2i;
typedef Point2i Point;

template <typename Tp> class Rect_ {
  public:
    Tp x_;      //!< x coordinate of the top-left corner
    Tp y_;      //!< y coordinate of the top-left corner
    Tp width_;  //!< width of the rectangle
    Tp height_; //!< height of the rectangle

    Rect_() = default;
    Rect_(Tp x, Tp y, Tp width, Tp height)
        : x_(x), y_(y), width_(width), height_(height){};

    void set(Tp x, Tp y, Tp width, Tp height) {
        x_ = x;
        y_ = y;
        width_ = width;
        height_ = height;
    }

    Tp x() const {
        return x_;
    }

    Tp y() const {
        return y_;
    }

    Tp width() const {
        return width_;
    }

    Tp height() const {
        return height_;
    }

    Point_<Tp> topLeft() { //!< coordinate of the top-left corner
        return {x_, y_};
    }

    Point_<Tp> topRight() { //!< coordinate of the top-right corner
        return {x_ + width_, y_};
    }

    Point_<Tp> bottomRight() { //!< coordinate of the bottom-right corner
        return {x_ + width_, y_ + height_};
    }

    Point_<Tp> bottomLeft() { //!< coordinate of the bottom-left corner
        return {x_, y_ + height_};
    }
};

typedef Rect_<int> Rect2i;
typedef Rect2i Rect;

/**
 * A rounded rectangle drawn clockwise.
 *
 * The drawing direction from start point to end point is clockwise.
 *
 */
template <typename Tp> class RoundedRect_ {
  public:
    Rect_<Tp> rect_;
    int corner_radius_;

    RoundedRect_() : corner_radius_(0) {
    }

    RoundedRect_(Tp x, Tp y, Tp width, Tp height, Tp cornerRadius)
        : corner_radius_(cornerRadius), rect_(x, y, width, height) {
    }

    void set(Tp x, Tp y, Tp width, Tp height, Tp corner_radius) {
        rect_.set(x, y, width, height);
        this->corner_radius_ = corner_radius;
    }

    Tp width() const {
        return rect_.width_;
    }

    Tp height() const {
        return rect_.height_;
    }

    Tp radius() const {
        return corner_radius_;
    }

    Point_<Tp> topLeftStartPoint() {
        return Point_<Tp>(rect_.topLeft().x(), rect_.topLeft().y() + corner_radius_);
    }

    Point_<Tp> topLeftEndPoint() {
        return Point_<Tp>(rect_.topLeft().x() + corner_radius_, rect_.topLeft().y());
    }

    Point_<Tp> topRightStartPoint() {
        return Point_<Tp>(rect_.topRight().x() - corner_radius_, rect_.topRight().y());
    }

    Point_<Tp> topRightEndPoint() {
        return Point_<Tp>(rect_.topRight().x(), rect_.topRight().y() + corner_radius_);
    }

    Point_<Tp> bottomRightStartPoint() {
        return Point_<Tp>(rect_.bottomRight().x(),
                          rect_.bottomRight().y() - corner_radius_);
    }

    Point_<Tp> bottomRightEndPoint() {
        return Point_<Tp>(rect_.bottomRight().x() - corner_radius_,
                          rect_.bottomRight().y());
    }

    Point_<Tp> bottomLeftStartPoint() {
        return Point_<Tp>(rect_.bottomLeft().x() + corner_radius_,
                          rect_.bottomLeft().y());
    }

    Point_<Tp> bottomLeftEndPoint() {
        return Point_<Tp>(rect_.bottomLeft().x(),
                          rect_.bottomLeft().y() - corner_radius_);
    }

    /**
     * Get the control point array corresponding to a cubic bezier curve
     * for the topLeft segment of a circular.
     *
     * The elements in the array correspond to control point1,
     * control point2 and end point.
     *
     */
    std::array<Point_<Tp>, 3> topLeftBezierCurve() {
        /**
         * For cubic bessel curves, the control point coefficient C
         * is often taken as 0.552284749831. Since the radius of the circle here
         * is small, we choose a faster coefficient value of 0.5.
         * At this point, the error in the result is no more than 1px,
         * which is barely noticeable visually.
         *
         */
        return {Point_<Tp>{rect_.topLeft().x(),
                           rect_.topLeft().y() + corner_radius_ - corner_radius_ / 2},

                Point_<Tp>{rect_.topLeft().x() + corner_radius_ - corner_radius_ / 2,
                           rect_.topLeft().y()},
                topLeftEndPoint()};
    }

    std::array<Point_<Tp>, 3> topRightBezierCurve() {
        return {Point_<Tp>{rect_.topRight().x() - corner_radius_ + corner_radius_ / 2,
                           rect_.topRight().y()},

                Point_<Tp>{rect_.topRight().x(),
                           rect_.topRight().y() + corner_radius_ - corner_radius_ / 2},
                topRightEndPoint()};
    }

    std::array<Point_<Tp>, 3> bottomRightBezierCurve() {
        return {Point_<Tp>{rect_.bottomRight().x(),
                           rect_.bottomRight().y() - corner_radius_ + corner_radius_ / 2},

                Point_<Tp>{rect_.bottomRight().x() - corner_radius_ + corner_radius_ / 2,
                           rect_.bottomRight().y()},
                bottomRightEndPoint()};
    }

    std::array<Point_<Tp>, 3> bottomLeftBezierCurve() {
        return {Point_<Tp>{rect_.bottomLeft().x() + corner_radius_ - corner_radius_ / 2,
                           rect_.bottomLeft().y()},

                Point_<Tp>{rect_.bottomLeft().x(),
                           rect_.bottomLeft().y() - corner_radius_ + corner_radius_ / 2},
                bottomLeftEndPoint()};
    }
};

typedef RoundedRect_<int> RoundedRect2i;
typedef RoundedRect2i RoundedRect;

// BGR color list
constexpr auto SuperChatTextColor = "313131";
constexpr auto SuperChatUserBoxColor0 = "FCE8D8";
constexpr auto SuperChatUserBoxColor1 = "CAF9F8";
constexpr auto SuperChatUserBoxColor2 = "D4F6FF";
constexpr auto SuperChatUserBoxColor3 = "E5E5FF";
constexpr auto SuperChatContentBoxColor0 = "E4A47A";
constexpr auto SuperChatContentBoxColor1 = "76E8E9";
constexpr auto SuperChatContentBoxColor2 = "8CCEF7";
constexpr auto SuperChatContentBoxColor3 = "8C8CF7";
constexpr auto SuperChatUserIDColor0 = "8A3619";
constexpr auto SuperChatUserIDColor1 = "1A8B87";
constexpr auto SuperChatUserIDColor2 = "236C64";
constexpr auto SuperChatUserIDColor3 = "0F0F75";

/**
 * +----------------------------+
 * | User Name                  |
 * |                            |
 * | Super Chat Price           |
 * +----------------------------+
 * | Super Chat Content         |
 * |                            |
 * +----------------------------+
 *
 */
class SuperChatBox {
  public:
    RoundedRect_<int> user_rect_;
    RoundedRect_<int> content_rect_;

    SuperChatBox() = default;

    SuperChatBox(int x, int y, int width, int user_height, int content_height,
                 int corner_radius)
        : user_rect_(x, y, width, user_height, corner_radius),
          content_rect_(x, y, width, content_height, corner_radius) {
    }

    std::string getUserBoxOutlineAss() {
        auto top_left_curve = user_rect_.topLeftBezierCurve();
        auto top_right_curve = user_rect_.topRightBezierCurve();

        return fmt::format(
            "m {} {} b {} {} {} {} {} {} l {} {} b {} {} {} {} {} "
            "{} l {} {} l {} {}",
            // start point
            user_rect_.topLeftStartPoint().x(), user_rect_.topLeftStartPoint().y(),
            // top left curve
            top_left_curve[0].x(), top_left_curve[0].y(), top_left_curve[1].x(),
            top_left_curve[1].y(), top_left_curve[2].x(), top_left_curve[2].y(),
            // top line
            user_rect_.topRightStartPoint().x(), user_rect_.topRightStartPoint().y(),
            // top right curve
            top_right_curve[0].x(), top_right_curve[0].y(), top_right_curve[1].x(),
            top_right_curve[1].y(), top_right_curve[2].x(), top_right_curve[2].y(),
            // right line -> do not add corner
            user_rect_.rect_.bottomRight().x(), user_rect_.rect_.bottomRight().y(),
            // bottom line -> do not add corner
            user_rect_.rect_.bottomLeft().x(), user_rect_.rect_.bottomLeft().y());
    }

    std::string getContentBoxOutlineAss() {
        auto bottom_left_curve = content_rect_.bottomLeftBezierCurve();
        auto bottom_right_curve = content_rect_.bottomRightBezierCurve();

        return fmt::format(
            "m {} {} l {} {} l {} {} b {} {} {} {} {} {} l {} {} b {} {} {} {} {} {}",
            // start point
            content_rect_.rect_.topLeft().x(), content_rect_.rect_.topLeft().y(),
            // top line
            content_rect_.rect_.topRight().x(), content_rect_.rect_.topRight().y(),
            // right line
            content_rect_.bottomRightStartPoint().x(),
            content_rect_.bottomRightStartPoint().y(),
            // bottom right curve
            bottom_right_curve[0].x(), bottom_right_curve[0].y(),
            bottom_right_curve[1].x(), bottom_right_curve[1].y(),
            bottom_right_curve[2].x(), bottom_right_curve[2].y(),
            // bottom line
            content_rect_.bottomLeftStartPoint().x(),
            content_rect_.bottomLeftStartPoint().y(),
            // bottom left curve
            bottom_left_curve[0].x(), bottom_left_curve[0].y(), bottom_left_curve[1].x(),
            bottom_left_curve[1].y(), bottom_left_curve[2].x(), bottom_left_curve[2].y());
    }
};

class TextProcess {
  protected:
    TextProcess() = default;
    inline static ASS_Library *ass_library_ = nullptr;
    inline static ASS_Renderer *ass_renderer_ = nullptr;
    inline static ASS_Track *ass_track_ = nullptr;

  public:
    static void Init(ASS_Library *lib, ASS_Renderer *render, ASS_Track *track) {
        ass_library_ = lib;
        ass_renderer_ = render;
        ass_track_ = track;
    }

    static TextProcess *GetInstance() {
        static TextProcess instance{};
        return &instance;
    }

    int get_text_width(std::string &text) {
        return ass_get_text_width(ass_renderer_, ass_track_, 1,
                                  const_cast<char *>(text.c_str()));
    }

    /**
     * Handle long text by inserting line breaks at appropriate locations
     *
     * @param text [in&out] text to be converted
     * @param max_width max line width in pixel
     * @param font_size font size
     *
     * @return line num
     */
    int break_word(std::string &text, int max_width, int font_size);
};

inline int TextProcess::break_word(std::string &text, int max_width, int font_size) {
    assert(ass_library_ != nullptr);
    assert(ass_renderer_ != nullptr);
    assert(ass_track_ != nullptr);

    constexpr auto ASS_STYLE_PREFIX = "{\\c&HFFFFFF\\q2}";
    constexpr float LINE_COMPENSATION = 0.8f;
    constexpr float WORD_COMPENSATION = 0.9f;
    char new_line_arr[2] = {'\\', 'N'};
    const int line_max_word =
        static_cast<int>(max_width / (font_size * LINE_COMPENSATION));
    int ret = 0;

    std::string res;
    assert(!text.empty());
    res.resize(text.size() * (1 + sizeof(new_line_arr)) + 1);

    const char *left = text.c_str();
    const char *right = left;
    const char *const border = left + text.length();
    char *dst = const_cast<char *>(res.c_str());

    // always process text [left, right)

    auto word_iter_move_left = [](const char *iter, const char *left_border,
                                  int max_word) {
        int words = 0;
        const auto *p = reinterpret_cast<const unsigned char *>(iter);
        const auto *border = reinterpret_cast<const unsigned char *>(left_border);

        p--;
        while (words < max_word && p >= border) {
            if (*p < 0x80) {
                words++;
            } else if ((*p & 0xC0) == 0xC0) { // 0b11xxxxxx, utf8 first byte
                words++;
            }

            p--;
        }
        p++;

        return reinterpret_cast<const char *>(p);
    };

    auto word_iter_move_right = [](const char *iter, const char *right_border,
                                   int max_word) {
        int words = 0;
        const auto *p = reinterpret_cast<const unsigned char *>(iter);
        const auto *border = reinterpret_cast<const unsigned char *>(right_border);

        while (words < max_word && p < border) {
            if (*p & 0x80) {
                int utf8_byte_len =
                    std::popcount(static_cast<std::uint8_t>(*p & 0b11110000));
                p += utf8_byte_len;
            } else {
                p++;
            }
            words++;
        }

        return reinterpret_cast<const char *>(p);
    };

    while (left < border) {
        int width;

        right = word_iter_move_right(right, border, line_max_word);
        std::string tmp_str =
            std::string(ASS_STYLE_PREFIX) + std::string(left, right - left);
        width = ass_get_text_width(ass_renderer_, ass_track_, 1,
                                   const_cast<char *>(tmp_str.c_str()));
        if (width < max_width) {
            const char *accept_iter;
            const char *tmp_right;

            do {
                accept_iter = right;
                tmp_right = word_iter_move_right(right, border, 1);
                if (tmp_right == right) {
                    break; // fail to move
                }

                right = tmp_right;
                tmp_str = std::string(ASS_STYLE_PREFIX) + std::string(left, right - left);
                width = ass_get_text_width(ass_renderer_, ass_track_, 1,
                                           const_cast<char *>(tmp_str.c_str()));
            } while (width < max_width);

            right = accept_iter;
        } else if (width > max_width) {
            const char *tmp_right;

            do {
                tmp_right = word_iter_move_left(right, left, 1);
                if (tmp_right == right) {
                    break; // fail to move, may not happen
                }
                right = tmp_right;
                tmp_str = std::string(ASS_STYLE_PREFIX) + std::string(left, right - left);
                width = ass_get_text_width(ass_renderer_, ass_track_, 1,
                                           const_cast<char *>(tmp_str.c_str()));
            } while (width > max_width);
        }

        assert(left != right);

        // copy to dst
        memcpy(dst, left, right - left);
        dst += right - left;
        if (right != border) { // do not insert break word into last line
            // insert break word
            memcpy(dst, new_line_arr, sizeof(new_line_arr));
            dst += sizeof(new_line_arr);
        }

        ret++;
        left = right;
    }

    res.resize(dst - const_cast<char *>(res.c_str()));

    text = res;
    return ret;
}

struct SuperChatEvent {
    explicit SuperChatEvent(int startX, int startY, int endX, int endY, int startTime,
                            int endTime)
        : startX(startX), startY(startY), endX(endX), endY(endY), startTime(startTime),
          endTime(endTime) {
    }

    int startX;
    int startY;
    int endX;
    int endY;
    int startTime;
    int endTime;
};

enum class SuperChatMessageUpdateType {
    immediate = 0,
    delayed = 1,
};

class SuperChatMessage {
  public:
    SuperChatMessageUpdateType updateType_;
    sc::sc_item_t sc_;
    SuperChatBox sc_box_;

    int total_height_;
    int content_height_;
    int user_height_;
    int width_;
    int corner_radius_;
    int font_size_;

    std::string text_color_;
    std::string user_box_color_;
    std::string content_box_color_;
    std::string user_name_color_;

    std::string user_box_outline_ass_;
    std::string content_box_outline_ass_;

  private:
    int margin_left_;
    int margin_right_;
    int user_name_width_;
    bool price_no_break_line_;
    int no_break_line_price_x_offset_;
    std::vector<struct SuperChatEvent> event_list_;

  public:
    explicit SuperChatMessage(SuperChatMessageUpdateType updateType, sc::sc_item_t &sc,
                              int x, int y, int width, int corner_radius, int font_size,
                              bool price_no_break_line = false)
        : updateType_(updateType), sc_(std::move(sc)), corner_radius_(corner_radius),
          font_size_(font_size), width_(width), price_no_break_line_(price_no_break_line),
          no_break_line_price_x_offset_(0), user_name_width_(0) {
        const float line_top_margin = (float)font_size / 6.0f;
        margin_left_ = corner_radius / 2;
        margin_right_ = corner_radius / 4;

        width_auto_reset();

        std::string &sc_content = sc_.content_;
        auto break_line_handle = TextProcess::GetInstance();
        int line_num = break_line_handle->break_word(
            sc_content, width_ - margin_left_ - margin_right_, font_size);

        content_height_ = calc_content_box_height(font_size, line_num, corner_radius);
        if (price_no_break_line) {
            bool ret = show_price_on_same_line();
            if (ret) {
                user_height_ =
                    calc_user_box_no_break_line_height(font_size, corner_radius);
            } else {
                price_no_break_line_ = false;
                user_height_ = calc_user_box_height(font_size, corner_radius);
            }
        } else {
            user_height_ = calc_user_box_height(font_size, corner_radius);
        }

        total_height_ = content_height_ + user_height_ + line_top_margin;

        sc_box_ =
            SuperChatBox{x, y, width_, user_height_, content_height_, corner_radius_};

        user_box_outline_ass_ = sc_box_.getUserBoxOutlineAss();
        content_box_outline_ass_ = sc_box_.getContentBoxOutlineAss();


        const int price = sc_.price_;
        text_color_ = SuperChatTextColor;
        if (price < 100) {
            user_box_color_ = SuperChatUserBoxColor0;
            content_box_color_ = SuperChatContentBoxColor0;
            user_name_color_ = SuperChatUserIDColor0;
        } else if (price < 500) {
            user_box_color_ = SuperChatUserBoxColor1;
            content_box_color_ = SuperChatContentBoxColor1;
            user_name_color_ = SuperChatUserIDColor1;
        } else if (price < 1000) {
            user_box_color_ = SuperChatUserBoxColor2;
            content_box_color_ = SuperChatContentBoxColor2;
            user_name_color_ = SuperChatUserIDColor2;
        } else {
            user_box_color_ = SuperChatUserBoxColor3;
            content_box_color_ = SuperChatContentBoxColor3;
            user_name_color_ = SuperChatUserIDColor3;
        }
    }

    SuperChatMessage(const SuperChatMessage &) = delete;
    SuperChatMessage &operator=(const SuperChatMessage &) = delete;

    SuperChatMessage(SuperChatMessage &&) = default;
    SuperChatMessage &operator=(SuperChatMessage &&) = default;

    void getSuperChatAss(int startX, int startY, int endX, int endY, int startTime,
                         int endTime, std::vector<std::string> &line_list,
                         bool force_get);

    std::vector<struct SuperChatEvent> &get_event_list() {
        return event_list_;
    }

  private:
    static int round_up(int num, int divisor) {
        return (num + divisor - 1) / divisor;
    }

    static int calc_user_box_height(int font_size, int corner_radius) {
        return static_cast<int>(font_size + font_size * (0.8f) + corner_radius / 2);
    }

    static int calc_user_box_no_break_line_height(int font_size, int corner_radius) {
        int margin_bottom = corner_radius / 4;
        return static_cast<int>(font_size + corner_radius / 2 + margin_bottom);
    }

    static int calc_content_box_height(int font_size, int line_num, int corner_radius) {
        return static_cast<int>(line_num * font_size + corner_radius / 2);
    }

    // Checks if the username is too long and auto resizes the width
    void width_auto_reset() {
        auto text_handle = TextProcess::GetInstance();

        std::string user_name_ass =
            fmt::format("{{\\fs{}\\b0\\q2}}{}", font_size_, sc_.user_name_);

        user_name_width_ = text_handle->get_text_width(user_name_ass);

        if (user_name_width_ + margin_left_ + margin_right_ > width_) {
            width_ = user_name_width_ + margin_left_ + margin_right_ + 5; // px
            price_no_break_line_ = false;
        }
    }

    /**
     * Try to show username and price on the same line.
     *
     * @return true: can be shown on same line
     *         false: should break line
     */
    bool show_price_on_same_line() {
        constexpr int price_padding_left = 3; // px

        auto text_handle = TextProcess::GetInstance();

        std::string price_ass = fmt::format("{{\\b0\\q2}}🔋 {}", sc_.price_);
        int price_width = text_handle->get_text_width(price_ass);
        // clang-format off
        if (margin_left_ + user_name_width_ + price_padding_left + price_width +
            margin_right_ > width_) {
            return false;
        }
        // clang-format on

        no_break_line_price_x_offset_ = width_ - margin_right_ - price_width;
        return true;
    }
};

inline void updateEventList(int startX, int startY, int endX, int endY, int startTime,
                            int endTime, std::vector<struct SuperChatEvent> &event_list) {
    if (event_list.empty()) {
        event_list.emplace_back(startX, startY, endX, endY, startTime, endTime);
        return;
    }

    auto &last = event_list.back();
    if (last.endTime > startTime)
        last.endTime = startTime;

    // merge same event
    if (last.startX == last.endX && last.startY == last.endY && startX == endX &&
        startY == endY && last.startX == startX && last.startY == startY) {
        last.endTime = endTime;
    } else {
        event_list.emplace_back(startX, startY, endX, endY, startTime, endTime);
    }
}

inline void SuperChatMessage::getSuperChatAss(int startX, int startY, int endX, int endY,
                                              int startTime, int endTime,
                                              std::vector<std::string> &line_list,
                                              bool force_get = false) {
    if (force_get == false && updateType_ != SuperChatMessageUpdateType::immediate) {
        return updateEventList(startX, startY, endX, endY, startTime, endTime,
                               event_list_);
    }

    std::string pos;
    std::string start_time = ass::time2ass(startTime);
    std::string end_time = ass::time2ass(endTime);

    auto getPosAss = [](int startX, int startY, int endX, int endY) {
        if (startX == endX && startY == endY) {
            return fmt::format("\\pos({},{})", startX, startY);
        } else {
            return fmt::format("\\move({},{},{},{})", startX, startY, endX, endY);
        }
    };

    auto add_list = [&line_list](std::string &&str) {
        line_list.push_back(std::move(str));
    };

    // All items have the same starting point,
    // plus a suitable offset to keep them in the correct position.

    // user box -> bottom layer
    pos = getPosAss(startX, startY, endX, endY);
    add_list(fmt::format("Dialogue: 0,{},{},"
                         "sc,,0000,0000,0000,,"
                         "{{{}\\c&{}\\shad0\\p1}}"
                         "{}\n",
                         start_time, end_time, pos, user_box_color_,
                         user_box_outline_ass_));

    // content box -> bottom layer
    pos = getPosAss(startX, startY + user_height_, endX, endY + user_height_);
    add_list(fmt::format("Dialogue: 0,{},{},"
                         "sc,,0000,0000,0000,,"
                         "{{{}\\shad0\\p1\\c&{}}}"
                         "{}\n",
                         start_time, end_time, pos, content_box_color_,
                         content_box_outline_ass_));

    // username -> top layer
    pos = getPosAss(startX + corner_radius_ / 2, startY + corner_radius_ / 3,
                    endX + corner_radius_ / 2, endY + corner_radius_ / 3);
    add_list(fmt::format("Dialogue: 1,{},{},"
                         "sc,,0000,0000,0000,,"
                         "{{{}\\c&{}\\fs{}\\b0\\q2}}" // do not use bold: \\b1
                         "{}\n",
                         start_time, end_time, pos, user_name_color_, font_size_,
                         sc_.user_name_));

    // price -> top layer
    if (price_no_break_line_) {
        pos =
            getPosAss(startX + no_break_line_price_x_offset_, startY + corner_radius_ / 3,
                      endX + no_break_line_price_x_offset_, endY + corner_radius_ / 3);
        add_list(fmt::format("Dialogue: 1,{},{},"
                             "sc,,0000,0000,0000,,"
                             "{{{}\\c&{}\\b0\\q2}}" // do not use bold
                             "🔋 {}\n",
                             start_time, end_time, pos, text_color_, sc_.price_));
    } else {
        pos = getPosAss(
            startX + corner_radius_ / 2, startY + font_size_ + corner_radius_ / 3,
            endX + corner_radius_ / 2, endY + font_size_ + corner_radius_ / 3);
        add_list(fmt::format("Dialogue: 1,{},{},"
                             "sc,,0000,0000,0000,,"
                             "{{{}\\c&{}\\fs{}\\b0\\q2}}" // do not use bold
                             "🔋 {}\n",
                             start_time, end_time, pos, text_color_,
                             static_cast<int>(font_size_ * 0.8), sc_.price_));
    }

    // content -> top layer
    pos = getPosAss(startX + corner_radius_ / 2, startY + user_height_,
                    endX + corner_radius_ / 2, endY + user_height_);
    add_list(fmt::format("Dialogue: 1,{},{},"
                         "sc,,0000,0000,0000,,"
                         "{{{}\\c&HFFFFFF\\q2}}"
                         "{}\n",
                         start_time, end_time, pos, sc_.content_));
}

} // namespace ass

#endif //BILIBILI_DANMAKU_ASS_UTIL_HPP
