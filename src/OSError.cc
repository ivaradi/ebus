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

#include "OSError.h"

#include <cstring>

//------------------------------------------------------------------------------

using std::string;

//------------------------------------------------------------------------------

string OSError::toString(int errorNumber, const std::string& prefix)
{
    if (errorNumber<0) errorNumber = errno;
    char buf[1024];
    char* errorStr = strerror_r(errorNumber, buf, sizeof(buf));
    if (errorStr==0) {
        snprintf(buf, sizeof(buf), "Unknown error: %d", errorNumber);
    } else if (errorStr!=buf) {
        strncpy(buf, errorStr, sizeof(buf));
    }

    return prefix.empty() ? buf : (prefix + ": " + buf);
}


//------------------------------------------------------------------------------

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
