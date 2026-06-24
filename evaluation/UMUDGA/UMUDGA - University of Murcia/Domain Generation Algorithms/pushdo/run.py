import os, time, datetime, hashlib

def rc4crypt(data, key):
    x = 0
    box = range(256)
    for i in range(256):
        x = (x + box[i] + ord(key[i % len(key)])) % 256
        box[i], box[x] = box[x], box[i]
    x = 0
    y = 0
    out = []
    for char in data:
        x = (x + 1) % 256
        y = (y + box[x]) % 256
        box[x], box[y] = box[y], box[x]
        out.append(chr(ord(char) ^ box[(box[x] + box[y]) % 256]))
    
    return ''.join(out)

def hasher(data, algorithm="md5"):
    h = hashlib.new(algorithm)
    h.update(data)
    return h.hexdigest()

def generateSeed(a1, a2, a3) :
  result = ''
  v4 = ''
  v5 = 0
  v6 = 0
  v7 = 0
  v8 = list("1F1C1F1E1F1E1F1F1E1F1E1F".decode("hex"))
  result = 0
  if ( a1 > 0 ) :
    if ( (a2 - 1) <= 0xB ) :
      if ((a3 - 1) <= 0x1E ) :
        v4 = (a1 & 0x80000003) == 0
        if ( (a1 & 0x80000003) < 0 ) :
          v4 = (((a1 & 0x80000003) - 1) | 0xFFFFFFFC) == -1
        if ( v4 ) :
          v8[11] = chr(0x1D)
        v5 = 0
        if ( a2 > 1 ) :
          v7 = v8 #&v8
          v6 = a2 - 1
	  i7 = 0
          while (v6) :
            v5 += ord(v7[i7]) #*v7
            i7 += 1
            v6 -= 1
        ecx = 365 * (a1 - (a1 / 4))
        eax = 366 * (a1 / 4)
        result = a3 + v5 + ecx + eax

  return result

def generateString(salt, seed):
    buf = ''
    tmp = "%08x" % seed
    tmp = tmp.decode("hex")
    for i in range(4) :
        buf = tmp[i]+buf
    return buf

def generateDomain(mdhash, length):
    buf = ''
    for c in mdhash:
        if len(buf) > length :
            return buf
	bl = ord(c)
	v1 = "aiou"
	v2 = "aeiouy"
    	c1 = "bcdfghjklmnpqrstvwxz"
    	c2 = "zxtsrqpnmlkgfdcb"
	edx = 0
    	eax = bl
	edi = 0x13
    	edx = eax%edi
    	bl += 1
	edi = 5
    	al = c1[edx]
	buf += al
    	eax = bl
	edx = 0
    	edx = eax%edi
	bl += 1
    	al = v2[edx]
	buf += al
	eax = 2
	
	if ord(al) == 0x65 :
        
            if bl & 0x07 :
                eax = bl
                edi = 3
                edx = 0
                edx = eax%edi
                al = v1[edx]
                buf += al
            '''
            else :
                if bl != 1 :
                    bl += 1
                    eax = bl
                    edi = 0x0F
                    edx = 0
                    edx = eax%edi
                    al = c2[edx]
                    print "\t[2]edx : %x al : %s bl : %x" % (edx, al, bl)
                    buf += al
            '''
        else :
            if (bl & 1) :
                bl += 1
                eax = bl
                edi = 0x0F
                edx = 0
                edx = eax%edi
                al = c2[edx]
                buf += al
        bl += 1
                    
    return buf

def dga(salt, date):
    domains = []
    seed = generateSeed(date.year, date.month, date.day)
    seed = generateString(salt, seed)#.decode("hex")
    for _ in range(0x1E):
        hashit = hasher(seed)
        domain = generateDomain(hashit.decode("hex"), 0x0A)

        s1 = "%08x"
        s2 = int(hashit[:8],16)
        s3 = 0x01000000
        s4 = (s2+s3)
        s5 = s1 % s4
        if (len(s5) % 2 != 0):       # IDK why, it's odd length string from time to time
            s5 = '0' + s5            # So when this happen, we add a leading 0 that won't change the number
        s6 = s5.decode("hex")

        seed = s6

        domain = domain + random.choice([".kz",".com"])

        domains.append(domain)

        s1 = None
        s2 = None
        s3 = None
        s4 = None
        s5 = None
        s6 = None
    return domains

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
    
    directory = "data/pushdo/list/"
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

        date = datetime.strptime(randomDate("1/1/1970 01:00 AM", "1/1/3000 1:10 AM", random.random()), "%m/%d/%Y %I:%M %p")

        for domain in dga(0, date):
            
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