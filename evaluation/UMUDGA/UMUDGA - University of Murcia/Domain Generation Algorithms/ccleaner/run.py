def msvcrt_rand(seed):
    new_seed = (0x343fd * seed + 0x269ec3) & ((1 << 32) - 1)
    randval = (new_seed >> 16) & 0x7fff
    return randval, new_seed

def dga(year, month):

    r1, seed = msvcrt_rand(year * 10000 + month)
    r2, seed = msvcrt_rand(seed)
    r3, seed = msvcrt_rand(seed)
    

    sld = 'ab%x%x' %(r2 * r3, r1)

    return sld + '.com'

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
    
    directory = "data/ccleaner/list/"
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

    while not stop:

        for y in range(2000,3000):
            for m in range(1,12):
                # Call the DGA
                domain = dga(y, m)
                
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
    
    # End While
#End Program