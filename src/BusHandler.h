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

#ifndef BUSHANDLER_H
#define BUSHANDLER_H
//------------------------------------------------------------------------------

#include "OSError.h"
#include "TimeoutException.h"

#include <inttypes.h>

//------------------------------------------------------------------------------

class EBUS;

//------------------------------------------------------------------------------

// FIXME: perhaps move it to some common header
class SYNException
{
};

//------------------------------------------------------------------------------

/**
 * A wrapper over eBUS with some helper functions to be able to manage the
 * bus.
 */
class BusHandler
{
public:
    /**
     * Type for symbols.
     */
    typedef uint8_t symbol_t;

    /**
     * Timeout for the auto-SYN symbol.
     */
    static const unsigned TIMEOUT_AUTO_SYN = 51;

    /**
     * Symbol: ACK
     */
    static const symbol_t SYMBOL_ACK = 0x00;

    /**
     * Symbol: Escape
     */
    static const symbol_t SYMBOL_ESC = 0xa9;

    /**
     * Symbol: SYN
     */
    static const symbol_t SYMBOL_SYN = 0xaa;

    /**
     * Symbol: broadcast address
     */
    static const symbol_t SYMBOL_BCAST = 0xfe;

    /**
     * Symbol: NACK
     */
    static const symbol_t SYMBOL_NACK = 0xff;

    /**
     * The CRC lookup table.
     */
    static const unsigned char crcLookupTable[256];

    /**
     * The length of the symbol history.
     */
    static const size_t historyLength = 64;

    /**
     * Update the given CRC value with the given symbol.
     */
    static symbol_t updateCRC(symbol_t& crc, symbol_t symbol);

    /**
     * Determine if the given symbol can be a master address.
     */
    static bool isMasterAddress(symbol_t symbol);

    /**
     * Determine if the given symbol is the broadcast address.
     */
    static bool isBroadcastAddress(symbol_t symbol);

    /**
     * Determine if the given symbol can be a slave address.
     */
    static bool isSlaveAddress(symbol_t symbol);

private:
    /**
     * The eBUS interface.
     */
    EBUS& ebus;

    /**
     * The current CRC value.
     */
    symbol_t crc;

    /**
     * Indicate if a SYN symbol has been left pending.
     */
    bool synPending;

    /**
     * History of normal (non-raw) symbols.
     */
    symbol_t history[historyLength];

    /**
     * The offset of the first symbol in the history.
     */
    size_t historyFirstOffset;

    /**
     * The offset of the next symbol in the history. If equal to
     * historyFirstOffset, the history is empty.
     */
    size_t historyNextOffset;

public:
    /**
     * Construct the bus handler.
     */
    BusHandler(EBUS& ebus);

    /**
     * Wait for the signal to appear, i.e. the reception of a SYN message.
     */
    bool waitSignal(unsigned timeout = 0) throw (OSError, TimeoutException);

    /**
     * Reset the CRC value to 0.
     */
    void resetCRC();

    /**
     * Reset the CRC value with the given initial symbol.
     */
    void resetCRC(symbol_t symbol);

    /**
     * Get the current value of the CRC.
     */
    symbol_t getCRC() const;

    /**
     * Read the next raw symbol with the SYN timeout. The symbol is raw,
     * because this function does not handle the conversion of the sequences
     * of 0xa9 and 0x00 and 0x01. The CRC is updated, however. The SYN timeout
     * is used.
     *
     * @return if the symbol could be read within the timeout.
     */
    bool nextRawSymbolMaybe(symbol_t& symbol) throw (OSError);

    /**
     * Read the next raw symbol with the SYN timeout.
     */
    symbol_t nextRawSymbol() throw (OSError, TimeoutException);

    /**
     * Reset the symbol history.
     */
    void resetHistory();

    /**
     * Reset the symbol history and put the given symbol into it.
     */
    void resetHistory(symbol_t symbol);

    /**
     * Get the symbol history. It also flushes the history.
     *
     * @return the number of symbols returned.
     */
    size_t getHistory(symbol_t* symbols);

    /**
     * Read the next symbol with the SYN timeout. It converts sequences of 0xa9
     * and 0x00 or 0x01 into the appropriate symbols. If a SYN symbol arrives,
     * a SYNException is thrown. It is flagged, and next time a raw symbol is
     * requested, the SYN symbol is returned.
     *
     * @return if the symbol could be read within the timeout.
     */
    bool nextSymbolMaybe(symbol_t& symbol) throw (OSError, SYNException);

    /**
     * Read the next symbol (with the SYN timeout). If a SYN symbol arrives,
     * a SYNException is thrown.
     */
    symbol_t nextSymbol() throw (OSError, TimeoutException, SYNException);

    /**
     * Write a symbol to the bus and read it back. CRC will be updated.
     */
    symbol_t writeSymbol(symbol_t symbol) throw (OSError);
};

//------------------------------------------------------------------------------
// Inline definitions
//------------------------------------------------------------------------------

inline BusHandler::symbol_t
BusHandler::updateCRC(symbol_t& crc, symbol_t symbol)
{
    crc = crcLookupTable[crc] ^ symbol;
    return crc;
}

//------------------------------------------------------------------------------

inline bool BusHandler::isMasterAddress(symbol_t symbol)
{
    unsigned char prioClass = symbol&0x0f;
    unsigned char subAddress = (symbol&0xf0)>>4;

    return __builtin_popcount(prioClass+1)==1 &&
        __builtin_popcount(subAddress+1)==1;
}

//------------------------------------------------------------------------------

inline bool BusHandler::isBroadcastAddress(symbol_t symbol)
{
    return symbol==SYMBOL_BCAST;
}

//------------------------------------------------------------------------------

inline bool BusHandler::isSlaveAddress(symbol_t symbol)
{
    return !isBroadcastAddress(symbol) && !isMasterAddress(symbol);
}


//------------------------------------------------------------------------------

inline BusHandler::BusHandler(EBUS& ebus) :
    ebus(ebus),
    crc(0),
    synPending(false),
    historyFirstOffset(0),
    historyNextOffset(0)
{
}

//------------------------------------------------------------------------------

inline void BusHandler::resetCRC()
{
    crc = 0;
}

//------------------------------------------------------------------------------

inline void BusHandler::resetCRC(symbol_t symbol)
{
    resetCRC();
    updateCRC(crc, symbol);
}

//------------------------------------------------------------------------------

inline BusHandler::symbol_t BusHandler::getCRC() const
{
    return crc;
}

//------------------------------------------------------------------------------

inline BusHandler::symbol_t BusHandler::nextRawSymbol()
    throw (OSError, TimeoutException)
{
    symbol_t symbol;
    if (!nextRawSymbolMaybe(symbol)) throw TimeoutException();
    return symbol;
}

//------------------------------------------------------------------------------

inline void BusHandler::resetHistory()
{
    historyNextOffset = historyFirstOffset = 0;
}

//------------------------------------------------------------------------------

inline void BusHandler::resetHistory(symbol_t symbol)
{
    history[0] = symbol;
    historyFirstOffset = 0;
    historyNextOffset = 1;
}

//------------------------------------------------------------------------------

inline BusHandler::symbol_t BusHandler::nextSymbol()
    throw (OSError, TimeoutException, SYNException)
{
    symbol_t symbol;
    if (!nextSymbolMaybe(symbol)) throw TimeoutException();
    return symbol;
}

//------------------------------------------------------------------------------
#endif // BUSHANDLER_H

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
