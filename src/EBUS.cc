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

#include "EBUS.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

//------------------------------------------------------------------------------

using std::string;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

EBUS::EBUS(const string& devicePath) throw (OSError) :
    epollFD(-1),
    devicePath(devicePath),
    portFD(-1)
{
    setupEPoll();
}

//------------------------------------------------------------------------------

EBUS::~EBUS()
{
    close();
    ::close(epollFD);
}

//------------------------------------------------------------------------------

void EBUS::open() throw(OSError)
{
    while(portFD<0) {
        try {
            setupPort(devicePath);
        } catch(const OSError& e) {
            printf("EBUS::open: failed to open port, waiting: %s\n",
                   e.what());
            sleep(1);
        }
    }
}

//------------------------------------------------------------------------------

uint8_t EBUS::read() throw(OSError)
{
    uint8_t symbol;
    if (::read(portFD, &symbol, 1)<0) {
        closeOnError("EBUS::read: read");
    }
    return symbol;
}

//------------------------------------------------------------------------------

bool EBUS::readMaybe(uint8_t& symbol, unsigned timeout) throw(OSError)
{
    struct epoll_event event;
    int numDesriptors = epoll_wait(epollFD, &event, 1, timeout);
    if (numDesriptors<0) {
        throw OSError("EBUS::readMaybe: epoll_wait");
    } else if (numDesriptors==0) {
        return false;
    } else {
        if ((event.events&(EPOLLHUP|EPOLLERR))!=0) {
            closeOnError("EBUS::readMaybe: EPOLLHUP or EPOLLERR occured");
        } else if ((event.events&EPOLLIN)!=0) {
            if (::read(portFD, &symbol, 1)<0) {
                closeOnError("EBUS::read: read");
            }
            return true;
        } else {
            closeOnError("EBUS::readMaybe: no EPOLLIN event");
        }
    }
    return false;
}

//------------------------------------------------------------------------------

void EBUS::write(uint8_t symbol) throw(OSError)
{
    if (::write(portFD, &symbol, sizeof(symbol))<0) {
        closeOnError("EBUS::writeMaybe: write");
    }
}

//------------------------------------------------------------------------------

void EBUS::close()
{
    ::close(portFD); portFD = -1;
}

//------------------------------------------------------------------------------

void EBUS::setupEPoll() throw(OSError)
{
    epollFD = epoll_create1(0);
    if (epollFD<0) {
        throw OSError("EBUS::setupEPoll: epoll_create1");
    }
}

//------------------------------------------------------------------------------

bool EBUS::setupPort(const std::string& devicePath) throw(OSError)
{
    portFD = ::open(devicePath.c_str(), O_RDWR | O_NOCTTY);
    if (portFD<0) {
        throw OSError("EBUS::setupPort: open");
    }

    // empty device buffer
    if (tcflush(portFD, TCIFLUSH)<0) {
        closeOnError("EBUS::setupPort: tcflush");
    }

    struct termios settings;
    memset(&settings, 0, sizeof(settings));

    settings.c_cflag |= (B2400 | CS8 | CLOCAL | CREAD);
    settings.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    settings.c_iflag |= IGNPAR;
    settings.c_oflag &= ~OPOST;

    // non-canonical mode: read() blocks until at least one byte is available
    settings.c_cc[VMIN]  = 1;
    settings.c_cc[VTIME] = 0;

    if (tcsetattr(portFD, TCSAFLUSH, &settings)<0) {
        closeOnError("EBUS::setupPort: tcsetattr");
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = portFD;

    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, portFD, &event)<0) {
        closeOnError("EBUS::setupPort: epoll_ctl");
    }

    return true;
}

//------------------------------------------------------------------------------

void EBUS::closeOnError(const std::string prefix, int errorNumber)
    throw(OSError)
{
    OSError osError(prefix, errorNumber);
    close();
    throw osError;
}

//------------------------------------------------------------------------------

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
