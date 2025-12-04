import re
import matplotlib.pyplot as plt

# ---- CONFIG ----
FILEPATH = "../../serialmonitor.log"   # metti il nome del file qui
SAMPLE_RATE = 50                     # Hz
# ----------------

# Legge il file
with open(FILEPATH, "r") as f:
    data_text = f.read()

# Estrae tutti gli IR_RAW
raw_vals = [int(m) for m in re.findall(r'IR_RAW:\s*(-?\d+)', data_text)]

# Crea l’asse temporale
dt = 1 / SAMPLE_RATE
t = [i * dt for i in range(len(raw_vals))]

# Plot
plt.figure(figsize=(12,4))
plt.plot(t, raw_vals)
plt.xlabel("Time (s)")
plt.ylabel("IR_RAW")
plt.title(f"PPG RAW Signal at {SAMPLE_RATE} SPS")
plt.grid(True)
plt.tight_layout()
plt.show()