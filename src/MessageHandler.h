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

#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H
//------------------------------------------------------------------------------

// FIXME: do not include BusHandler once SYNException is moved out
#include "BusHandler.h"
#include "OSError.h"
#include "util.h"

#include <deque>

//------------------------------------------------------------------------------

class Telegram;

//------------------------------------------------------------------------------

/**
 * The class handling the reception and the sending of messages.
 */
class MessageHandler
{
private:
    /**
     * The bus handler we work with.
     */
    BusHandler& busHandler;

    /**
     * The queue of telegrams to send.
     */
    std::deque<Telegram*> sendQueue;

public:
    /**
     * Construct the message handler for the given bus handler.
     */
    MessageHandler(BusHandler& busHandler);

    /**
     * Handle the messages.
     */
    void run() throw (OSError);

    /**
     * Send the given telegram. It will be enqueued, and attempted to be sent.
     */
    void send(Telegram* telegram);

protected:
    /**
     * Called when a telegram was received.
     */
    virtual void received(const Telegram& telegram);

    /**
     * Called when the signal status changes.
     */
    virtual void signalChanged(bool hasSignal);

private:
    /**
     * Read a telegram from the given source.
     */
    void readTelegram(symbol_t source)
        throw(SYNException, TimeoutException, OSError);

    /**
     * Read the slave reply.
     */
    void readReply(Telegram& telegram)
        throw(SYNException, TimeoutException, OSError);

    /**
     * Read the slave reply and the corresponding master acknowledgement.
     */
    void readReplyAndACK(Telegram& telegram)
        throw(SYNException, TimeoutException, OSError);

    /**
     * Try to send the first telegram in the queue.
     *
     * @return 0 on success, otherwise the number of SYN symbols to wait for
     * trying to send again.
     */
    unsigned trySend() throw(SYNException, TimeoutException, OSError);
};

//------------------------------------------------------------------------------
// Inline definitions
//------------------------------------------------------------------------------

inline MessageHandler::MessageHandler(BusHandler& busHandler) :
    busHandler(busHandler)
{
}

//------------------------------------------------------------------------------
#endif // MESSAGEHANDLER_H

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
