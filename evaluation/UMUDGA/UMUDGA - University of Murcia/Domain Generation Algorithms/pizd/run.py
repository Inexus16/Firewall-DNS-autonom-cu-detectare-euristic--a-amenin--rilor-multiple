import argparse
from datetime import datetime
import numpy as np

wordlist = [
    # From https://github.com/baderj/domain_generation_algorithms/blob/master/pizd/pizd
    'above',
    'action',
    'advance',
    'afraid',
    'against',
    'airplane',
    'almost',
    'alone',
    'already',
    'although',
    'always',
    'amount',
    'anger',
    'angry',
    'animal',
    'another',
    'answer',
    'appear',
    'apple',
    'around',
    'arrive',
    'article',
    'attempt',
    'banker',
    'basket',
    'battle',
    'beauty',
    'became',
    'because',
    'become',
    'before',
    'begin',
    'behind',
    'being',
    'believe',
    'belong',
    'beside',
    'better',
    'between',
    'beyond',
    'bicycle',
    'board',
    'borrow',
    'bottle',
    'bottom',
    'branch',
    'bread',
    'bridge',
    'bright',
    'bring',
    'broad',
    'broken',
    'brought',
    'brown',
    'building',
    'built',
    'business',
    'butter',
    'captain',
    'carry',
    'catch',
    'caught',
    'century',
    'chair',
    'chance',
    'character',
    'charge',
    'chief',
    'childhood',
    'children',
    'choose',
    'cigarette',
    'circle',
    'class',
    'clean',
    'clear',
    'close',
    'clothes',
    'college',
    'company',
    'complete',
    'condition',
    'consider',
    'contain',
    'continue',
    'control',
    'corner',
    'country',
    'course',
    'cover',
    'crowd',
    'daughter',
    'decide',
    'degree',
    'delight',
    'demand',
    'desire',
    'destroy',
    'device',
    'difference',
    'different',
    'difficult',
    'dinner',
    'direct',
    'discover',
    'distance',
    'distant',
    'divide',
    'doctor',
    'dollar',
    'double',
    'doubt',
    'dress',
    'dried',
    'during',
    'early',
    'eearly',
    'effort',
    'either',
    'electric',
    'electricity',
    'english',
    'enough',
    'enter',
    'escape',
    'evening',
    'every',
    'except',
    'expect',
    'experience',
    'explain',
    'family',
    'famous',
    'fancy',
    'father',
    'fellow',
    'fence',
    'fifteen',
    'fight',
    'figure',
    'finger',
    'finish',
    'flier',
    'flower',
    'follow',
    'foreign',
    'forest',
    'forever',
    'forget',
    'fortieth',
    'forward',
    'found',
    'fresh',
    'friend',
    'further',
    'future',
    'garden',
    'gather',
    'general',
    'gentle',
    'gentleman',
    'glass',
    'glossary',
    'goodbye',
    'govern',
    'guard',
    'happen',
    'health',
    'heard',
    'heart',
    'heaven',
    'heavy',
    'history',
    'honor',
    'however',
    'hunger',
    'husband',
    'include',
    'increase',
    'indeed',
    'industry',
    'inside',
    'instead',
    'journey',
    'kitchen',
    'known',
    'labor',
    'ladder',
    'language',
    'large',
    'laugh',
    'laughter',
    'leader',
    'leave',
    'length',
    'letter',
    'likely',
    'listen',
    'little',
    'machine',
    'manner',
    'market',
    'master',
    'material',
    'matter',
    'mayor',
    'measure',
    'meeting',
    'member',
    'method',
    'middle',
    'might',
    'million',
    'minute',
    'mister',
    'modern',
    'morning',
    'mother',
    'mountain',
    'movement',
    'nation',
    'nature',
    'nearly',
    'necessary',
    'needle',
    'neighbor',
    'neither',
    'niece',
    'night',
    'north',
    'nothing',
    'notice',
    'number',
    'object',
    'oclock',
    'office',
    'often',
    'opinion',
    'order',
    'orderly',
    'outside',
    'paint',
    'partial',
    'party',
    'people',
    'perfect',
    'perhaps',
    'period',
    'person',
    'picture',
    'pleasant',
    'please',
    'pleasure',
    'position',
    'possible',
    'power',
    'prepare',
    'present',
    'president',
    'pretty',
    'probable',
    'probably',
    'problem',
    'produce',
    'promise',
    'proud',
    'public',
    'quarter',
    'question',
    'quiet',
    'rather',
    'ready',
    'realize',
    'reason',
    'receive',
    'record',
    'remember',
    'report',
    'require',
    'result',
    'return',
    'ridden',
    'right',
    'river',
    'round',
    'safety',
    'school',
    'season',
    'separate',
    'service',
    'settle',
    'severa',
    'several',
    'shake',
    'share',
    'shore',
    'short',
    'should',
    'shoulder',
    'shout',
    'silver',
    'simple',
    'single',
    'sister',
    'smell',
    'smoke',
    'soldier',
    'space',
    'speak',
    'special',
    'spent',
    'spread',
    'spring',
    'square',
    'station',
    'still',
    'store',
    'storm',
    'straight',
    'strange',
    'stranger',
    'stream',
    'street',
    'strength',
    'strike',
    'strong',
    'student',
    'subject',
    'succeed',
    'success',
    'sudden',
    'suffer',
    'summer',
    'supply',
    'suppose',
    'surprise',
    'sweet',
    'system',
    'therefore',
    'thick',
    'think',
    'third',
    'those',
    'though',
    'thought',
    'through',
    'thrown',
    'together',
    'toward',
    'trade',
    'train',
    'training',
    'travel',
    'trouble',
    'trust',
    'twelve',
    'twenty',
    'understand',
    'understood',
    'until',
    'valley',
    'value',
    'various',
    'wagon',
    'water',
    'weather',
    'welcome',
    'wheat',
    'whether',
    'while',
    'white',
    'whose',
    'window',
    'winter',
    'within',
    'without',
    'woman',
    'women',
    'wonder',
    'worth',
    'would',
    'write',
    'written',
    'yellow']

def generate_domain(timestamp,wordl):
 """
 Generate one domain name
 :param timestamp: tinm
 :param wordl:
 :return:
 """
 inv_key = [0, 5, 10, 14 ,9 ,3 ,11, 7, 2, 13, 4, 8, 1, 12, 6]
 bin_temp = timestamp[-15::1]
 nib=np.int_(np.zeros(len(bin_temp)))
 for x in range(0,14):
     nib[x] = bin_temp[inv_key[x]]
 res = [''.join([str(char) for char in nib[:7]]),''.join([str(char) for char in nib[7:]])]
 res = [wordl[int(res[0],2)], wordl[int(res[1],2)+128] , ".net" ]
 return ''.join([str(wds) for wds in res])


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
    
    directory = "data/pizd/list/"
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

        date = randomDate("1/1/1970 01:00 AM", "1/1/3000 1:10 AM", random.random())
        d = datetime.strptime(date, "%m/%d/%Y %I:%M %p")

        totalseconds = time.mktime(d.timetuple())

        # Call the DGA
        domain = generate_domain(bin(int(totalseconds*1000) + counter), wordlist)
        
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