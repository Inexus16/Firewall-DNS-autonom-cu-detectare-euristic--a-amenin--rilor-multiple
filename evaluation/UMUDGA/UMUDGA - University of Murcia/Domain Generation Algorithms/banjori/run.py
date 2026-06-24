def map_to_lowercase_letter(s):
    return ord('a') + ((s - ord('a')) % 26)

def next_domain(domain):
    dl = [ord(x) for x in list(domain)]
    dl[0] = map_to_lowercase_letter(dl[0] + dl[3])
    dl[1] = map_to_lowercase_letter(dl[0] + 2*dl[1])
    dl[2] = map_to_lowercase_letter(dl[0] + dl[2] - 1)
    dl[3] = map_to_lowercase_letter(dl[1] + dl[2] + dl[3])
    return ''.join([chr(x) for x in dl])


seeds = [
    # Seeds from https://blog.kleissner.org/?p=69
    "xxxxedsafe.com",
    "xxxxhuxmiax.com",
    "xxxxierihon.com",
    "xxxxtomvader.com",
    "xxxxednog.com",
    "xxxxapontis.com",
    "xxxxefnomosk.com",
    "xxxxdminmont.com",
    "xxxxesroater.com",
    "xxxxggelds.com",
    "xxxxmobama.com",
    # Seeds from https://www.johannesbader.ch/2015/02/the-dga-of-banjori/
    "antisemitismgavenuteq.com",
    "bandepictom.com",
    "buckbyplaywobb.com",
    "telemachuslazaroqok.com",
    "texanfoulilp.com",
    "clearasildeafeninguvuc.com",
    "marisagabardinedazyx.com",
    "pickfordlinnetavox.com",
    "snapplefrostbitecycz.com",
    "filtererwyatanb.com",
    "antwancorml.com",
    "stravinskycattederifg.com",
    "forepartbulkyf.com",
    "fundamentalistfanchonut.com",
    "criterionirkutskagl.com",
    "criminalcentricem.com",
    "babysatformalisticirekb.com",
    "earnestnessbiophysicalohax.com",
    # Seeds from https://blog.kleissner.org/?p=192
    "displeasuredehydratorysagp.com",
    "antisemitismgavenuteq.com",
    "cantorcajanunal.com",
    "andersensinaix.com",
    "formidablyhoosieraw.com",
    "gbpsenhancedysb.com",
    "genialitydevonianizuwb.com",
    "gerhardenslavetusul.com",
    "kennanerraticallyqozaw.com",
    "rozellaabettingk.com",
    "doniellefrictionlessv.com",
    "anshanarianaqh.com",
    "buckbyplaywobb.com"
]

import sys
import random

if __name__=="__main__":
    
    directory = "data/banjori/list/"
    intseed = 521496385

    import os
    if not os.path.exists(directory):
        os.makedirs(directory)


    random.seed(intseed)
    
    counter = 0
    collision = 0

    data = set()

    f1000 = open(directory + "1000.txt","w")
    f5000 = open(directory + "5000.txt","w")
    f10000 = open(directory + "10000.txt","w")
    f50000 = open(directory + "50000.txt","w")
    f100000 = open(directory + "100000.txt","w")
    f500000 = open(directory + "500000.txt","w")

    stop = False

    domain = random.choice(seeds)

    while not stop:

        # Call the DGA
        domain = next_domain(domain)
        
        datasize = len(data)
        data.add(domain)
        
        # If it's a collision ignore it.
        if len(data) == datasize:
            collision = collision +1
            if collision % 100 == 0:
                domain = random.choice(seeds)
                collision = 0

            continue

        counter = counter + 1

        if len(data) <= 1000:
			f1000.write("%s\n" % domain)
			f5000.write("%s\n" % domain)
			f10000.write("%s\n" % domain)
			f50000.write("%s\n" % domain)
			f100000.write("%s\n" % domain)
			f500000.write("%s\n" % domain)
        elif len(data) <= 5000:
			f5000.write("%s\n" % domain)
			f10000.write("%s\n" % domain)
			f50000.write("%s\n" % domain)
			f100000.write("%s\n" % domain)
			f500000.write("%s\n" % domain)
        elif len(data) <= 10000:
			f10000.write("%s\n" % domain)
			f50000.write("%s\n" % domain)
			f100000.write("%s\n" % domain)
			f500000.write("%s\n" % domain)
        elif len(data) <= 50000:
            f50000.write("%s\n" % domain)
            f100000.write("%s\n" % domain)
            f500000.write("%s\n" % domain)
        elif len(data) <= 100000:
            f100000.write("%s\n" % domain)
            f500000.write("%s\n" % domain)
        elif len(data) <= 500000:
            f500000.write("%s\n" % domain)
        else:
            f1000.close()
            f5000.close()
            f10000.close()
            f50000.close()
            f100000.close()
            f500000.close()
            stop = True
            break
    
    # End While
#End Program