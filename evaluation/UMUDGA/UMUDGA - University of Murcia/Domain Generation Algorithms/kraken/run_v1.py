import time
from ctypes import c_int, c_uint
import argparse

def rand(r):
    t =  c_int(1103515245 * r + 12435).value
    return t

def crop(r):
    return (r // 256)  % 32768 

def dga(index, seed_set, temp_file=True):

    seeds = {'a': {'ex': -0x0FCFBF88, 'nex': 0x8924541}, 
            'b': {'ex': -0x1FCFBF87, 'nex': 0x7924542}}

    tlds = ["dyndns.org", "yi.org", "dynserv.com", "mooo.com"]
    domain_nr = int(index/2) + 1000015

    if temp_file:
        x = int(c_int(domain_nr*(domain_nr + 7)*(domain_nr+12)).value /9.0) 
        y = domain_nr*(domain_nr+1)
        r = c_int(x + y + seeds[seed_set]['ex']).value 
    else:
        x = int(c_int((domain_nr + 2)*(domain_nr + 7)*domain_nr).value/9.0)
        y = (domain_nr*3 + 1)*domain_nr
        r = c_int(x + y + seeds[seed_set]['nex']).value

    rands = 3*[0]
    for i in range(3):
        r = rand(r)
        rands[i] = crop(r)
    domain_length = (rands[0]*rands[1] - rands[2]) % 6 + 6
    domain = ""
    for i in range(domain_length):
        r = rand(r)
        ch = crop(r) % 26 + ord('a')
        domain += chr(ch)
    domain += "." + tlds[domain_nr % 4]
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
    
    directory = "data/kraken_v1/list/"
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
    
    dgaIndex = 0

    while not stop:

        date = randomDate("1/1/1970 01:00 AM", "1/1/3000 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        domains = []
        for temp_file in range(2):
            domains.append(dga(dgaIndex*2, "a", temp_file))
            domains.append(dga(dgaIndex*2, "b", temp_file))

        for domain in domains:
            
            datasize = len(data)
            data.add(domain)
            
            if len(data) == datasize:
                forceCloseCounter = forceCloseCounter + 1
                if forceCloseCounter == 2*counter:
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
                
        dgaIndex = dgaIndex + 1
    
    # End While
#End Program