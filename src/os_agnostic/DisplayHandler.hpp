/**
 * @file DisplayHandler.hpp
 * @brief Renders the console's marquee frames.
*/
#pragma once

#include "Context.hpp"
#include <atomic>
#include <string>

/**
 * @brief is in charge of the marquee's live console rendering.
 * This class uses the shared context to update the console.
 * It reacts to control flags such as active and pause and produces new
 * text frames that move around the screen.
 */
class DisplayHandler : public Handler {
public:
    /**
     * @brief Create a DisplayHandler that is connected to the shared context.
     * @param c Shared MarqueeContext for state access and synchronization.
    */
    explicit DisplayHandler(MarqueeContext& c) : Handler(c) {}

    /**
    * @brief The main thread function that manages the rendering of marquees.
    *
    * Loops and draws after waiting for other threads to be ready.
    * new frames based on the text and marquee state as of right now.
    */
    void operator()();

    /**
     * @brief initiates scrolling on the marquee display.
     */
    void start() { ctx.setMarqueeActive(true); }

    /**
     * @brief halts the animation of the marquee (freeze display).
    */
    void stop()  { ctx.setMarqueeActive(false); }

private:
    /**
     * @brief Move the text to the left by one character.
     * @param s Current marquee string.
     * After scrolling, @return New string.
    */
    std::string scrollOnce(const std::string& s);  // used internally during render loop
};
