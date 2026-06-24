import argparse

class RandInt:

    def __init__(self, seed): 
        self.value = seed

    def rand_int_modulus(self, modulus):
        ix = self.value
        ix = 16807*(ix % 127773) - 2836*(ix // 127773) & 0xFFFFFFFF        
        self.value = ix 
        return ix % modulus 

def get_domain(r_int):
    seed_a = r_int.value
    domain_len = r_int.rand_int_modulus(12) + 8
    seed_b = r_int.value
    domain = ""
    for _ in range(domain_len):
        char = chr(ord('a') + r_int.rand_int_modulus(25))
        domain += char
    tld = random.choice(tlds)
    domain += '.' if tld[0] != '.' else ''
    domain += tld
    m = seed_a*seed_b
    r_int.value = (m + m//(2**32)) % 2**32 
    return domain

seeds = [
    # From https://github.com/baderj/domain_generation_algorithms/blob/master/ramnit/dga.py
    4011936703,
    675843818,
    1274854506,
    2031459344,
    2465513013,
    1124253770,
    1378321992,
    2538799770,
    # From https://www.cert.pl/en/news/single/ramnit-in-depth-analysis/
    790544302,
    1124253770,
    1108585239,
    1458440109,
    2039546858,
    2435699865,
    2695420049,
    2960547961,
    3738229229,
    3801515385,
    3815882521,
    3998246919,
    4040478694,
    4096376725,
    4205202272,
    57607789,
    697527549,
    742724187,
    # From https://www.symantec.com/content/dam/symantec/docs/security-center/white-papers/w32-ramnit-analysis-15-en.pdf
    2031459344,
    1823844313,
    3750708132,
    3245545401,
    1690227383,
    3489430955,
    1274854506,
    1321735650,
    1526939975,
    1499543098,
    113381493,
    675843818,
    840290331,
    1617769919,
    2391944259,
    376310571
]

tlds = [
    ".com"
]

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
    
    directory = "data/ramnit/list/"
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

    r_int = RandInt(random.choice(seeds))

    while not stop:

        date = randomDate("1/1/1970 01:00 AM", "1/1/3000 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        # Call the DGA
        domain = get_domain(r_int)
        
        datasize = len(data)
        data.add(domain)
        
        # If it's a collision ignore it.
        if len(data) == datasize:
            forceCloseCounter = forceCloseCounter + 1

            r_int = RandInt(random.choice(seeds))

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