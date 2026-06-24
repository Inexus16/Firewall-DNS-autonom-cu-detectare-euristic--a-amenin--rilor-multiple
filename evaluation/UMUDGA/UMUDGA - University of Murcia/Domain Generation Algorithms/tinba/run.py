from __future__ import print_function


def dga(dga_config):
    dga_config['domain_l'] = [ord(l) for l in dga_config['domain']]
    dga_config['seed_sum'] = sum(dga_config['seed_l'][:16])
    new_domain = []
    tmp = dga_config['seed_l'][15] & 0xFF
    for i in range(12):
        while True:
            tmp += dga_config['domain_l'][i]
            tmp ^= (dga_config['seed_sum'] & 0xFF)
            tmp += dga_config['domain_l'][i+1]
            tmp &= 0xFF
            if 0x61 < tmp < 0x7a:
                new_domain.append(tmp)
                break
            else:
                dga_config['seed_sum'] += 1

    base_domain = ''.join([chr(x) for x in new_domain])
    #domains = []
    #for tld in dga_config['tlds']:
    #    dga_config['domain'] = base_domain + '.' + tld
    #    domains.append(dga_config['domain'])
    #return domains
    domain = base_domain + '.' + random.choice(dga_config['tlds'])
    dga_config['domain'] = domain
    return domain


dga_configurations = {
    0: { # http://garage4hackers.com/entry.php?b=3086
        'seed': 'oGkS3w3sGGOGG7oc', 
        'domain': 'ssrgwnrmgrxe.com',
        'tlds': ['com']
    },
    1: { # https://johannesbader.ch/2015/04/new-top-level-domains-for-tinbas-dga
        'seed': 'jc74FlUna852Ji9o', 
        'domain': 'blackfreeqazyio.cc',
        'tlds': ['com', 'net', 'in', 'ru']
    },
    2: { # https://www.sophos.com/en-us/threat-center/threat-analyses/viruses-and-spyware/Troj~Tinba-EL/detailed-analysis.aspx
         # https://github.com/baderj/domain_generation_algorithms/commit/c7d154a39bb172c4632f7565e0c9380e8b36c18e
        'seed': 'yqokqFC2TPBFfJcG', 
        'domain': 'watchthisnow.xyz',
        'tlds': ['pw', 'us', 'xyz', 'club']
    },
    3: {# https://github.com/baderj/domain_generation_algorithms/commit/c7d154a39bb172c4632f7565e0c9380e8b36c18e
        'seed': 'j193HsnW72Yqns7u', 
        'domain': 'j193hsne720uie8i.cc',
        'tlds': ['com', 'net', 'biz', 'org']
    }
}
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
    
    directory = "data/tinba/list/"
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

        config = random.choice(dga_configurations)
        config['seed'] += (17 - len(config['seed']))*'\x00'
        config['seed_l'] = [ord(s) for s in config['seed']]

        #for domain in dga(config):
        domain = dga(config)
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