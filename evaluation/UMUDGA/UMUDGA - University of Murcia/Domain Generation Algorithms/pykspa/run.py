import json
from datetime import datetime
import time


def get_sld(length, seed):
    sld = ""
    modulo = 541 * length + 4
    a = length * length
    for i in range(length):
        index = (a + (seed*((seed % 5) + (seed % 123456) +
            i*((seed & 1) + (seed % 4567))) & 0xFFFFFFFF))  % 26
        a += length
        a &= 0xFFFFFFFFF
        sld += chr(ord('a') + index)
        seed += (((7837632 * seed * length) & 0xFFFFFFFF) + 82344) % modulo
    return sld


def generate_domains(date, nr, set_nr):
    with open("generators/pykspa/set{}_seeds.json".format(set_nr), "r") as r:
        seeds = json.load(r)
        dt = time.mktime(date.timetuple())
        days = 20 if set_nr == 1 else 1
        index = int(dt//(days*3600*24))
        if str(index) not in seeds:
            # print("Sorry, {} is out of the time range I know".format(date))
            return set()
        
        seed = int(seeds.get(str(index), None), 16)
        original_seed = seed 

        data = set()

        for dga_nr in range(nr):
            # determine next seed
            s = seed % (dga_nr + 1)
            seed += (s + 1)
            
            # second level length
            length = (seed + dga_nr) % 7 + 6  

            # get second level domain
            second_level_domain = get_sld(length, seed)

            # get first level domain
            tlds = ['com', 'net', 'org', 'info', 'cc']
            top_level_domain = tlds[(seed & 3)]

            # concatenate and print domain
            domain = second_level_domain + '.' +  top_level_domain
            data.add(domain)
        return data

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

def genNoise():
    directory = "data/pykspa_noise/list/"
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

    stop = False

    while not stop:

        date = randomDate("3/1/2013 01:00 AM", "3/1/2020 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        domains = generate_domains(d, 100, 2)

        for domain in domains:
            
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
            elif len(data) <= 5000:
                f5000.write("%s\n" % domain)
                f10000.write("%s\n" % domain)
                f50000.write("%s\n" % domain)
                f100000.write("%s\n" % domain)
            elif len(data) <= 10000:
                f10000.write("%s\n" % domain)
                f50000.write("%s\n" % domain)
                f100000.write("%s\n" % domain)
            elif len(data) <= 50000:
                f50000.write("%s\n" % domain)
                f100000.write("%s\n" % domain)
            elif len(data) <= 100000:
                f100000.write("%s\n" % domain)
            else:
                f1000.close()
                f5000.close()
                f10000.close()
                f50000.close()
                f100000.close()
                stop = True
                break
        
    # End While

def genUseful():
    directory = "data/pykspa/list/"
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

    while not stop:

        date = randomDate("3/1/2013 01:00 AM", "3/1/2020 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        domains = generate_domains(d, 100, 1)

        for domain in domains:
            
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
        
    # End While

if __name__=="__main__":
    genNoise()
    genUseful()
    
#End Program