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
#include "Log.h"

#include <cstring>

//------------------------------------------------------------------------------

void MessageHandler::run() throw (OSError)
{
    BusHandler::symbol_t history[BusHandler::historyLength];

    bool synPending = false;
    while(true) {
        if (!synPending) {
            signalChanged(false);
            Log::info("Waiting for signal...");
            while(!busHandler.waitSignal(1000)) {
                Log::info("Still no signal...");
            }
            Log::info("Signal detected");
            signalChanged(true);
        }

        bool dumpData = false;
        unsigned waitSYNBeforeSend = 0;
        unsigned long long lastSYNTime = 0;
        try {
            synPending = false;
            while (true) {
                auto source = BusHandler::SYMBOL_SYN;
                while (source == BusHandler::SYMBOL_SYN) {
                    lastSYNTime = currentTimeMillis();
                    if (!busHandler.nextRawSymbolMaybe(source)) {
                        Log::info("Timeout waiting for a message...");
                    }
                    if (waitSYNBeforeSend>0) --waitSYNBeforeSend;
                    if (waitSYNBeforeSend==0 &&
                        source==BusHandler::SYMBOL_SYN && !sendQueue.empty())
                    {
                        waitSYNBeforeSend = trySend();
                    }
                }

                if (!BusHandler::isMasterAddress(source)) {
                    Log::info("The first byte after SYN is not master address: 0x%02x, delay: %llu ms!",
                              source, currentTimeMillis() - lastSYNTime);
                    continue;
                }

                readTelegram(source);
            }

        } catch(const TimeoutException&) {
            Log::info("Timeout occurred");
            dumpData = true;
        } catch(const SYNException&) {
            Log::info("Unexpected SYN symbol!");
            synPending = true;
            dumpData = true;
        }

        if (dumpData) {
            size_t historySize = busHandler.getHistory(history);
            if (historySize>0) {
                char buffer[3*historySize + 32];
                strcpy(buffer, "  the bytes received so far:");
                size_t bufferLength = strlen(buffer);

                for(size_t i = 0; i<historySize; ++i) {
                    bufferLength += snprintf(buffer + bufferLength,
                                             sizeof(buffer) - bufferLength,
                                             " %02x", history[i]);
                }

                Log::info("%s", buffer);
            }
        }
    }
}

//------------------------------------------------------------------------------

void MessageHandler::send(Telegram* telegram)
{
    sendQueue.push_back(telegram);
}

//------------------------------------------------------------------------------

void MessageHandler::received(const Telegram& /*telegram*/)
{
}

//------------------------------------------------------------------------------

void MessageHandler::signalChanged(bool /*hasSignal*/)
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
            Log::error("Missing ACK symbol, SYN received instead");
            received(telegram);
            throw;
        }
        if (ack!=BusHandler::SYMBOL_ACK) {
            Log::error("No ACK at the end of the message: %02x", ack);
        }
        telegram.acknowledgement = Telegram::symbol2ack(ack);
    }

    if (BusHandler::isSlaveAddress(destination) &&
        ack==BusHandler::SYMBOL_ACK)
    {
        readReplyAndACK(telegram);
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
}

//------------------------------------------------------------------------------

void MessageHandler::readReplyAndACK(Telegram& telegram)
    throw(SYNException, TimeoutException, OSError)
{
    readReply(telegram);

    auto replyACK = busHandler.nextSymbol();
    if (replyACK!=BusHandler::SYMBOL_ACK) {
        Log::error("No ACK at the end of the slave reply: %02x", replyACK);
    }
    telegram.masterAcknowledgement = Telegram::symbol2ack(replyACK);
}

//------------------------------------------------------------------------------

unsigned MessageHandler::trySend()
    throw(SYNException, TimeoutException, OSError)
{
    auto telegram = sendQueue.front();

    busHandler.resetCRC();
    auto symbol = busHandler.writeSymbol(telegram->source);
    if (symbol==telegram->source) {
        busHandler.writeSymbol(telegram->destination);
        busHandler.writeSymbol(telegram->primaryCommand);
        busHandler.writeSymbol(telegram->secondaryCommand);
        busHandler.writeSymbol(telegram->numDataSymbols);
        for(size_t i = 0; i<telegram->numDataSymbols; ++i) {
            busHandler.writeSymbol(telegram->dataSymbols[i]);
        }
        busHandler.writeSymbol(busHandler.getCRC());
        telegram->crcOK = true;

        bool isOK = true;
        if (!BusHandler::isBroadcastAddress(telegram->destination)) {
            auto ack = busHandler.nextSymbol();
            if (ack!=BusHandler::SYMBOL_ACK) {
                Log::error("No ACK at received from destination: %02x", ack);
            }
            telegram->acknowledgement = Telegram::symbol2ack(ack);
            if (telegram->acknowledgement==Telegram::ACK) {
                if (BusHandler::isSlaveAddress(telegram->destination)) {
                    readReply(*telegram);
                    isOK = telegram->replyCRCOK;

                    auto replyACK = isOK ?
                        BusHandler::SYMBOL_ACK : BusHandler::SYMBOL_NACK;
                    busHandler.writeSymbol(replyACK);

                    telegram->masterAcknowledgement =
                        Telegram::symbol2ack(replyACK);
                }
            } else {
                isOK = false;
            }
        }
        if (isOK) {
            received(*telegram);
            sendQueue.pop_front();
            delete telegram;
        }
        return 0;
    } else {
        return ((symbol&0x0f)==(telegram->source&0x0f)) ? 1 : 2;
    }
}

//------------------------------------------------------------------------------

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
