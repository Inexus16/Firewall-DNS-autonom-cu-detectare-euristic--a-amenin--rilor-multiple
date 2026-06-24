import hashlib
from datetime import datetime, timedelta
import struct
import argparse

def get_seed(seq_nr, date):
    key = "\x01\x05\x19\x35"
    seq_nr = struct.pack('<I', seq_nr) 
    year = struct.pack('<H', date.year)
    month = struct.pack('<H', date.month)
    day = struct.pack('<H', date.day)
    m = hashlib.md5()
    m.update(seq_nr)
    m.update(year)
    m.update(key)
    m.update(month)
    m.update(key)
    m.update(day)
    m.update(key)
    return m.hexdigest()

def create_domain(seq_nr, date):
    def generate_domain_part(seed, nr):
        part = [] 
        for i in range(nr-1):
            edx = seed % 36
            seed /= 36
            if edx > 9:
                char = chr(ord('a') + (edx-10))
            else:
                char = chr(edx + ord('0'))
            part += char
            if seed == 0:
                break
        part = part[::-1]
        return ''.join(part)    

    def hex_to_int(seed):
        indices = range(0, 8, 2)
        data = [seed[x:x+2] for x in indices]
        seed = ''.join(reversed(data))
        return int(seed,16)

    seed_value = get_seed(seq_nr, date)
    domain = ""
    for i in range(0,16,4):
        seed = seed_value[i*2:i*2+8]
        seed = hex_to_int(seed)
        domain += generate_domain_part(seed, 8)
    if seq_nr % 4 == 0:
        domain += ".com"
    elif seq_nr % 3 == 0:
        domain += ".org"
    elif seq_nr % 2 == 0:
        domain += ".biz"
    else:
        domain += ".net"
    return domain


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
    
    directory = "data/zeus-newgoz/list/"
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
    forceCloseCounter = 0

    while not stop:

        date = randomDate("1/1/1970 01:00 AM", "1/1/3000 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        # Call the DGA
        # 4,294,967,295 is the max size for unsigned int of 4 bytes
        # https://docs.python.org/2/library/struct.html#format-characters
        # https://www.tutorialspoint.com/cprogramming/c_data_types.htm
        domain = create_domain(random.randint(1, 4294967295), d)
        
        datasize = len(data)
        data.add(domain)
        
        # If it's a collision ignore it.
        if len(data) == datasize:
            forceCloseCounter = forceCloseCounter + 1
            if forceCloseCounter == 10*counter:
                f1000.close()
                f5000.close()
                f10000.close()
                f50000.close()
                f100000.close()
                f500000.close()
                f1000000.close()
                stop = True 
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