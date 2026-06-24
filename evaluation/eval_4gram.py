import math, struct

# Load the 4-gram model same file used in firewall
def load_model(path):
    table = {}
    with open(path, "rb") as f:
        count = struct.unpack("<I", f.read(4))[0]
        for _ in range(count):
            gram = f.read(4).decode("ascii", errors="replace")
            logp = struct.unpack("<f", f.read(4))[0]
            table[gram] = logp
    return table

model = load_model("4grams.bin")
MIN_LOGPROB = -20.0   # default for unseen grams

def avg_logprob(domain):
    sld = domain.strip().split('.')[0].lower()
    letters = [c for c in sld if 'a' <= c <= 'z']
    if len(letters) < 4:
        return None
    grams = [''.join(letters[i:i+4]) for i in range(len(letters)-3)]
    logs = [model.get(g, MIN_LOGPROB) for g in grams]
    return sum(logs) / len(logs)

# Load your benign 10k file
with open("benign_10k_sample.txt") as f:
    domains = [line.strip() for line in f if line.strip()]

avgs = []
for d in domains:
    avg = avg_logprob(d)
    if avg is not None:
        avgs.append(avg)

avgs.sort()
n = len(avgs)
lo = avgs[int(n * 0.01)]   # 1st percentile
hi = avgs[int(n * 0.99)]   # 99th percentile
print(f"Recommended LOG_LO: {lo:.2f}")
print(f"Recommended LOG_HI: {hi:.2f}")
