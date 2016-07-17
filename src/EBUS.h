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

#ifndef EBUS_H
#define EBUS_H
//------------------------------------------------------------------------------

#include "OSError.h"
#include "TimeoutException.h"

//------------------------------------------------------------------------------

/**
 * The main eBUS handler class.
 */
class EBUS
{
private:
    /**
     * The epoll file descriptor.
     */
    int epollFD;

    /**
     * The path of the device.
     */
    std::string devicePath;

    /**
     * The file descriptor of the serial port.
     */
    int portFD;

public:
    /**
     * Construct the eBUS handler for the given device.
     */
    EBUS(const std::string& devicePath) throw(OSError);

    /**
     * Destroy the eBUS handler by closing the device.
     */
    ~EBUS();

    /**
     * Try to open the device and wait if it is not yet available.
     */
    void open() throw(OSError);

    /**
     * Read a byte from the bus. Wait indefinitely if no byte is available
     * immediately.
     */
    uint8_t read() throw(OSError);

    /**
     * Read a byte from the bus with the given timeout.
     *
     * @return true if the byte could be read within the given amount of time.
     */
    bool readMaybe(uint8_t& symbol, unsigned timeout) throw(OSError);

    /**
     * Read a byte from the bus with the given timeout in milliseconds. If the
     * timeout is exceed  a TimeoutException is thrown.
     */
    uint8_t read(unsigned timeout) throw(OSError, TimeoutException);

    /**
     * Write a byte to the bus.
     */
    void write(uint8_t symbol) throw(OSError);

    /**
     * Close the device.
     */
    void close();

private:
    /**
     * Setup the epoll file descriptor.
     */
    void setupEPoll() throw(OSError);

    /**
     * Setup the serial port.
     *
     * @return whether the port could be setup. If false is returned, the file
     * descriptor might still be valid!
     */
    bool setupPort(const std::string& devicePath) throw(OSError);

    /**
     * Close the port and throw an OSError with the given error message and the
     * error code before closing the port.
     */
    void closeOnError(const std::string prefix = "", int errorNumber = -1)
        throw(OSError);
};

//------------------------------------------------------------------------------
// Inline definitions
//------------------------------------------------------------------------------

inline uint8_t EBUS::read(unsigned timeout) throw(OSError, TimeoutException)
{
    uint8_t symbol;
    if (!readMaybe(symbol, timeout)) throw TimeoutException();
    return symbol;
}

//------------------------------------------------------------------------------
#endif // EBUS_H

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
