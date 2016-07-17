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
#include "BusHandler.h"
#include "MessageHandler.h"
#include "Telegram.h"
#include "DataSymbolReader.h"
#include "Data.h"
#include "Log.h"

#include <fstream>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <csignal>

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

//------------------------------------------------------------------------------

using std::exception;
using std::string;
using std::ofstream;
using std::endl;

//------------------------------------------------------------------------------

/**
 * The data to be written into the JSON file.
 */
class WebData
{
public:
    /**
     * The actual error code.
     */
    int errorCode = 0;

    /**
     * The room temperature.
     */
    double roomTemp = -10.0;

    /**
     * The outside temperature.
     */
    double outsideTemp = -20.0;

    /**
     * The average outside temperature.
     */
    double outsideTempAvg = -20.0;

    /**
     * The temperature of the service water.
     */
    double serviceWaterTemp = 50.0;

    /**
     * The target temperature of the service water.
     */
    double serviceWaterTargetTemp = 50.0;

    /**
     * The boiler temperature.
     */
    double boilerTemp = 5.0;

    /**
     * The return water temperature.
     */
    double returnWaterTemp = 5.0;

    /**
     * The boiler target temperature.
     */
    double boilerTargetTemp = 5.0;

    /**
     * The last timestamp received.
     */
    std::string lastTime = "1970-01-01 00:00:00";

private:
    /**
     * The file into which the data should be written.
     */
    std::string targetPath;

public:
    /**
     * Construct the data with the given target path.
     */
    WebData(const std::string& targetPath);

    /**
     * Write the data.
     */
    void write();
};

//------------------------------------------------------------------------------

