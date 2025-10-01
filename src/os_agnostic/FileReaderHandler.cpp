/**
 * @file FileReaderHandler.cpp
 * @brief returns content to callbacks after loading ASCII files.
 */


#include "FileReaderHandler.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * @brief sends the contents of an ASCII text file to the specified callback after loading it.
 *
 * Tries to open the file at the specified path. reads the entire content if successful.
 * and uses the outcome to invoke the callback. Upon failure (read error or file not found),
 * passes an empty string to the callback and outputs an error to the console.
 *
 * @param path The path of the file to load.
 * @param cb Callback to obtain the contents of the file (or, if unsuccessful, an empty string).
 */
void FileReaderHandler::loadASCII(const std::string& path, std::function<void(const std::string&)> cb) {
    try {
        std::ifstream in(path);  // open file stream

        if (!in) {
            std::lock_guard<std::mutex> lock(ctx.coutMutex);
            std::cout << "Error: cannot open file: " << path << std::endl;

            if (cb) cb(std::string{});  // To indicate failure, pass an empty string.
            return;
        }

        std::ostringstream ss;
        ss << in.rdbuf();  // fill the buffer with the entire file.

        auto content = ss.str();  // turn it into an actual string

        if (cb) cb(content);  // provide the caller with content

    } catch (const std::exception& e) {
        // There was some sort of read error (permissions, I/O, etc.).
        std::lock_guard<std::mutex> lock(ctx.coutMutex);
        std::cout << "Error reading file '" << path << "': " << e.what() << std::endl;

        if (cb) cb(std::string{});  // fallback content that is empty
    }
}
