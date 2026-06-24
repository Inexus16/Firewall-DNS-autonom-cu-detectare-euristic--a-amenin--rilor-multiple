def generate_domains(seed, words):
    nr = seed
    res = 16*[0]
    shuffle = [3, 9, 13, 6, 2, 4, 11, 7, 14, 1, 10, 5, 8, 12, 0]
    for i in range(15):
        res[shuffle[i]] = nr % 2
        nr = nr >> 1

    first_word_index = 0
    for i in range(7):
        first_word_index <<= 1
        first_word_index ^= res[i]

    second_word_index = 0
    for i in range(7,15):
        second_word_index <<= 1
        second_word_index ^= res[i]
    second_word_index += 0x80

    first_word = words[first_word_index]
    second_word = words[second_word_index]
    tld = ".net"
    seed += 1
    return "" + first_word + "" + second_word + "" + tld, seed


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

    wordlistName = "3"

    words = None

    with open("generators/suppobox/wordlists/words{}.txt".format(wordlistName), "r") as r:
        words = [w.strip() for w in r.readlines()]
    
    directory = "data/suppobox_" + wordlistName + "/list/"
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
    # Wordlists have only 384 words, results are limited to 100k group

    stop = False
    forceCloseCounter = 0

    while not stop:

        date = randomDate("1/1/1970 01:00 AM", "1/1/3000 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")
        t = time.mktime(d.timetuple())
        
        genseed = seed = int(t) >> 9
        tmpExit = 0

        while True:
            domain, genseed = generate_domains(genseed, words)
            
            datasize = len(data)
            data.add(domain)
            
            # If it's a collision ignore it.
            if len(data) == datasize:
                forceCloseCounter = forceCloseCounter + 1
                tmpExit = tmpExit + 1
                if forceCloseCounter == 10*counter:
                    f1000.close()
                    f5000.close()
                    f10000.close()
                    f50000.close()
                    f100000.close()
                    stop = True 
                if tmpExit > len(data):
                    break
                else:
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
#End Program