inline WebData::WebData(const std::string& targetPath) :
    targetPath(targetPath)
{
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Our message handler.
 */
class MainMessageHandler : public MessageHandler
{
private:
    /**
     * Convert the given address into a string. For known addresses it will be
     * some abbreviation, for unknown ones it will be the hex digits.
     */
    static std::string address2String(symbol_t symbol);

    /**
     * Convert the given bitfield into a string.
     */
    static std::string bit2String(BitData bitData,
                                  const std::string& bit0Str = "BIT_0",
                                  const std::string& bit1Str = "BIT_1",
                                  const std::string& bit2Str = "BIT_2",
                                  const std::string& bit3Str = "BIT_3",
                                  const std::string& bit4Str = "BIT_4",
                                  const std::string& bit5Str = "BIT_5",
                                  const std::string& bit6Str = "BIT_6",
                                  const std::string& bit7Str = "BIT_7");

private:
    /**
     * The data to send to the website.
     */
    WebData webData;

public:
    /**
     * Construct the message handler.
     */
    MainMessageHandler(BusHandler& busHandler, const std::string& jsonFilePath);

private:
    /**
     * Process the received telegram.
     */
    virtual void received(const Telegram& telegram);

    /**
     * Dump a telegram we don't decode.
     */
    void dumpTelegram(const Telegram& telegram);

    /**
     * Process the command 0503
     */
    void process0503(const Telegram& telegram) throw(OverrunException);

    /**
     * Process the command 0507
     */
    void process0507(const Telegram& telegram) throw(OverrunException);

    /**
     * Process the command 0700
     */
    void process0700(const Telegram& telegram) throw(OverrunException);

    /**
     * Process the command 0800
     */
    void process0800(const Telegram& telegram) throw(OverrunException);

    /**
     * Process the command 5014
     */
    void process5014(const Telegram& telegram) throw(OverrunException);
};

//------------------------------------------------------------------------------

inline MainMessageHandler::MainMessageHandler(BusHandler& busHandler,
                                              const std::string& jsonFilePath) :
    MessageHandler(busHandler),
    webData(jsonFilePath)
{
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void WebData::write()
{
    std::string tmpPath = targetPath + ".tmp";
    try {
        ofstream outFile(tmpPath);
        outFile << "{" << endl;
        outFile << "    \"errorCode\":" << errorCode << "," << endl;
        outFile << "    \"roomTemp\":" << roomTemp << "," << endl;
        outFile << "    \"outsideTemp\":" << outsideTemp << "," << endl;
        outFile << "    \"outsideTempAvg\":" << outsideTempAvg << "," << endl;
        outFile << "    \"serviceWaterTemp\":" << serviceWaterTemp << "," << endl;
        outFile << "    \"serviceWaterTargetTemp\":" << serviceWaterTargetTemp << "," << endl;
        outFile << "    \"boilerTemp\":" << boilerTemp << "," << endl;
        outFile << "    \"returnWaterTemp\":" << returnWaterTemp << "," << endl;
        outFile << "    \"boilerTargetTemp\":" << boilerTargetTemp << "," << endl;
        outFile << "    \"lastTime\": \"" << lastTime << "\"" << endl;
        outFile << "}" << endl;
        outFile.close();

        ::rename(tmpPath.c_str(), targetPath.c_str());
    } catch(const exception& e) {
        Log::error("Failed to write web data: %s", e.what());
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

template <typename T, typename U>
void updateValue(bool& modified, T& value, const U& newValue)
{
    const T nv = static_cast<T>(newValue);

    if (nv==value) return;

    value = nv;
    modified = true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

string MainMessageHandler::address2String(symbol_t symbol)
{
    char buffer[4];
    snprintf(buffer, sizeof(buffer), "%02x", symbol);
    return buffer;
}

//------------------------------------------------------------------------------

void MainMessageHandler::received(const Telegram& telegram)
{
    try {
        if (telegram.primaryCommand==0x05 &&
            telegram.secondaryCommand==0x03)
        {
            process0503(telegram);
        } else if (telegram.primaryCommand==0x05 &&
            telegram.secondaryCommand==0x07)
        {
            process0507(telegram);
        } else if (telegram.primaryCommand==0x07 &&
                   telegram.secondaryCommand==0x00)
        {
            process0700(telegram);
        } else if (telegram.primaryCommand==0x08 &&
                   telegram.secondaryCommand==0x00)
        {
            process0800(telegram);
        } else if (telegram.primaryCommand==0x50 &&
                   telegram.secondaryCommand==0x14)
        {
            process5014(telegram);
        } else {
            dumpTelegram(telegram);
        }
    } catch(const OverrunException&) {
        Log::error("!!! Overrun while processing telegram:");
        dumpTelegram(telegram);
    }
}

//------------------------------------------------------------------------------

string MainMessageHandler::bit2String(BitData bitData,
                                      const string& bit0Str,
                                      const string& bit1Str,
                                      const string& bit2Str,
                                      const string& bit3Str,
                                      const string& bit4Str,
                                      const string& bit5Str,
                                      const string& bit6Str,
                                      const string& bit7Str)
{
    const string* bitStrs[8] = {
        &bit0Str,
        &bit1Str,
        &bit2Str,
        &bit3Str,
        &bit4Str,
        &bit5Str,
        &bit6Str,
        &bit7Str,
    };

    string bitsStr;
    uint8_t bits(bitData);

    for(unsigned i = 0; i<8; ++i) {
        uint8_t mask = 1<<i;
        if ( (bits&mask)==mask ) {
            bitsStr.append(" ");
            bitsStr.append(*bitStrs[i]);
        }
    }

    return bitsStr;
}

//------------------------------------------------------------------------------

void MainMessageHandler::dumpTelegram(const Telegram& telegram)
{
    // FIXME: either make logging more intelligent or provide some better way
    // to produce such a buffer.
    char buffer[4096];
    size_t bufferLength = 0;

    bufferLength +=
        snprintf(buffer + bufferLength, sizeof(buffer) - bufferLength,
                 "Telegram: %s->%s %02x%02x [",
                 address2String(telegram.source).c_str(),
                 address2String(telegram.destination).c_str(),
                 telegram.primaryCommand, telegram.secondaryCommand);
    for(unsigned i = 0; i<telegram.numDataSymbols; ++i) {
        if (i>0) bufferLength += snprintf(buffer + bufferLength,
                                          sizeof(buffer) - bufferLength,
                                          " ");
        bufferLength += snprintf(buffer + bufferLength,
                                 sizeof(buffer) - bufferLength,
                                 "%02x", telegram.dataSymbols[i]);
    }
    bufferLength += snprintf(buffer + bufferLength,
                             sizeof(buffer) - bufferLength,
                             "]");
    if (!telegram.crcOK) {
        bufferLength += snprintf(buffer + bufferLength,
                                 sizeof(buffer) - bufferLength,
                                 " (CRC mismatch)");
    }
    if (!BusHandler::isBroadcastAddress(telegram.destination) &&
        telegram.acknowledgement!=Telegram::ACK)
    {
        if (telegram.acknowledgement==Telegram::NONE) {
            bufferLength += snprintf(buffer + bufferLength,
                                     sizeof(buffer) - bufferLength,
                                     " (no acknowledgement)");
        } else {
            bufferLength += snprintf(buffer + bufferLength,
                                     sizeof(buffer) - bufferLength,
                                     " (negative acknowledgement)");
        }
    }
    Log::info("%s", buffer);

    if (BusHandler::isSlaveAddress(telegram.destination) &&
        telegram.acknowledgement==Telegram::ACK)
    {
        bufferLength = 0;
        bufferLength += snprintf(buffer + bufferLength,
                                 sizeof(buffer) - bufferLength,
                                 "  ==> [");
        for(unsigned i = 0; i<telegram.numReplyDataSymbols; ++i) {
            if (i>0) bufferLength += snprintf(buffer + bufferLength,
                                              sizeof(buffer) - bufferLength,
                                              " ");
            bufferLength += snprintf(buffer + bufferLength,
                                     sizeof(buffer) - bufferLength,
                                     "%02x", telegram.replyDataSymbols[i]);
        }
        bufferLength += snprintf(buffer + bufferLength,
                                 sizeof(buffer) - bufferLength,
                                 "]");
        if (!telegram.replyCRCOK) {
            bufferLength += snprintf(buffer + bufferLength,
                                     sizeof(buffer) - bufferLength,
                                     " (CRC mismatch)");
        }
        if (telegram.acknowledgement==Telegram::NONE) {
            bufferLength += snprintf(buffer + bufferLength,
                                     sizeof(buffer) - bufferLength,
                                     " (no acknowledgement)");
        } else if (telegram.acknowledgement==Telegram::NACK) {
            bufferLength += snprintf(buffer + bufferLength,
                                     sizeof(buffer) - bufferLength,
                                     " (negative acknowledgement)");
        }
        Log::info("%s", buffer);
    }
}

//------------------------------------------------------------------------------

void MainMessageHandler::process0503(const Telegram& telegram)
    throw(OverrunException)
{
    DataSymbolReader reader(telegram);

    ByteData blockNumber(reader);
    if (blockNumber==0x01) {
        BitData status(reader);
        BitData burnerControlState(reader);
        CharData minMaxBoilerPerf(reader);
        Data1c boilerTemp(reader);
        CharData returnWaterTemp(reader);
        CharData boilerTemp2(reader);
        SignedCharData outsideTemp(reader);

        uint8_t statusValue(status);
        char statusStr[16];
        if ((statusValue&0x80)==0x80) {
            snprintf(statusStr, sizeof(statusStr), "ERROR %u",
                     statusValue&0x7f);
        } else {
            snprintf(statusStr, sizeof(statusStr), "OK");
        }

        string burnerControlStateStr =
            bit2String(burnerControlState, "LDW", "GDW", "WS",
                       "Flame", "Valve1", "Valve2", "UWP", "Alarm");

        Log::info("%s->%s BC Op. Data block 01: %s, state: %s, min-max boiler perf: %u%%, boiler temp: %.2f°C, return water temp: %u°C, boiler temp2: %u°C, outside temp: %d°C",
                  address2String(telegram.source).c_str(),
                  address2String(telegram.destination).c_str(),
                  statusStr, burnerControlStateStr.c_str(),
                  minMaxBoilerPerf.get(),
                  boilerTemp.get(),
                  returnWaterTemp.get(),
                  boilerTemp2.get(), outsideTemp.get());

        if (telegram.source==0x03 &&
            BusHandler::isBroadcastAddress(telegram.destination))
        {
            bool modified = false;

            updateValue(modified, webData.errorCode, status);
            updateValue(modified, webData.boilerTemp, boilerTemp);
            updateValue(modified, webData.returnWaterTemp,  returnWaterTemp);
            updateValue(modified, webData.serviceWaterTemp, boilerTemp2);
            updateValue(modified, webData.outsideTemp, outsideTemp);

            if (modified) webData.write();
        }

    } else if (blockNumber==0x02) {
        Data2c exhaustTemp(reader);
        Data1c bwwLeadWaterTemp(reader);
        Data1c effBoilerPerf(reader);
        Data1c jointLeadWaterTemp(reader);
        ByteData _available(reader);

        Log::info("%s->%s BC Op. Data block 02: exhaust temp: %.2f°C, BWW lead water temp: %.1f°C, eff. boiler perf: %.1f%%, joint lead water temp: %.1f°C",
                  address2String(telegram.source).c_str(),
                  address2String(telegram.destination).c_str(),
                  exhaustTemp.get(),
                  bwwLeadWaterTemp.get(), effBoilerPerf.get(),
                  jointLeadWaterTemp.get());
    } else {
        dumpTelegram(telegram);
    }
}

//------------------------------------------------------------------------------

void MainMessageHandler::process0507(const Telegram& telegram)
    throw(OverrunException)
{
    DataSymbolReader reader(telegram);

    ByteData heatRequest(reader);
    ByteData action(reader);
    Data2c boilerTargetTemp(reader);
    Data2b boilerTargetPressure(reader);
    Data1c settingDegree(reader);
    Data1c serviceWaterTargetTemp(reader);
    ByteData fuelType(reader);

    string heatRequestStr;
    if (heatRequest==0x00) {
        heatRequestStr = "shut down burner";
    } else if (heatRequest==0x01) {
        heatRequestStr = "no action";
    } else if (heatRequest==0x55) {
        heatRequestStr = "prepare service water";
    } else if (heatRequest==0xaa) {
        heatRequestStr = "heating operation";
    } else if (heatRequest==0xcc) {
        heatRequestStr = "emission check";
    } else if (heatRequest==0xdd) {
        heatRequestStr = "tech check";
    } else if (heatRequest==0xee) {
        heatRequestStr = "stop controller";
    } else {
        char str[32];
        snprintf(str, sizeof(str), "request #%02x", heatRequest.get());
        heatRequestStr = str;
    }

    string actionStr;
    if (action==0x00) {
        actionStr = "no action";
    } else if (action==0x01) {
        actionStr = "turn off boiler pump";
    } else if (action==0x02) {
        actionStr = "turn on boiler pump";
    } else if (action==0x03) {
        actionStr = "turn off variable user";
    } else if (action==0x04) {
        actionStr = "turn on variable user";
    } else {
        char str[32];
        snprintf(str, sizeof(str), "action #%02x", action.get());
        actionStr = str;
    }

    string fuelTypeStr;
    if ((fuelType&0x03)==0x01) {
        fuelTypeStr = " (gas)";
    } else if ((fuelType&0x03)==0x02) {
        fuelTypeStr = " (oil)";
    }

    Log::info("%s->%s RC to BC Oper. Data: heatRequest: %s, action: %s, boiler target temp: %.2f°C, boiler target pressure: %.2fbar, setting degree: %.1f%%, service water target temp: %.2f°C, fuel type: 0x%02x%s",
              address2String(telegram.source).c_str(),
              address2String(telegram.destination).c_str(),
              heatRequestStr.c_str(),
              actionStr.c_str(),
              boilerTargetTemp.get(),
              boilerTargetPressure.get(),
              settingDegree.get(),
              serviceWaterTargetTemp.get(),
              fuelType.get(), fuelTypeStr.c_str());
}

//------------------------------------------------------------------------------

void MainMessageHandler::process0700(const Telegram& telegram)
    throw(OverrunException)
{
    static const char* weekDayNames[7] = {
        "Mon",
        "Tue",
        "Wed",
        "Thu",
        "Fri",
        "Sat",
        "Sun",
    };

    DataSymbolReader reader(telegram);

    Data2b outsideTemp(reader);
    BCDData seconds(reader);
    BCDData minutes(reader);
    BCDData hours(reader);
    BCDData day(reader);
    BCDData month(reader);
    BCDData weekday(reader);
    BCDData year(reader);

    string weekDayStr;
    unsigned weekDayIndex = weekday;
    if (weekDayIndex>=1 && weekDayIndex<7) {
        weekDayStr = weekDayNames[weekDayIndex-1];
    } else {
        char str[4];
        snprintf(str, sizeof(str), "%02x", weekDayIndex);
        weekDayStr = str;
    }

    Log::info("%s->%s Date/Time: outside temp: %.2f°C, 20%02u-%02u-%02u (%s) %02u:%02u:%02u",
              address2String(telegram.source).c_str(),
              address2String(telegram.destination).c_str(),
              outsideTemp.get(),
              year.get(), month.get(), day.get(), weekDayStr.c_str(),
              hours.get(), minutes.get(), seconds.get());

    if (telegram.source==0x30 &&
        BusHandler::isBroadcastAddress(telegram.destination))
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
                 2000+year.get(), month.get(), day.get(),
                 hours.get(), minutes.get(), seconds.get());
        string lastTime(buf);

        bool modified = false;

        updateValue(modified, webData.lastTime, lastTime);

        if (modified) webData.write();
    }

}

//------------------------------------------------------------------------------

void MainMessageHandler::process0800(const Telegram& telegram)
     throw(OverrunException)
{
    DataSymbolReader reader(telegram);

    Data2b boilerTargetTemp(reader);
    Data2b outsideTemp(reader);
    Data1b forcePerformance(reader);
    BitData status(reader);
    Data2b serviceWaterTargetTemp(reader);

    string statusStr = bit2String(status, "BWR_active",
                                  "heater_circuit_active");

    Log::info("%s->%s RC Target Values: boiler temp: %.2f°C, outside temp: %.2f°C, service water temp: %.2f°C, force performance: %d%%, status:%s",
              address2String(telegram.source).c_str(),
              address2String(telegram.destination).c_str(),
              boilerTargetTemp.get(), outsideTemp.get(),
              serviceWaterTargetTemp.get(), forcePerformance.get(),
              statusStr.c_str());

    if (telegram.source==0xf1 &&
        BusHandler::isBroadcastAddress(telegram.destination))
    {
        bool modified = false;

        updateValue(modified, webData.boilerTargetTemp, boilerTargetTemp);
        updateValue(modified, webData.outsideTempAvg, outsideTemp);
        updateValue(modified, webData.serviceWaterTargetTemp,
                    serviceWaterTargetTemp);

        if (modified) webData.write();
    }
}

//------------------------------------------------------------------------------

void MainMessageHandler::process5014(const Telegram& telegram)
    throw(OverrunException)
{
    DataSymbolReader reader(telegram);

    BitData status(reader);
    Data2b boilerTargetTemp(reader);
    Data2b roomTemp(reader);
    Data2b mixerTemp(reader);

    string statusStr = bit2String(status);

    Log::info("%s->%s Control data (5014): boiler temp: %.2f°C, room temp: %.2f°C, mixer temp: %.2f°C, status:%s",
              address2String(telegram.source).c_str(),
              address2String(telegram.destination).c_str(),
              boilerTargetTemp.get(), roomTemp.get(), mixerTemp.get(),
              statusStr.c_str());

    bool modified = false;

    updateValue(modified, webData.roomTemp, roomTemp);

    if (modified) webData.write();
}

//------------------------------------------------------------------------------

int usage(bool error, char* argv[])
{
    FILE* f = error ? stderr : stdout;

    fprintf(f, "Usage: %s [-d <device file>] [-w <web file path>] [-f] [-l <log file path>] [-p <PID file path>]\n",
            argv[0]);
    fprintf(f, "  where\n");
    fprintf(f, "    -d <device file>: the device file to use (default: /dev/ttyUSB0)\n");
    fprintf(f, "    -w <web file path>: the JSON file to put the data into (default: ebus.json)\n");
    fprintf(f, "    -f: run in the foreground\n");
    fprintf(f, "    -l <log file path>: the path of the log file. If not given, no logging is done to a file\n");
    fprintf(f, "    -p <PID file path>: the path of the PID file (default: ebus.pid)\n");

    return error ? 1 : 0;
}

//------------------------------------------------------------------------------

void handleHUP(int /*signo*/)
{
    Log::reopenFile();
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    int opt;

    string deviceFile("/dev/ttyUSB0");
    string webFilePath("ebus.json");
    string pidFilePath("ebus.pid");
    bool foreground = false;
    string logFilePath;


    while((opt = getopt(argc, argv, "d:w:fl:hp:")) != -1) {
        switch (opt) {
          case 'd':
            deviceFile = optarg;
            break;
          case 'w':
            webFilePath = optarg;
            break;
          case 'f':
            foreground = true;
            break;
          case 'l':
            logFilePath = optarg;
            break;
          case 'p':
            pidFilePath = optarg;
            break;
          case 'h':
            return usage(false, argv);
            break;
          default:
            return usage(true, argv);
        }
    }

    if (!logFilePath.empty()) {
        Log::enableFile(logFilePath);

        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = &handleHUP;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sa.sa_restorer = 0;

        //signal(SIGHUP, &handleHUP);
        if (sigaction(SIGHUP, &sa, 0)<0) {
            perror("sigaction");
            return 3;
        }
    }

    if (foreground) {
        Log::enableStdout();
    } else {
        if (daemon(0, 0)<0) {
            perror("daemon");
            return 2;
        }
    }

    FILE* pidFile = fopen(pidFilePath.c_str(), "wt");
    if (pidFile!=0) {
        fprintf(pidFile, "%d\n", getpid());
        fclose(pidFile);
    }

    EBUS ebus(deviceFile);
    try {
        BusHandler busHandler(ebus);
        MainMessageHandler messageHandler(busHandler, webFilePath);

        while(true) {
            try {
                ebus.open();

                messageHandler.run();
            } catch(const OSError& e) {
                Log::error("OSError: %s, trying to open the port again",
                           e.what());
            }
        }
        return 0;
    } catch(const exception& e) {
        Log::error("Exception caught: %s", e.what());
        return 1;
    }
}

//------------------------------------------------------------------------------

// Local Variables:
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
