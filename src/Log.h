// Copyright (c) 2016 by István Váradi

// This file is part of eBUS, an eBUS handler utility

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef LOG_H
#define LOG_H
//------------------------------------------------------------------------------

#include <string>

#include <cstdio>
#include <cstdarg>
#include <cassert>

//------------------------------------------------------------------------------

/**
 * Support for logging
 */
class Log
{
private:
    /**
     * Indicate if logging should go to the standard output
     */
    static bool useStdout;

    /**
     * The path of the log file, if logging into a file is enabled.
     */
    static std::string logFilePath;

    /**
     * The output file stream.
     */
    static FILE* logFile;

    /**
     * Indicate if the log file should be repoened.
     */
    static bool shouldReopenFile;

public:
    /**
     * Enable logging to the standard output.
     */
    static void enableStdout();

    /**
     * Disable logging to the standard output.
     */
    static void disableStdout();

    /**
     * Enable logging to the given file.
     */
    static void enableFile(const std::string& path);

    /**
     * Disable logging to a file.
     */
    static void disableFile();

    /**
     * Close and reopen the log file the next time it is written to.
     */
    static void reopenFile();

    /**
     * Normal log.
     */
    static void info(const char* format, ...)
        __attribute__((format (printf, 1, 2)));

    /**
     * Error log.
     */
    static void error(const char* format, ...)
        __attribute__((format (printf, 1, 2)));

private:
    /**
     * Perform the logging using the given argument list.
     */
    static void log(bool error, const char* format, va_list& ap);
};

//------------------------------------------------------------------------------
// Inline definitions
//------------------------------------------------------------------------------

inline void Log::enableStdout()
{
    useStdout = true;
}

//------------------------------------------------------------------------------

inline void Log::disableStdout()
{
    useStdout = false;
}

//------------------------------------------------------------------------------

inline void Log::enableFile(const std::string& path)
{
    assert(logFilePath.empty());
    logFilePath = path;
}

//------------------------------------------------------------------------------

inline void Log::disableFile()
{
    if (logFile!=0) {
        fclose(logFile);
        logFile = 0;
    }
    logFilePath.clear();
}

//------------------------------------------------------------------------------

inline void Log::reopenFile()
{
    shouldReopenFile = true;
}

//------------------------------------------------------------------------------

inline void Log::info(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    log(false, format, ap);
    va_end(ap);
}

//------------------------------------------------------------------------------

inline void Log::error(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    log(true, format, ap);
    va_end(ap);
}

//------------------------------------------------------------------------------
#endif // LOG_H

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
