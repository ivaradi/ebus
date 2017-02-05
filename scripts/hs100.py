#!/usr/bin/env python
#-*- encoding: utf-8 -*-

import argparse
import sys
import socket
import json
import time
import os
import subprocess
import daemon
import pyinotify
import threading
import Queue
from ConfigParser import SafeConfigParser

#----------------------------------------------------------------------------------------

class Config(object):
    """The configuration."""
    def __init__(self):
        """Create a configuration.

        If the path is None, no values will be filled."""
        self.address = None
        self.port = 9999

        self.requestsDir = None
        self.statusFile = None
        self.pidFile = None

    def loadFrom(self, f):
        """Load the configuration from the given file."""
        try:
            configParser = SafeConfigParser()
            configParser.readfp(f)

            if configParser.has_option("device", "address"):
               self.address = configParser.get("device", "address")

            if configParser.has_option("device", "port"):
                self.port = configParser.getint("device", "port")

            if configParser.has_option("daemon", "requestsDir"):
                self.requestsDir = configParser.get("daemon", "requestsDir")

            if configParser.has_option("daemon", "statusFile"):
                self.statusFile = configParser.get("daemon", "statusFile")

            if configParser.has_option("daemon", "pidFile"):
                self.pidFile = configParser.get("daemon", "pidFile")
        except Exception, e:
            print >> sys.stderr, "Failed to read configuration:", e

    def setupFromArgs(self, args):
        """Set the values from the given arguments."""
        if "address" in args and args.address is not None:
            self.address = args.address
        if "port" in args and args.port is not None:
            self.port = args.port
        if "requestsDir" in args and args.requestsDir is not None:
            self.requestsDir = args.requestsDir
        if "statusFile" in args and args.statusFile is not None:
            self.statusFile = args.statusFile
        if "pidFile" in args and args.pidFile is not None:
            self.pidFile = args.pidFile

#----------------------------------------------------------------------------------------

class HS100Exception(Exception):
    """An exception signalling an issue with the result of a HS100 command."""
    def __init__(self, errorCode, message):
        """Construct the exception."""
        super(HS100Exception, self).__init__("HS100 error: %d: %s" %
                                             (errorCode, message))

#----------------------------------------------------------------------------------------

