import re
import matplotlib.pyplot as plt

# ---- CONFIG ----
FILEPATH = "samples.txt"   # metti il nome del file qui
SAMPLE_RATE = 50           # Hz (50 samples per second)
# ----------------

# Legge tutto il contenuto del file
with open(FILEPATH, "r") as f:
    data_text = f.read()

# Estrae tutti gli IR_AC
ac_vals = [int(m) for m in re.findall(r'IR_AC:\s*(-?\d+)', data_text)]

# Crea l’asse temporale
dt = 1 / SAMPLE_RATE
t = [i * dt for i in range(len(ac_vals))]

# Plot
plt.figure(figsize=(12,4))
plt.plot(t, ac_vals)
plt.xlabel("Time (s)")
plt.ylabel("IR_AC")
plt.title(f"PPG AC Component at {SAMPLE_RATE} SPS")
plt.grid(True)
plt.tight_layout()
plt.show()