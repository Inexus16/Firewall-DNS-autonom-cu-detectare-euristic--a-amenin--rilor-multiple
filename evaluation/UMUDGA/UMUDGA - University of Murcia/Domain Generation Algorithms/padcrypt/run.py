"""
    The DGA of PadCrypt 

    See 
    - https://twitter.com/BleepinComputer/status/705813885673201665
    - http://www.bleepingcomputer.com/news/security/padcrypt-the-first-ransomware-with-live-support-chat-and-an-uninstaller/
    - http://www.bleepingcomputer.com/news/security/the-padcrypt-ransomware-is-still-alive-and-kicking/
    - http://johannesbader.ch/2016/03/the-dga-of-padcrypt/

"""

import argparse
import hashlib
from datetime import datetime

configs = {
    "2.2.86.1" : {
        'nr_domains': 24,
        'tlds': ['com', 'co.uk', 'de', 'org', 'net', 'eu', 'info', 'online',
            'co', 'cc', 'website'],
        'digit_mapping': "abcdnfolmk",
        'separator': ':',
        },
    "2.2.97.0" : {
        'nr_domains': 24*3,
        'tlds': ['com', 'co.uk', 'de', 'org', 'net', 'eu', 'info', 'online',
            'co', 'cc', 'website'],
        'digit_mapping': "abcdnfolmk",
        'separator': '|'
        }
}

def dga(date, config_nr):
    config = configs[config_nr]
    dm = config['digit_mapping']
    tlds = config['tlds']
    for i in range(config['nr_domains']):
        seed_str = "{}-{}-{}{}{}".format(date.day, date.month, date.year,
                config['separator'], i)
        h = hashlib.sha256(seed_str.encode('ascii')).hexdigest()
        domain = ""
        for hh in h[3:16+3]:
            domain += dm[int(hh)] if '0' <= hh <= '9' else hh
        tld_index = int(h[-1], 16)
        tld_index = 0 if tld_index >= len(tlds) else tld_index
        domain += "." + config['tlds'][tld_index]
        yield domain

from datetime import datetime
import sys
import random
import time

def strTimeProp(start, end, format, prop):
    """Get a time at a proportion of a range of two formatted times.
    start and end should be strings specifying times formated in the
    given format (strftime-style), giving an interval [start, end].
    prop specifies how a proportion of the interval to be taken after
    start.  The returned time will be in the specified format.
    """

    stime = time.mktime(time.strptime(start, format))
    etime = time.mktime(time.strptime(end, format))

    ptime = stime + prop * (etime - stime)

    return time.strftime(format, time.localtime(ptime))
  
def randomDate(start, end, prop):
    return strTimeProp(start, end, '%m/%d/%Y %I:%M %p', prop)

# HERE THE DGA FUNCTION

if __name__=="__main__":
    
    directory = "data/padcrypt/list/"
    seed = "3138C81ED54AD5F8E905555A6623C9C9"
    intseed = 521496385

    import os
    if not os.path.exists(directory):
        os.makedirs(directory)


    random.seed(intseed)
    
    counter = 0

    data = set()

    f1000 = open(directory + "1000.txt","w")
    f5000 = open(directory + "5000.txt","w")
    f10000 = open(directory + "10000.txt","w")
    f50000 = open(directory + "50000.txt","w")
    f100000 = open(directory + "100000.txt","w")
    f500000 = open(directory + "500000.txt","w")
    f1000000 = open(directory + "1000000.txt","w")

    stop = False

    while not stop:

        date = randomDate("1/1/1970 01:00 AM", "1/1/3000 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        for domain in dga(d,"2.2.97.0"): 
            
            datasize = len(data)
            data.add(domain)
            
            # If it's a collision ignore it.
            if len(data) == datasize:
                continue

            counter = counter + 1

            if len(data) <= 1000:
                f1000.write("%s\n" % domain)
                f5000.write("%s\n" % domain)
                f10000.write("%s\n" % domain)
                f50000.write("%s\n" % domain)
                f100000.write("%s\n" % domain)
                f500000.write("%s\n" % domain)
                f1000000.write("%s\n" % domain)
            elif len(data) <= 5000:
                f5000.write("%s\n" % domain)
                f10000.write("%s\n" % domain)
                f50000.write("%s\n" % domain)
                f100000.write("%s\n" % domain)
                f500000.write("%s\n" % domain)
                f1000000.write("%s\n" % domain)
            elif len(data) <= 10000:
                f10000.write("%s\n" % domain)
                f50000.write("%s\n" % domain)
                f100000.write("%s\n" % domain)
                f500000.write("%s\n" % domain)
                f1000000.write("%s\n" % domain)
            elif len(data) <= 50000:
                f50000.write("%s\n" % domain)
                f100000.write("%s\n" % domain)
                f500000.write("%s\n" % domain)
                f1000000.write("%s\n" % domain)
            elif len(data) <= 100000:
                f100000.write("%s\n" % domain)
                f500000.write("%s\n" % domain)
                f1000000.write("%s\n" % domain)
            elif len(data) <= 500000:
                f500000.write("%s\n" % domain)
                f1000000.write("%s\n" % domain)	
            elif len(data) <= 1000000:
                f1000000.write("%s\n" % domain)
            else:
                f1000.close()
                f5000.close()
                f10000.close()
                f50000.close()
                f100000.close()
                f500000.close()
                f1000000.close()
                stop = True
                break
    
    # End While
#End Program
    