class HS100(object):
    """Class to represent a HS100 smart plug."""
    @staticmethod
    def encrypt(msg):
        """Encrypt the given message."""
        result = "\0" * 4

        key = 171
        for c in msg:
            a = ord(c) ^ key
            result += chr(a)
            key = a

        return result

    @staticmethod
    def decrypt(msg):
        """Decrypt the given message."""
        result = ""

        key = 171
        for c in msg[4:]:
            a = key ^ ord(c)
            result += chr(a)
            key = ord(c)

        return result

    def __init__(self, address, port = 9999):
        """Construct the object."""
        self._address = address
        self._port = port

    def getSystemInfo(self):
        """Get system information."""
        return self._simpleCall("system", "get_sysinfo")

    def reboot(self, delay = 1):
        """Reboot the device."""
        return self._simpleCall("system", "reboot", {"delay": delay})

    def reset(self, delay = 1):
        """Reset the device to factory settings."""
        return self._simpleCall("system", "reset", {"delay": delay})

    def setRelay(self, on):
        """Set the relay state to on or off."""
        return self._simpleCall("system", "set_relay_state",
                                {"state": 1 if on else 0})

    def setRelayOn(self):
        """Set the relay state to on."""
        return self.setRelay(True)

    def setRelayOff(self):
        """Set the relay state to off."""
        return self.setRelay(False)

    def setLED(self, on):
        """Set the LED state to on or off."""
        return self._simpleCall("system", "set_led_off",
                                {"off": 0 if on else 1})

    def setLEDOn(self):
        """Set the LED state to on."""
        return self.setLED(True)

    def setLEDOff(self):
        """Set the KED state to off."""
        return self.setLED(False)

    def setAlias(self, alias):
        """Set the alias of the device."""
        return self._simpleCall("system", "set_dev_alias",
                                {"alias": alias})

    def setMACAddress(self, mac):
        """Set the MAC address of the device."""
        return self._simpleCall("system", "set_mac_addr",
                                {"mac": mac})

    def setID(self, id):
        """Set the ID of the device."""
        return self._simpleCall("system", "set_device_id",
                                {"deviceId": id})

    def setHardwareID(self, id):
        """Set the hardware ID of the device."""
        return self._simpleCall("system", "set_hw_id",
                                {"hwId": id})

    def setLocation(self, longitude, latitude):
        """Set the geographical location of the device."""
        return self._simpleCall("system", "set_dev_location",
                                {"longitude": longitude,
                                 "latitude": latitude})
    def testCheckUBoot(self):
        """Check the U-Boot."""
        return self._simpleCall("system", "test_check_uboot")


    def getIcon(self):
        """Get the device icon."""
        return self._simpleCall("system", "get_dev_icon")

    def setIcon(self, iconData, iconHash):
        """Set the device icon."""
        return self._simpleCall("system", "set_dev_icon",
                                {"icon": iconData,
                                 "hash": iconHash})

    def setTestMode(self, enable = 1):
        """Set the test mode."""
        return self._simpleCall("system", "set_test_mode",
                                {"enable": enable})

    def downloadFirmware(self, url):
        """Start downloading the firmware from the given URL."""
        return self._simpleCall("system", "download_firmware",
                                {"url": url})

    def getDownloadState(self):
        """Get the download state."""
        return self._simpleCall("system", "get_download_state")

    def getDownloadState(self):
        """Flash the firmware download."""
        return self._simpleCall("system", "flash_firmware")

    def checkNewConfig(self):
        """Check the configuration."""
        return self._simpleCall("system", "check_new_config")


    def getWLANScanInfo(self, refresh = 1):
        """Get the WLAN scan info."""
        return self._simpleCall("netif", "get_scaninfo",
                                {"refresh": refresh })

    def setWLAN(self, ssid, password):
        """Setup the WLAN SSID and password"""
        return self._simpleCall("netif", "set_stainfo",
                                {"ssid" : ssid,
                                 "password" : password,
                                 "key_type" : 3})



    def getCloudInfo(self):
        """Get information on the cloud settings and status"""
        return self._simpleCall("cnCloud", "get_info")

    def getCloudFirmwareList(self):
        """Get the firmware list from the cloud server."""
        return self._simpleCall("cnCloud", "get_intl_firmware_list")

    def setCloudServerURL(self, url):
        """Set the cloud server URL."""
        return self._simpleCall("cnCloud", "set_server_url",
                                {"server": url})

    def bindCloud(self, username, password):
        """Bind to the cloud with the given user name and password."""
        return self._simpleCall("cnCloud", "bind",
                                {"username": username, "password": password})

    def unbindCloud(self):
        """Unbind from the cloud."""
        return self._simpleCall("cnCloud", "unbind")


    def getTime(self):
        """Get the device's current time."""
        return self._simpleCall("time", "get_time")

    def getTimezone(self):
        """Get the device's time zone."""
        return self._simpleCall("time", "get_timezone")

    def setTimezone(self, index, t = None):
        """Set the time zone.

        t is a struct_time, or if None, the current local time will be
        used."""
        if t is None:
            t = time.localtime()

        return self._simpleCall("time", "set_timezone",
                                {"year": t.tm_year,
                                 "month": t.tm_mon,
                                 "mday": t.tm_mday,
                                 "hour": t.tm_hour,
                                 "min": t.tm_min,
                                 "sec": t.tm_sec,
                                 "index": index})

    def getNextScheduledAction(self):
        """Get the next scheduled action."""
        return self._simpleCall("schedule", "get_next_action")

    def getScheduleRules(self):
        """Get the list of the schedule rules."""
        return self._simpleCall("schedule", "get_rules")

    def addScheduleRule(self, name, wday,
                        startTimeOpt, startMin, startAct,
                        endTimeOpt, endMin, endAct,
                        year = 0, month = 0, day = 0,
                        enable = 1, repeat = 1, force = 0,
                        longitude = 0, latitude = 0,
                        overallEnable = 1):
        """Add a schedule rule with the given data."""
        return self._simpleCall("schedule", "add_rule",
                                { "name": name,
                                  "wday" : wday,
                                  "stime_opt" : startTimeOpt,
                                  "smin": startMin,
                                  "sact": startAct,
                                  "etime_opt" : endTimeOpt,
                                  "emin": endMin,
                                  "eact": endAct,
                                  "year": year,
                                  "month": month,
                                  "day": day,
                                  "enable": enable,
                                  "repeat": repeat,
                                  "force": force,
                                  "longitude": longitude,
                                  "latitude": latitude,
                                  "set_overall_enable" : {
                                      "enable": overallEnable
                                      }})

    def editScheduleRule(self, id, name, wday,
                         startTimeOpt, startMin, startAct,
                         endTimeOpt, endMin, endAct,
                         year = 0, month = 0, day = 0,
                         enable = 1, repeat = 1, force = 0,
                         longitude = 0, latitude = 0,
                         overallEnable = 1):
        """Edit the a schedule rule with the given ID."""
        return self._simpleCall("schedule", "add_rule",
                                { "id": id,
                                  "name": name,
                                  "wday" : wday,
                                  "stime_opt" : startTimeOpt,
                                  "smin": startMin,
                                  "sact": startAct,
                                  "etime_opt" : endTimeOpt,
                                  "emin": endMin,
                                  "eact": endAct,
                                  "year": year,
                                  "month": month,
                                  "day": day,
                                  "enable": enable,
                                  "repeat": repeat,
                                  "force": force,
                                  "longitude": longitude,
                                  "latitude": latitude,
                                  "set_overall_enable" : {
                                      "enable": overallEnable
                                      }})

    def deleteScheduleRule(self, id):
        """Delete the schedule rule with the given ID."""
        return self._simpleCall("schedule", "delete_rule", {"id": id})

    def deleteAllScheduleRules(self):
        """Delete all schedule rules and statistics."""
        reply = self._call({"schedule":
                            {"delete_all_rules": None,
                             "erase_runtime_stat": None}})
        return self._processSimpleReply("schedule", "delete_all_rules")

    def getCountdownRules(self):
        """Get the countdown rules."""
        return self._simpleCall("count_down", "get_rules")

    def addCountdownRule(self, name, delay, enable = 1, act = 1):
        """Add a new countdown rule."""
        return self._simpleCall("count_down", "add_rule",
                                { "name": name,
                                  "delay": delay,
                                  "enable": enable,
                                  "act": act})

    def editCountdownRule(self, id, name, delay, enable = 1, act = 1):
        """Edit the countdown rule with the given ID."""
        return self._simpleCall("count_down", "add_rule",
                                { "id": id,
                                  "name": name,
                                  "delay": delay,
                                  "enable": enable,
                                  "act": act})

    def deleteCountdownRule(self, id):
        """Delete the countdown rule with the given ID."""
        return self._simpleCall("count_down", "delete_rule", { "id": id })

    def deletAllCountdownRules(self, id):
        """Delete all the countdown rules."""
        return self._simpleCall("count_down", "delete_all_rules")

    def getAntiTheftRules(self):
        """Get the anti-theft rules."""
        return self._simpleCall("anti_theft", "get_rules")

    def addAntiTheftRule(self, name, wday,
                         startTimeOpt, startMin,
                         endTimeOpt, endMin,
                         duration, lastFor, frequency,
                         year = 0, month = 0, day = 0,
                         enable = 1, repeat = 1, force = 0,
                         longitude = 0, latitude = 0,
                         overallEnable = 1):
        """Add a new anti-theft rule."""
        reply = self._call({"anti_theft": {"add_rule" :
                            { "name": name,
                              "stime_opt": startTimeOpt,
                              "smin": startMin,
                              "etime_opt": endTimeOpt,
                              "emin": endMin,
                              "duration": duration,
                              "lastfor": lastFor,
                              "frequency": frequency,
                              "year": year,
                              "month": month,
                              "day": day,
                              "enable": enable,
                              "repeat": repeat,
                              "force": force,
                               "longitude": longitude,
                               "latitude": latitude},
                             "set_overall_enable": overallEnable }})
        return self._processSimpleReply("anti_theft", "add_rule", reply)

    def editAntiTheftRule(self, id, name, wday,
                         startTimeOpt, startMin,
                         endTimeOpt, endMin,
                         duration, lastFor, frequency,
                         year = 0, month = 0, day = 0,
                         enable = 1, repeat = 1, force = 0,
                         longitude = 0, latitude = 0,
                         overallEnable = 1):
        """Edit the anti-theft rule with the given ID."""
        reply = self._call({"anti_theft": {"edit_rule" :
                            { "id" : id,
                              "name": name,
                              "stime_opt": startTimeOpt,
                              "smin": startMin,
                              "etime_opt": endTimeOpt,
                              "emin": endMin,
                              "duration": duration,
                              "lastfor": lastFor,
                              "frequency": frequency,
                              "year": year,
                              "month": month,
                              "day": day,
                              "enable": enable,
                              "repeat": repeat,
                              "force": force,
                               "longitude": longitude,
                               "latitude": latitude},
                             "set_overall_enable": overallEnable }})
        return self._processSimpleReply("anti_theft", "add_rule", reply)

    def deleteAntiTheftRule(self, id):
        """Delete the anti-theft rule with the given ID."""
        return self._simpleCall("anti_theft", "delete_rule", { "id" : id})

    def deleteAllAntiTheftRules(self):
        """Delete all anti-theft rules."""
        return self._simpleCall("anti_theft", "delete_all_rules")

    def _simpleCall(self, group, command, data = None):
        """Issue a simple call for the given group and command.

        The JSON dictionary is built up having the group as the key with the
        corresponding value being a dictionary with the command as the key and
        its value the given data.

        The reply is expected to be of the same structure, with the data being
        a dictionary that contains a key 'err_code'. If its value is not 0,
        a HS100Exception is thrown. Otherwise the data is returned."""
        reply = self._call({group: {command: data}})
        return self._processSimpleReply(group, command, reply)

    def _processSimpleReply(self, group, command, reply):
        """Process a simple reply."""
        replyData = reply[group][command]
        errorCode = replyData["err_code"]
        if errorCode==0:
            return replyData
        else:
            raise HS100Exception(errorCode, replyData["err_msg"])

    def _call(self, command):
        """Issue the given command to the plug.

        A connection is made to the HS100 and the given command is encrypted
        and sent. The reply is decrypted, decoded from JSON and returned as a
        dictionary.

        The command may be a string or another object which is then dumped into
        a string.

        Various exceptions may be thrown in case of network or encoding
        problems."""

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((self._address, self._port))
        if not isinstance(command, str):
            command = json.dumps(command)

        s.send(HS100.encrypt(command))
        reply = s.recv(4096)
        s.close()

        return json.loads(HS100.decrypt(reply))

