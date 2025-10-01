/**
 * @file FileReaderHandler.hpp
 * @brief returns content to callbacks after loading ASCII files.
*/
#pragma once

#include "Context.hpp"
#include <functional>
#include <string>

/**
 * @brief manages reading from ASCII (plain text) files.
 *
 * Helpful for importing scripts or marquee messages. It displays the
 * file content and forwards it to a callback, enabling handlers such as
 * CommandHandler to use external input to update the display.
*/
class FileReaderHandler : public Handler {
public:

    /**
     * @brief Provides access to shared context when building the file reader.
     * @param c The global MarqueeContext is referenced.
     */
    explicit FileReaderHandler(MarqueeContext& c) : Handler(c) {}

    /**
     * @brief sends the content to the specified callback after reading a file.
     *
     * Logs an error and invokes the callback if the file cannot be opened or read.
     * with a blank string in its place.
     *
     * @param path The path to load the text file.
     * @param cb Callback function that gets the contents of the file.
    s */
    void loadASCII(const std::string& path, std::function<void(const std::string&)> cb);
};
