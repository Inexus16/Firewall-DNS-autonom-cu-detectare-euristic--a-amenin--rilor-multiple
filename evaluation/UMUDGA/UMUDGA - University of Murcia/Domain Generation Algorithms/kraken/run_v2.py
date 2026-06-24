import time
import argparse
from datetime import datetime

def rand(r):
    t =  (1103515245 * r + 12435) & 0xFFFFFFFF
    return t

def crop(r):
    return (r // 256)  % 32768 

def dga(index, date, seed_set, temp_file=True, tld_set_nr=1):
    tld_sets = {1: ["com", "net", "tv", "cc"],
                2: ["dyndns.org", "yi.org", "dynserv.com", "mooo.com"]}

    seeds = {'a': {'ex': 24938314 , 'nex': 24938315 }, 
            'b': {'ex': 1600000, 'nex': 1600001}}
    tlds = tld_sets[tld_set_nr] 

    domain_nr = int(index/2)
    if temp_file:
        r = 3*domain_nr + seeds[seed_set]['ex']
    else:
        r = 3*domain_nr + seeds[seed_set]['nex']


    discards = (int(time.mktime(date.timetuple())) - 1207000000) // 604800  + 2
    if domain_nr % 9 < 8:
        if domain_nr % 9 >= 6:
            discards -= 1
        for _ in range(discards):
            r = crop(rand(r))

    rands = 3*[0]
    for i in range(3):
        r = rand(r)
        rands[i] = crop(r)
    domain_length = (rands[0]*rands[1] + rands[2]) % 6 + 7
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
    
    directory = "data/kraken_v2/list/"
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

    stop = False

    forceCloseCounter = 0
    dgaIndex = 0

    while not stop:

        date = randomDate("1/1/1970 01:00 AM", "1/1/3000 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        for temp_file in range(2):
            if stop:
                break
            for tld_set in [1,2]:
                if stop:
                    break
                for seed in ['a','b']:
                    if stop:
                        break
                    domain = dga(dgaIndex*2, d, seed, temp_file, tld_set)
            
                    datasize = len(data)
                    data.add(domain)
                    
                    # If it's a collision ignore it.
                    if len(data) == datasize:
                        forceCloseCounter = forceCloseCounter + 1
                        if forceCloseCounter == 10*counter:
                            f1000.close()
                            f5000.close()
                            f10000.close()
                            stop = True 
                        continue

                    counter = counter + 1

                    if len(data) <= 1000:
                        f1000.write("%s\n" % domain)
                        f5000.write("%s\n" % domain)
                        f10000.write("%s\n" % domain)
                    elif len(data) <= 5000:
                        f5000.write("%s\n" % domain)
                        f10000.write("%s\n" % domain)
                    elif len(data) <= 10000:
                        f10000.write("%s\n" % domain)
                    else:
                        f1000.close()
                        f5000.close()
                        f10000.close()
                        stop = True
                        break
                dgaIndex = dgaIndex + 1
    
    # End While
#End Program