#----------------------------------------------------------------------------------------

class HS110(HS100):
    """A HS110 smart plug, which adds the electricy meter functions."""
    def getEMeterRealtime(self):
        """Get real-time readings."""
        return self._simpleCommand("emeter", "get_realtime")

    def getEMeterGain(self):
        """Get the gain settings."""
        return self._simpleCommand("emeter", "get_vgain_igain")

    def setEMeterGain(self, vGain, iGain):
        """Set the gain settings."""
        return self._simpleCommand("emeter", "set_vgain_igain",
                                   {"vgain": vGain, "igain": iGain})

    def startEMeterCalibration(self, vTarget, iTarget):
        """Start the calibration."""
        return self._simpleCommand("emeter", "start_calibration",
                                   {"vtarget": vTarget, "itarget": iTarget})

    def getEMeterDailyStats(self, year, month):
        """Get daily statistics for the given month of the given year."""
        return self._simpleCommand("emeter", "get_daystat",
                                   {"month": month, "year": year})

    def getEMeterMonthlyStats(self, year):
        """Get monthly statistics for the given year."""
        return self._simpleCommand("emeter", "get_monthstat",
                                   {"year": year})

    def eraseEMeterStats(self, year):
        """Erase all statistics."""
        return self._simpleCommand("emeter", "erase_emeter_stat")

