from datetime import datetime
from ctypes import c_uint

wordlists = {
    'luther': (4, '.com'), 
    'rfc4343': (3, '.com'), 
    'nasa': (5, '.com'),
    'gpl': (5, '.ru')
    }

seeds = {
        'luther': {
            'div': 4, 
            'tld': '.com', 
            'nr': 12
            },
        'rfc4343': {
            'div': 3, 
            'tld': '.com', 
            'nr': 10
            },
        'nasa': {
            'div': 5, 
            'tld': '.com', 
            'nr': 12
            },
        'gpl': {
            'div': 5, 
            'tld': '.ru', 
            'nr': 10
            }
        }
        
        
class Rand:

    def __init__(self, seed):
        self.r = c_uint(seed) 

    def rand(self):
        self.r.value = 1664525*self.r.value + 1013904223
        return self.r.value

def get_words(wordlist):
    with open("generators/gozi/wordlists/" + wordlist, 'r') as r:
        return [w.strip() for w in r if w.strip()]

def dga(date, wordlist):
    r.rand()
    v = r.rand()
    length = v % 12 + 12
    domain = ""
    while len(domain) < length:
        v = r.rand() % len(words)
        word = words[v] 
        l = len(word)
        if not r.rand() % 3:
            l >>= 1
        if len(domain) + l <= 24:
            domain += word[:l]
    domain += seeds[wordlist]['tld']
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

words = None
diff = None
days_passed = None
flag = None
dgaseed = None
r = None
#


if __name__=="__main__":
    
    wordlistName = 'luther'

    wordlist = wordlists[wordlistName]

    directory = "data/gozi_" + wordlistName + "/list/"
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

        date = randomDate("1/1/2015 01:00 AM", "1/1/3000 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        words = get_words(wordlistName)
        diff = d - datetime.strptime("2015-01-01", "%Y-%m-%d")
        days_passed = (diff.days // seeds[wordlistName]['div'])
        flag = 1
        dgaseed = (flag << 16) + days_passed - 306607824
        r = Rand(dgaseed) 

        domain = dga(d, wordlistName)
#         
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