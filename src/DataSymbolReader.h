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

#ifndef DATASYMBOLREADER_H
#define DATASYMBOLREADER_H
//------------------------------------------------------------------------------

#include "Telegram.h"

#include "util.h"

//------------------------------------------------------------------------------

/**
 * An exception thrown when an overrun occurs while reading symbols.
 */
class OverrunException
{
};

//------------------------------------------------------------------------------

/**
 * A reader for data symbols to be used for setting up commands from telegrams.
 */
class DataSymbolReader
{
private:
    /**
     * The number of symbols.
     */
    size_t numDataSymbols;

    /**
     * The data symbols
     */
    const symbol_t* dataSymbols;

    /**
     * Indicate if an overrun is acceptable.
     */
    bool overrunOK;

    /**
     * The offset to be read next.
     */
    size_t nextOffset;

public:
    /**
     * Construct the given data symbol reader from the data in the given
     * telegram.
     *
     * @param useReply if true, the slave reply data will be used
     */
    DataSymbolReader(const Telegram& telegram, bool useReply = false,
                     bool overrunOK = false);

    /**
     * Get an 8-bit value. If an overrun occurs, and it is OK, return the given
     * default value.
     */
    uint8_t get8(uint8_t defaultValue) throw(OverrunException);

    /**
     * Get a 16-bit value. If an overrun occurs, and it is OK, return the given
     * default value. If only one byte could be read, that will be used to
     * replace the lower byte of the default value.
     */
    uint16_t get16(uint16_t defaultValue) throw(OverrunException);

    /**
     * Read a single 8-bit value.
     */
    operator uint8_t() throw(OverrunException);

    /**
     * Read a 16-bit value. The result is in host byte order.
     */
    operator uint16_t() throw(OverrunException);
};

//------------------------------------------------------------------------------
// Inline definitions
//------------------------------------------------------------------------------

inline DataSymbolReader::
DataSymbolReader(const Telegram& telegram, bool useReply, bool overrunOK) :
    numDataSymbols(useReply ?
                   telegram.numReplyDataSymbols : telegram.numDataSymbols),
    dataSymbols(useReply ?
                telegram.replyDataSymbols : telegram.dataSymbols),
    overrunOK(overrunOK),
    nextOffset(0)
{
}

//------------------------------------------------------------------------------

inline uint8_t DataSymbolReader::get8(uint8_t defaultValue)
    throw(OverrunException)
{
    if (nextOffset>=numDataSymbols) {
        if (overrunOK) return defaultValue;
        throw OverrunException();
    }
    return dataSymbols[nextOffset++];
}

//------------------------------------------------------------------------------

inline uint16_t DataSymbolReader::get16(uint16_t defaultValue)
    throw(OverrunException)
{
    uint16_t lowByte = get8(static_cast<uint8_t>(defaultValue&0xff));
    uint16_t highByte = get8(static_cast<uint8_t>((defaultValue>>8)&0xff));

    return (highByte<<8) | lowByte;
}

//------------------------------------------------------------------------------

inline DataSymbolReader::operator uint8_t() throw(OverrunException)
{
    return get8(0);
}

//------------------------------------------------------------------------------

inline DataSymbolReader::operator uint16_t() throw(OverrunException)
{
    return get16(0);
}

//------------------------------------------------------------------------------
#endif // DATASYMBOLREADER_H

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
