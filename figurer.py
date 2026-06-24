import matplotlib.pyplot as plt

# Pragurile selectate
thresholds = [0.10, 0.20, 0.30, 0.40, 0.43, 0.45, 0.48, 0.50, 0.60, 0.70, 0.80, 0.90]

# Datele corespunzătoare
fpr = [86.23, 84.84, 74.22, 16.26, 6.54, 3.43, 1.12, 0.61, 0.05, 0.00, 0.00, 0.00]
dr  = [99.78, 99.78, 99.60, 89.06, 73.86, 63.62, 45.54, 32.26, 8.90, 1.16, 0.00, 0.00]

plt.figure(figsize=(11, 6))  # puțin mai lat pentru a acomoda etichetele rotite

# Curbe
plt.plot(thresholds, fpr, 'o-', label='False Positive Rate (FPR)', color='red', linewidth=2, markersize=8)
plt.plot(thresholds, dr, 's-', label='Detection Rate (DR)', color='green', linewidth=2, markersize=8)

# Linie verticală la pragul recomandat
plt.axvline(x=0.48, color='blue', linestyle='--', linewidth=2, label='Operating threshold (0.48)')

# Etichete și titlu
plt.xlabel('DGA Threshold', fontsize=12)
plt.ylabel('Rate (%)', fontsize=12)
plt.title('DGA Detection Threshold Sweep (Improved 4-gram Scorer)', fontsize=14)
plt.legend(fontsize=11)
plt.grid(True, linestyle='--', alpha=0.6)

# ROTIREA ETICHETELOR DE PE AXA X – aceasta rezolvă suprapunerea
plt.xticks(thresholds, rotation=45, ha='right', fontsize=10)

plt.xlim(0.0, 0.95)
plt.ylim(0, 105)
plt.tight_layout()  # ajustează marginile automat

# Salvare
plt.savefig('threshold_sweep_selected.pdf', bbox_inches='tight')
plt.savefig('threshold_sweep_selected.png', bbox_inches='tight', dpi=300)

print("✅ Graficul salvat cu etichete lizibile.")
plt.show()
