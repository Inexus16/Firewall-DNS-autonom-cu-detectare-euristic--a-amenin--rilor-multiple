import sys
import time
import string
from datetime import datetime

class domain_generator:
    def __init__(self, aseed):
        self.const1 = 0xef5eb
        self.const2 = 0x39339
        with open("generators/matsnu/wordlists/1.txt", 'rb') as f:
            self.dict1 = f.read().split('\n')
        with open("generators/matsnu/wordlists/2.txt", 'rb') as f:
            self.dict2 = f.read().split('\n')
        self.seed = aseed
        self.daysSinceEpoch = None
        self.next_domain_no = 1

    def choose_next_word(self, dictionary):
        self.seed &= 0xffff
        self.seed = (self.seed * self.const1) & 0xffff
        self.seed = (self.seed * self.daysSinceEpoch) & 0xffff
        self.seed = (self.seed * self.const2) & 0xffff
        self.seed = (self.seed * self.next_domain_no) & 0xffff
        self.seed = (self.seed ^ self.const1) & 0xffff

        rem = dictionary[self.seed % len(dictionary)].strip()
        return rem

    def generate_domain(self):
        domain = ''
        self.parity_flag = 0
        while len(domain) < 0x18:
            if len(domain) > 0xc:
                break
            if len(domain) == 0:
                domain += self.choose_next_word(self.dict1)
            elif self.parity_flag == 0:
                domain += self.choose_next_word(self.dict1)
            else:
                domain += self.choose_next_word(self.dict2)

            self.parity_flag = (self.parity_flag + 1) % 2

            if self.seed & 0x1 == 0x1:
                domain += '-'
        if domain[-1] == '-':
            domain = domain[:-1]

        domain += '.com'
        self.next_domain_no += 1
        return domain

    def set_days_since_epoch(self, date):
        epoch = datetime.utcfromtimestamp(0)
        self.daysSinceEpoch = (date - epoch).days


import sys
import random

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
    
    directory = "data/matsnu/list/"
    seed = "3138C81ED54AD5F8E905555A6623C9C9"
    intseed = 521496385

    dga = domain_generator(1)

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

        date = randomDate("1/2/1970 01:00 AM", "1/1/3000 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        dga.set_days_since_epoch(d)

        # Call the DGA
        domain = dga.generate_domain()
        
        datasize = len(data)
        data.add(domain)
        
        # If it's a collision ignore it.
        if len(data) == datasize:
            dga.next_domain_no = 1
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