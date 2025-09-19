/*
    Pure animation logic for the marquee: centering vs. scrolling, and box
    composition (borders). No threads here. DisplayHandler asks for a frame
    every tick. Keeping logic separate makes DisplayHandler much easier to read.
*/

#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

#pragma once

class AnimationHandler {
public:
    // total size INCLUDING borders (min 3x3 so we have a 1x1 inner area)
    void setSize(int w, int h) {
        std::lock_guard<std::mutex> lk(mtx_);
        width_ = std::max(3, w);
        height_ = std::max(3, h);
        scrollOffset_ = 0;
    }

    void setText(const std::string& s) {
        std::lock_guard<std::mutex> lk(mtx_);
        text_ = s;
        scrollOffset_ = 0;
    }

    void setSpeed(int ms) {
        std::lock_guard<std::mutex> lk(mtx_);
        speedMs_ = std::max(1, ms);
    }

    int speedMs() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return speedMs_;
    }

    void resetScroll() {
        std::lock_guard<std::mutex> lk(mtx_);
        scrollOffset_ = 0;
    }

    // build one full frame (top/bottom border, inner rows).
    // if scrolling=false: centered text on the middle inner row.
    // if scrolling=true : horizontal scroll in that row.
    std::vector<std::string> buildFrame(bool scrolling) {
        // snapshot state under lock
        int W, H, innerW, innerH, centerRow;
        std::string T;
        size_t localOffset;

        {
            std::lock_guard<std::mutex> lk(mtx_);
            W = width_;
            H = height_;
            innerW = std::max(1, W - 2);
            innerH = std::max(1, H - 2);
            centerRow = innerH / 2;

            T = text_;
            localOffset = scrollOffset_;

            if (scrolling) {
                const int loopLen = std::max(1, (int)(T.size() + gap_.size()));
                scrollOffset_ = (scrollOffset_ + 1) % loopLen;
            }
        }

        const std::string border = "+" + std::string(std::max(1, innerW), '-') + "+";
        const std::string empty = "|" + std::string(std::max(1, innerW), ' ') + "|";

        // middle row content
        std::string content(std::max(1, innerW), ' ');
        if (!T.empty()) {
            if (!scrolling) {
                // static: center and clip if needed
                if ((int)T.size() >= innerW) {
                    content = T.substr(0, innerW);
                }
                else {
                    int left = (innerW - (int)T.size()) / 2;
                    int right = innerW - (int)T.size() - left;
                    content = std::string(left, ' ') + T + std::string(right, ' ');
                }
            }
            else {
                // scrolling window over (text + gap)
                const std::string loop = T + gap_;
                // make sure we can safely slice regardless of pos/width
                std::string repeated;
                while ((int)repeated.size() < innerW + (int)localOffset + (int)loop.size())
                    repeated += loop;

                const size_t pos = localOffset % loop.size();
                if (pos + innerW <= repeated.size()) {
                    content = repeated.substr(pos, innerW);
                }
                else {
                    const size_t first = repeated.size() - pos;
                    content = repeated.substr(pos, first) + repeated.substr(0, innerW - first);
                }
            }
        }
        const std::string middle = "|" + content + "|";

        // assemble frame
        std::vector<std::string> lines;
        lines.reserve(H);
        lines.push_back(border);
        for (int r = 0; r < innerH; ++r) {
            lines.push_back(r == (innerH / 2) ? middle : empty);
        }
        lines.push_back(border);
        return lines;
    }

private:
    mutable std::mutex mtx_;

    // basic state
    std::string text_{ "CSU Marquee Emulator" };
    int speedMs_{ 120 };   // 120ms ~ 8.3fps; set 17 for ~60fps if you like
    int width_{ 36 };
    int height_{ 7 };

    // animation state
    size_t scrollOffset_{ 0 };
    const std::string gap_{ "   " }; // a little breathing room between repeats
};
