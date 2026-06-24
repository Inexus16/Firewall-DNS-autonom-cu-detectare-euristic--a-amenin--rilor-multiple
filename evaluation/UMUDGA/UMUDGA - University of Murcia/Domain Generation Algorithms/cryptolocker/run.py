tlds = ["com", "net", "biz", "ru", "org", "co.uk", "info"]

def domainsFor(date):
    domains = []
    for z in range(1000):
      d = date.day
      m = date.month
      y = date.year + z
 
      d *= 65537
      m *= 65537
      y *= 65537
       
      s = d >> 3 ^ y >> 8 ^ y >> 11
      s &= 3
      s += 12
 
      n = ""
      for _ in range(s):
        d = ((d << 13 & 0xFFFFFFFF) >> 19 & 0xFFFFFFFF) ^ ((d >> 1 & 0xFFFFFFFF) << 13 & 0xFFFFFFFF) ^ (d >> 19 & 0xFFFFFFFF)
        d &= 0xFFFFFFFF
        m = ((m << 2 & 0xFFFFFFFF) >> 25 & 0xFFFFFFFF) ^ ((m >> 3 & 0xFFFFFFFF) << 7 & 0xFFFFFFFF)  ^ (m >> 25 & 0xFFFFFFFF)
        m &= 0xFFFFFFFF
        y = ((y << 3 & 0xFFFFFFFF) >> 11 & 0xFFFFFFFF) ^ ((y >> 4 & 0xFFFFFFFF) << 21 & 0xFFFFFFFF) ^ (y >> 11 & 0xFFFFFFFF)
        y &= 0xFFFFFFFF
         
        n += chr(ord('a') + (y ^ m ^ d) % 25)
 
      domain = n + "." + tlds[z % 7]
      domains.append(domain)
    return domains

from datetime import datetime
import sys
import random
import time

def getRandomDate(start="1/1/1970 01:00 AM", end="1/1/3000 1:10 AM", format='%m/%d/%Y %I:%M %p'):
    rand = random.random()
    stime = time.mktime(time.strptime(start, format))
    etime = time.mktime(time.strptime(end, format))
    ptime = stime + rand * (etime - stime)
    return datetime.strptime(time.strftime(format, time.localtime(ptime)), format)

def getRandomNumber(min=1, max=sys.maxsize):
    return random.randint(min, max)

# HERE THE DGA FUNCTION

if __name__=="__main__":
    
    directory = "data/cryptolocker/list/"
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

        date = getRandomDate()

        for domain in domainsFor(date):

            if stop:
                break

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