#----------------------------------------------------------------------------------------

def setupHS100(interface, deviceID, ssid, password,
               timezoneIndex = None, alias = None,
               longitude = None, latitude = None,
               cloudServer = None, port = 9999):
    """Setup the smart plug from factory state."""
    print "Bringing " + interface + " up"
    subprocess.call(["sudo", "ifconfig", interface, "down"])
    subprocess.call(["sudo", "ifconfig", interface, "0.0.0.0"])
    subprocess.call(["sudo", "ifconfig", interface, "up"])

    deviceSSID = "TP-LINK_Smart Plug_" + deviceID
    print "Looking for WLAN network " + deviceSSID + "..."
    found = False
    while not found:
        process = subprocess.Popen(["sudo", "iwlist", interface, "scan"],
                                   stdout = subprocess.PIPE,
                                   stderr = subprocess.PIPE)
        (stdout, stderr) = process.communicate()
        for line in stdout.splitlines():
            line = line.strip()
            if line=="ESSID:\"" + deviceSSID + "\"":
                found = True
                break
        if not found:
            print "  not found, sleeping..."
            time.sleep(3)

    print "Found network, connecting..."
    configured = False
    while not configured:
        retval = subprocess.call(["sudo", "iwconfig", interface,
                                  "essid", deviceSSID])
        if retval==0:
            configured = True
        else:
            print "  failed to configure SSID, sleeping before trying again..."
            time.sleep(3)

    time.sleep(10)

    connected = False
    while not connected:
        retval = subprocess.call(["sudo", "dhclient", interface])
        if retval==0:
            connected = True
        else:
            print "  DHCP client failed, sleeping before trying again..."
            time.sleep(3)

    isOK = True
    print "Connected, configuring device..."
    try:
        hs100 = HS100("192.168.0.1", port)
        if timezoneIndex is not None:
            print "  setting timezone..."
            hs100.setTimezone(timezoneIndex)
        if alias is not None:
            print "  setting alias to '%s'..." % (alias,)
            hs100.setAlias("Kazán")
        if longitude is not None and latitude is not None:
            print "  setting location to lon=%.4f, lat=%.4f..." % (longitude,
                                                                   latitude)
            hs100.setLocation(longitude, latitude)
        if cloudServer is not None:
            print "  setting the cloud server to %s..." % (cloudServer,)
            hs100.setCloudServerURL(cloudServer)

        print "  configuring SSID %s..." % (ssid,)
        hs100.setWLAN(ssid, password)

        print "Device configured."
    except Exception, e:
        isOK = False
        print >> sys.stderr, "Failed to configure device: %s" % (e,)

    print "Bringing " + interface + " down"
    subprocess.call(["sudo", "ifconfig", interface, "0.0.0.0"])
    subprocess.call(["sudo", "ifconfig", interface, "down"])

    return 0 if isOK else 1

