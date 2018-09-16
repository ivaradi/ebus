#!/usr/bin/env python
# -*- encoding: utf-8 -*-

import sys
import smtplib

if __name__ == "__main__":
    errorCode = int(sys.argv[1])

    msg = "From: eBUS démon <ebus@superhost.com>\r\n"
    msg += "To: ivaradi@mail.superhost.com\r\n"
    msg += "Subject: Kazán hiba: %03d\r\n" % (errorCode,)
    msg += "\r\n"
    msg += "Üdvözletem!\r\n"
    msg += "\r\n"
    msg += "A kazán hibát jelzett %03d kóddal!\r\n" % (errorCode,)
    msg += "\r\n"
    msg += "Az eBUS démon\r\n"

    server = smtplib.SMTP_SSL()
    server.connect("mail.superhost.com", 25)
    #server.set_debuglevel(9)
    server.sendmail("ebus@superhost.com",
                    ["ivaradi@mail.superhost.com"],
                    msg)
    server.quit()
