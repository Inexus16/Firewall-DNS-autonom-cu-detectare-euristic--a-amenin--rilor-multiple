#!/bin/bash
# Table 5.5 – Wilson confidence intervals (95%), threshold 0.48

python3 - <<EOF
import math

def wilson_interval(successes, trials, z=1.96):
    p = successes / trials
    denom = 1 + z**2 / trials
    centre = (p + z**2 / (2*trials)) / denom
    margin = z * math.sqrt(p*(1-p)/trials + z**2/(4*trials**2)) / denom
    return max(0, centre - margin), min(1, centre + margin)

# FPR: 112 false positives, 10000 benign (measured at threshold 0.48)
fpr_low, fpr_high = wilson_interval(112, 10000)
print(f"FPR: 1.12%  (95% CI: {fpr_low*100:.2f}% - {fpr_high*100:.2f}%)")

# DR: 2277 true positives, 5000 DGA (measured at threshold 0.48)
dr_low, dr_high = wilson_interval(2277, 5000)
print(f"DR: 45.54% (95% CI: {dr_low*100:.2f}% - {dr_high*100:.2f}%)")
EOF