#----------------------------------------------------------------------------------------

def handleSetup(config, args):
    """Handle the setup subcommand"""
    return setupHS100(args.interface, args.deviceID, args.ssid, args.password,
                      timezoneIndex = args.timezone, alias = args.alias,
                      longitude = args.longitude, latitude = args.latitude,
                      cloudServer = args.cloudserver)

#----------------------------------------------------------------------------------------

def handleSimple(config, args):
    """Handle a simple command which class a member function on the device."""
    if config.address is None:
        print >> sys.stderr, "Address is missing."""
        return 2

    hs100 = HS100(config.address, config.port)
    try:
        getattr(hs100, args.attr)()
        return 0
    except Exception, e:
        print >> sys.stderr, "Command failed: ", e
        return 1

#----------------------------------------------------------------------------------------

def addSimpleCommand(subparsers, command, help, attr):
    """Add a simple command to the given set of subparsers."""
    onParser = subparsers.add_parser(command, help = help)
    onParser.add_argument("address", default = None, nargs = "?",
                          help = "the address of the device")
    onParser.set_defaults(func = handleSimple)
    onParser.set_defaults(attr = attr)

#----------------------------------------------------------------------------------------

class Daemon(pyinotify.ProcessEvent):
    """The daemon functionality handling class."""
    def __init__(self, config):
        """Construct the daemon."""
        self._config = config
        self._hs100 = HS100(config.address, config.port)

        self._queue = Queue.Queue()

        self._inotifyThread = threading.Thread(target = self._handleInotify)
        self._inotifyThread.daemon = True

        self._statusFile = config.statusFile
        self._statusFileNew = config.statusFile + ".new"

        self._lastRequestTS = 0

    def run(self):
        """Run the daemon's operation."""
        if self._config.pidFile is not None:
            with open(self._config.pidFile, "w") as f:
                print >> f, os.getpid()

        self._updateStatus()

        self._inotifyThread.start()

        while True:
            try:
                item = self._queue.get(True, 10)
                item()
            except Queue.Empty:
                self._updateStatus()

    def _updateStatus(self):
        """Update the status."""
        try:
            systemInfo = self._hs100.getSystemInfo()

            t = long(time.time()*1000)

            data = {"lastRequest": self._lastRequestTS,
                    "updated": t,
                    "relayOn": systemInfo["relay_state"]==1,
                    "ledOn": systemInfo["led_off"]==0}
            with open(self._statusFileNew, "wt") as f:
                json.dump(data, f)
            os.rename(self._statusFileNew, self._statusFile)
        except Exception, e:
            print >> sys.stderr, "Failed to update status:", e

    def _handleInotify(self):
        """Setup and run the inotify loop."""
        wm = pyinotify.WatchManager()

        mask = pyinotify.IN_MOVED_TO

        notifier = pyinotify.Notifier(wm, self)

        wm.add_watch(self._config.requestsDir, mask)

        print "Runnig loop"
        notifier.loop()

    def process_IN_MOVED_TO(self, event):
        self._queue.put(lambda: self._processRequest(event.pathname))

    def _processRequest(self, path):
        print "_processRequest", path
        baseName = os.path.basename(path)
        if not baseName.startswith("request."):
            return

        with open(path, "rt") as f:
            data = json.load(f)

        if data["relayOn"]:
            self._hs100.setRelayOn()
        else:
            self._hs100.setRelayOff()

        if data["ledOn"]:
            self._hs100.setLEDOn()
        else:
            self._hs100.setLEDOff()

        self._lastRequestTS = long(baseName[8:])

        self._updateStatus()


