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

#ifndef TELEGRAM_H
#define TELEGRAM_H
//------------------------------------------------------------------------------

#include "BusHandler.h"

#include "util.h"

#include <cassert>

//------------------------------------------------------------------------------

/**
 * A telegram. It can be used to represent all three kinds of telegrams
 * (master-master, master-slave, broadcast).
 */
class Telegram
{
public:
    /**
     * Type for the acknowledgement status.
     */
    typedef enum {
        // ACK received
        ACK,

        // NACK received
        NACK,

        // Neither ACK nor NACK is received
        NONE
    } acknowledgement_t;

    /**
     * Get the acknowledgement type for the given symbol.
     */
    static acknowledgement_t symbol2ack(symbol_t symbol);

public:
    /**
     * The source address.
     */
    symbol_t source;

    /**
     * The destination address.
     */
    symbol_t destination;

    /**
     * The primary command
     */
    symbol_t primaryCommand;

    /**
     * The secondary command
     */
    symbol_t secondaryCommand;

    /**
     * The number of the data symbols.
     */
    size_t numDataSymbols;

    /**
     * The data symbols.
     */
    symbol_t* dataSymbols;

    /**
     * Indicate if the CRC was OK.
     */
    bool crcOK;

    /**
     * The acknowledgement status. This is not relevant for broadcast telegrams.
     */
    acknowledgement_t acknowledgement;

    /**
     * The length of the reply data symbols for master-slave telegrams.
     */
    size_t numReplyDataSymbols;

    /**
     * The data symbols of the reply for master-slave telegrams.
     */
    symbol_t* replyDataSymbols;

    /**
     * Indicate if the CRC was OK in the reply of a master-slave telegram.
     */
    bool replyCRCOK;

    /**
     * Indicate the akcnowledgement status by the source (i.e. the master) in
     * case of a master-slave telegram.
     */
    acknowledgement_t masterAcknowledgement;

public:
    /**
     * Construct the telegram with the given basic data.
     *
     * A buffer for the data symbols will be allocated too.
     */
    Telegram(symbol_t source, symbol_t destination,
             symbol_t primaryCommand, symbol_t secondaryCommand,
             size_t numDataSymbols);

    /**
     * Move construct the telegram
     */
    Telegram(Telegram&& other);

    /**
     * The copy constructor is deleted.
     */
    Telegram(const Telegram&) = delete;

    /**
     * Destroy the telegram.
     */
    ~Telegram();

    /**
     * Start a reply by allocating a buffer of the given number of symbols for
     * the reply.
     */
    void startReply(size_t numDataSymbols);
};

//------------------------------------------------------------------------------
// Inline definitions
//------------------------------------------------------------------------------

inline Telegram::acknowledgement_t Telegram::symbol2ack(symbol_t symbol)
{
    return (symbol==BusHandler::SYMBOL_ACK) ? ACK :
        ((symbol==BusHandler::SYMBOL_NACK) ? NACK : NONE);
}

//------------------------------------------------------------------------------

inline Telegram::Telegram(symbol_t source, symbol_t destination,
                          symbol_t primaryCommand, symbol_t secondaryCommand,
                          size_t numDataSymbols) :
    source(source),
    destination(destination),
    primaryCommand(primaryCommand),
    secondaryCommand(secondaryCommand),
    numDataSymbols(numDataSymbols),
    dataSymbols(new symbol_t[numDataSymbols]),
    crcOK(false),
    acknowledgement(NONE),
    numReplyDataSymbols(0),
    replyDataSymbols(0),
    replyCRCOK(false),
    masterAcknowledgement(NONE)
{
}

//------------------------------------------------------------------------------

inline Telegram::Telegram(Telegram&& other) :
    source(other.source),
    destination(other.destination),
    primaryCommand(other.primaryCommand),
    secondaryCommand(other.secondaryCommand),
    numDataSymbols(other.numDataSymbols),
    dataSymbols(other.dataSymbols),
    crcOK(other.crcOK),
    acknowledgement(other.acknowledgement),
    numReplyDataSymbols(other.numReplyDataSymbols),
    replyDataSymbols(other.replyDataSymbols),
    replyCRCOK(other.replyCRCOK),
    masterAcknowledgement(other.masterAcknowledgement)
{
    other.dataSymbols = 0;
    other.replyDataSymbols = 0;
}

//------------------------------------------------------------------------------

inline Telegram::~Telegram()
{
    delete[] replyDataSymbols;
    delete[] dataSymbols;
}

//------------------------------------------------------------------------------

inline void Telegram::startReply(size_t numDataSymbols)
{
    delete [] replyDataSymbols;
    numReplyDataSymbols = numDataSymbols;
    replyDataSymbols = new symbol_t[numDataSymbols];
}

//------------------------------------------------------------------------------
#endif // TELEGRAM_H

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
