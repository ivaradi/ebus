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

//------------------------------------------------------------------------------

#include "Log.h"

#include <ctime>
#include <cstring>

//------------------------------------------------------------------------------

using std::string;

//------------------------------------------------------------------------------

bool Log::useStdout = false;

string Log::logFilePath;

FILE* Log::logFile = 0;

bool Log::shouldReopenFile = false;

//------------------------------------------------------------------------------

void Log::log(bool error, const char* format, va_list& ap)
{
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, ap);

    time_t t = time(0);
    struct tm lt;
    localtime_r(&t, &lt);
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &lt);

    char buffer1[1100];
    snprintf(buffer1, sizeof(buffer), "[%s] %s\n", timeStr, buffer);

    if (useStdout) {
        fwrite(buffer1, strlen(buffer1), 1, error ? stderr : stdout);
    }

    if (!logFilePath.empty()) {
        if (shouldReopenFile && logFile!=0) {
            fclose(logFile); logFile = 0;
        }

        if (logFile==0) {
            logFile = fopen(logFilePath.c_str(), "at");
        }

        if (logFile!=0) {
            fwrite(buffer1, strlen(buffer1), 1, logFile);
            fflush(logFile);
        }
    }
    shouldReopenFile = false;
}

//------------------------------------------------------------------------------

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
