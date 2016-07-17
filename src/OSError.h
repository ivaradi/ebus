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

#ifndef OSERROR_H
#define OSERROR_H
//------------------------------------------------------------------------------

#include <string>
#include <system_error>

//------------------------------------------------------------------------------

/**
 * An error thrown for an OS error represented by the errno.
 */
class OSError : public std::system_error
{
public:
    /**
     * Get the string representation of the given error code.
     */
    std::string toString(int errorNumber, const std::string& prefix = "");

public:
    /**
     * Construct the exception.
     */
    OSError(const std::string& prefix = "", int errorNumber = -1);
};


//------------------------------------------------------------------------------
// Inline definitions
//------------------------------------------------------------------------------

inline OSError::OSError(const std::string& prefix, int errorNumber) :
    std::system_error(std::error_code((errorNumber<0) ? errno : errorNumber,
                                      std::system_category()),
                      prefix)
{
}

//------------------------------------------------------------------------------
#endif // OSERROR_H

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
