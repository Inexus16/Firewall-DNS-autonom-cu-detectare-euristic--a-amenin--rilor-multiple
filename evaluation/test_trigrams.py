import math
sum_log = []
with open('benign_10k_sample.txt') as f:
    for line in f:
        sld = line.strip().split('.')[0].lower()
        letters = [c for c in sld if 'a' <= c <= 'z']
        if len(letters) >= 3:
            trigs = [''.join(letters[i:i+3]) for i in range(len(letters)-2)]
            logs = [trigram_model.get(t, MIN_LOGPROB) for t in trigs]
            avg = sum(logs) / len(logs)
            sum_log.append(avg)
print(min(sum_log), max(sum_log), statistics.mean(sum_log))
