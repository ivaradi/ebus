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

#include "MessageHandler.h"

#include "BusHandler.h"
#include "Telegram.h"

//------------------------------------------------------------------------------

void MessageHandler::run() throw (OSError)
{
    BusHandler::symbol_t history[BusHandler::historyLength];

    bool synPending = false;
    while(true) {
        if (!synPending) {
            printf("Waiting for signal...\n");
            while(!busHandler.waitSignal(1000)) {
                printf("Still no signal...\n");
            }
            printf("Signal detected\n");
        }

        bool dumpData = false;
        try {
            synPending = false;
            while (true) {
                auto source = BusHandler::SYMBOL_SYN;
                while (source == BusHandler::SYMBOL_SYN) {
                    if (!busHandler.nextRawSymbolMaybe(source)) {
                        printf("Timeout waiting for a message...\n");
                    }
                }

                if (!BusHandler::isMasterAddress(source)) {
                    printf("First byte after SYN is not master address!\n");
                    continue;
                }

                readTelegram(source);
            }

        } catch(const TimeoutException&) {
            printf("Timeout occurred\n");
            dumpData = true;
        } catch(const SYNException&) {
            printf("Unexpected SYN symbol!\n");
            synPending = true;
            dumpData = true;
        }

        if (dumpData) {
            size_t historySize = busHandler.getHistory(history);
            if (historySize>0) {
                printf("  the bytes received so far:");
                for(size_t i = 0; i<historySize; ++i) {
                    printf(" %02x", history[i]);
                }
                printf("\n");
            }
        }
    }
}

//------------------------------------------------------------------------------

void MessageHandler::received(const Telegram& /*telegram*/)
{
}

//------------------------------------------------------------------------------

void MessageHandler::readTelegram(symbol_t source)
    throw(SYNException, TimeoutException, OSError)
{
    busHandler.resetCRC(source);
    busHandler.resetHistory(source);

    auto destination = busHandler.nextSymbol();

    auto primaryCommand = busHandler.nextSymbol();
    auto secondaryCommand = busHandler.nextSymbol();

    auto numDataSymbols = busHandler.nextSymbol();

    Telegram telegram(source, destination,
                      primaryCommand, secondaryCommand,
                      numDataSymbols);

    for(unsigned i = 0; i<numDataSymbols; ++i) {
        telegram.dataSymbols[i] = busHandler.nextSymbol();
    }

    auto crc = busHandler.getCRC();

    unsigned char sentCRC = busHandler.nextSymbol();

    telegram.crcOK = crc==sentCRC;

    BusHandler::symbol_t ack = BusHandler::SYMBOL_NACK;
    if (!BusHandler::isBroadcastAddress(destination)) {
        try {
            ack = busHandler.nextSymbol();
        } catch(const SYNException&) {
            fprintf(stderr, "Missing ACK symbol, SYN received instead\n");
            received(telegram);
            throw;
        }
        if (ack!=BusHandler::SYMBOL_ACK) {
            fprintf(stderr, "No ACK at the end of the message: %02x\n", ack);
        }
        telegram.acknowledgement = Telegram::symbol2ack(ack);
    }

    if (BusHandler::isSlaveAddress(destination) &&
        ack==BusHandler::SYMBOL_ACK)
    {
        readReply(telegram);
    }

    received(telegram);
}

//------------------------------------------------------------------------------

void MessageHandler::readReply(Telegram& telegram)
    throw(SYNException, TimeoutException, OSError)
{
    busHandler.resetCRC();

    auto numReplySymbols = busHandler.nextSymbol();
    telegram.startReply(numReplySymbols);
    for(unsigned i = 0; i<numReplySymbols; ++i) {
        telegram.replyDataSymbols[i] = busHandler.nextSymbol();
    }

    auto replyCRC = busHandler.getCRC();
    auto sentReplyCRC = busHandler.nextSymbol();
    telegram.replyCRCOK = replyCRC == sentReplyCRC;

    auto replyACK = busHandler.nextSymbol();
    if (replyACK!=BusHandler::SYMBOL_ACK) {
        fprintf(stderr, "Not ACK at the end of the slave reply: %02x\n", replyACK);
    }
    telegram.masterAcknowledgement = Telegram::symbol2ack(replyACK);
}

//------------------------------------------------------------------------------

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
