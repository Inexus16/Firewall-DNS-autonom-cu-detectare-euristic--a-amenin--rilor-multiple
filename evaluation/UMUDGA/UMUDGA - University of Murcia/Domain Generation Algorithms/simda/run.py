import copy
bases = {
        # From https://www.johannesbader.ch/2015/03/the-dga-of-simda-shiz/
        0: {
            'length': 7,
            'tld': 'com',
            'key': '1676d5775e05c50b46baa5579d4fc7',
            'base': 0x45AE94B2
        },
        1: {
            'length': 5,
            'tld': 'eu',
            'key': '1670cf21500911e1758e2b0dd5b4',
            'base': 0x45AE94B2
        },
        2: {
            'length': 7,
            'tld': 'info',
            'key': '167cd47c0a09c9036d6097b754ab2e73',
            'base': 0x45AE94B2
        },
        3: {
            'length': 7,
            'tld': 'info',
            'key': 2038,
            'base': 0x45AE94B2
        }, 
        4: {
            'length': 11,
            'tld': 'eu',
            'key': "1670cf215403c56d8859a0636ffc74",
            'base': 0x45AE94B2
        }, 
        5: {
            'length': 7,
            'tld': 'info',
            'key': 2182,
            'base': 0x45AE94B2
        }
    }
consonants = "qwrtpsdfghjklzxcvbnmv"
vowels = "eyuioa"

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
    
    directory = "data/simda/list/"
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

        iv = random.choice(bases)

        if type(iv['key']) == int:
            step = iv['key']
        else:
            step = 0
            for m in iv['key']:
                step += ord(m)
            iv['key'] = step

        domain = ""
        iv['base'] += step

        for i in range(iv['length']):
            index = int(iv['base']/(3+2*i))
            if i % 2 == 0:
                char = consonants[index % 20]
            else:
                char = vowels[index % 6]
            domain += char

        domain += "." + iv['tld']
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