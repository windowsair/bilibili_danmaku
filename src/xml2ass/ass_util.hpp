#ifndef BILIBILI_DANMAKU_ASS_UTIL_HPP
#define BILIBILI_DANMAKU_ASS_UTIL_HPP

#include <string>
#include <vector>
#include <array>
#include <bit>

#include "sc_item.h"
#include "ass_danmaku.h"

#include "thirdparty/fmt/include/fmt/format.h"
#include "thirdparty/fmt/include/fmt/core.h"

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
 * |----------------------------|
 * | User Name                  |
 * |                            |
 * | Super Chat Price           |
 * |----------------------------|
 * | Super Chat Content         |
 * |                            |
 * |----------------------------|
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

class SuperChatMessage {
  public:
    sc::sc_item_t sc_;
    SuperChatBox sc_box_;

    int total_height_;
    int content_height_;
    int user_height_;
    int width_;
    const int corner_radius_;
    const int font_size_;

    std::string text_color_;
    std::string user_box_color_;
    std::string content_box_color_;
    std::string user_name_color_;

    std::string user_box_outline_ass_;
    std::string content_box_outline_ass_;

    explicit SuperChatMessage(sc::sc_item_t &sc, int x, int y, int width,
                              int corner_radius,
                              int font_size) // TODO: may be we can remove x/y?
        : sc_(std::move(sc)), corner_radius_(corner_radius), font_size_(font_size),
          width_(width) {
        const float line_top_margin = (float)font_size / 6.0f;
        constexpr float SC_BOX_STR_LEN_COMPENSATION = 0.9f;

        std::string &sc_content = sc_.content_;
        auto &sc_content_len = sc_.content_len_;
        int line_num = static_cast<int>(
            (sc_content_len * font_size * SC_BOX_STR_LEN_COMPENSATION) / width + 1);
        if (line_num > 1) {
            int line_max_word = round_up(sc_content_len, line_num);
            insert_new_line(sc_content, line_max_word, line_num);
        }


        content_height_ = calc_content_box_height(font_size, line_num, corner_radius);
        user_height_ = calc_user_box_height(font_size, corner_radius);
        total_height_ = content_height_ + user_height_ + line_top_margin;

        sc_box_ = SuperChatBox{x, y, width, user_height_, content_height_, corner_radius};

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
                         int endTime, std::vector<std::string> &line_list);

  private:
    static int round_up(int num, int divisor) {
        return (num + divisor - 1) / divisor;
    }

    static int calc_user_box_height(int font_size, int corner_radius) {
        return static_cast<int>(font_size + font_size * (0.8f) + corner_radius / 2);
    }

    static int calc_content_box_height(int font_size, int line_num, int corner_radius) {
        return static_cast<int>(line_num * font_size + corner_radius / 2);
    }


    void insert_new_line(std::string &s, int line_max_word, int line_num) {
        std::string res;
        char new_line_arr[2] = {'\\', 'N'};
        size_t sz = s.size();
        size_t word_count = 0, i = 0;
        int utf8_byte_len;
        res.resize(sz + sizeof(new_line_arr) * (line_num - 1));
        unsigned char *src =
            reinterpret_cast<unsigned char *>(const_cast<char *>(s.c_str()));
        unsigned char *dst =
            reinterpret_cast<unsigned char *>(const_cast<char *>(res.c_str()));

        while (i < sz) {
            if (src[i] & 0x80) {
                utf8_byte_len =
                    std::popcount(static_cast<std::uint8_t>(src[i] & 0b11110000));
                memcpy(dst, &src[i], utf8_byte_len);
                dst += utf8_byte_len;
                i += utf8_byte_len;
            } else {
                *dst++ = src[i++];
            }

            word_count++;
            // insert \N
            if (word_count % line_max_word == 0 && i < sz) {
                memcpy(dst, new_line_arr, sizeof(new_line_arr));
                dst += sizeof(new_line_arr);
            }
        }

        s = res;
    }
};

inline void SuperChatMessage::getSuperChatAss(int startX, int startY, int endX, int endY,
                                              int startTime, int endTime,
                                              std::vector<std::string> &line_list) {
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
                         "{{{}\\c&{}\\fs{}\\b1\\q2}}"
                         "{}\n",
                         start_time, end_time, pos, user_name_color_, font_size_,
                         sc_.user_name_));

    // price -> top layer
    pos = getPosAss(startX + corner_radius_ / 2, startY + font_size_ + corner_radius_ / 3,
                    endX + corner_radius_ / 2, endY + font_size_ + corner_radius_ / 3);
    add_list(fmt::format("Dialogue: 1,{},{},"
                         "sc,,0000,0000,0000,,"
                         "{{{}\\c&{}\\fs{}\\q2}}"
                         "🔋 {}\n",
                         start_time, end_time, pos, text_color_,
                         static_cast<int>(font_size_ * 0.8), sc_.price_));

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