#----------------------------------------------------------------------------------------

def handleDaemon(config, args):
    """The main function for the daemon operation."""
    if config.address is None:
        print >> sys.stderr, "Address is missing."""
        return 2
    if config.requestsDir is None:
        print >> sys.stderr, "The request file directory path is missing."""
        return 3
    if config.statusFile is None:
        print >> sys.stderr, "The status file path is missing."""
        return 4

    d = Daemon(config)
    if args.foreground:
        d.run()
    else:
        with daemon.DaemonContext():
            d.run()

#----------------------------------------------------------------------------------------

def main():
    """The main function."""
    parser = argparse.ArgumentParser("hs100",
                                     description = "TP-Link HS100/HS110 smart plug client")

    parser.add_argument("-c", "--config", default = None, type = file,
                        help = "the configuration file")

    parser.add_argument("-p", "--port", default = 9999, type = int,
                        help = "the port the smart plug listens on")

    subparsers = parser.add_subparsers(title = "subcommands",
                                       description = "subcommands accepted")

    setupParser = subparsers.add_parser("setup",
                                        help = "setup a device in factory state")
    setupParser.add_argument("-t", "--timezone", type = int,
                             help = "the time zone index")
    setupParser.add_argument("-a", "--alias", type = str,
                             help = "the device alias")
    setupParser.add_argument("-lon", "--longitude", type = float,
                             help = "the longitude of the device")
    setupParser.add_argument("-lat", "--latitude", type = float,
                             help = "the latitude of the device")
    setupParser.add_argument("-c", "--cloudserver", type = str,
                             help = "the URL of the cloud server")
    setupParser.add_argument("interface",
                             help = "the WLAN interface to use")
    setupParser.add_argument("deviceID",
                             help = "the last 2 bytes of the MAC address in hex")
    setupParser.add_argument("ssid",
                             help = "the SSID the device should connect to")
    setupParser.add_argument("password",
                             help = "the WLAN password for the given SSID")
    setupParser.set_defaults(func = handleSetup)

    addSimpleCommand(subparsers, "on", "turn the device's relay on",
                     "setRelayOn")
    addSimpleCommand(subparsers, "off", "turn the device's relay off",
                     "setRelayOff")

    addSimpleCommand(subparsers, "ledon", "turn the device's LED on",
                     "setLEDOn")
    addSimpleCommand(subparsers, "ledoff", "turn the device's LED off",
                     "setLEDOff")

    daemonParser = subparsers.add_parser("daemon",
                                         help = "run the program in daemon mode")
    daemonParser.add_argument("-a", "--address", default = None,
                              help = "the device's address")
    daemonParser.add_argument("-r", "--requestsDir", default = None,
                              help = "the directory containing the request files")
    daemonParser.add_argument("-s", "--statusFile", default = None,
                              help = "the file into which the status should be written")
    daemonParser.add_argument("-p", "--pidFile", default = None,
                              help = "the path of the PID file")
    daemonParser.add_argument("-f", "--foreground", action = "store_true",
                              help = "run in the foreground")
    daemonParser.set_defaults(func = handleDaemon)

    args = parser.parse_args()
    config = Config()
    if args.config is not None:
        config.loadFrom(args.config)
    config.setupFromArgs(args)

    return args.func(config, args)

if __name__ == "__main__":
    sys.exit(main())
