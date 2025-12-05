import re
import matplotlib.pyplot as plt

# ---- CONFIG ----
FILEPATH = "../../serialmonitor.log"
SAMPLE_PERIOD_MS = 20.0  # 50 SPS → ogni sample 20ms
# -----------------

# ---- PARSING LOG ----
with open(FILEPATH, "r") as f:
    text = f.read()

# Legge direttamente dal log:
#   AVG_BPM: <valore>
#   - delta: <valore>
bpm_vals   = [float(v) for v in re.findall(r"AVG_BPM:\s*([0-9.]+)", text)]
delta_vals = [int(v)   for v in re.findall(r"delta:\s*([0-9]+)", text)]

# Tempo reale tra i battiti
time_ms_vals = [d * SAMPLE_PERIOD_MS for d in delta_vals]

# Asse X → numero di beat (non tempo assoluto)
x = list(range(len(bpm_vals)))

# ---- GRAFICO BPM ----
plt.figure(figsize=(13,5))
plt.plot(x, bpm_vals, marker="o", label="AVG BPM")
plt.xlabel("Beat index")
plt.ylabel("BPM")
plt.title("Andamento del BPM letto da log")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

# ---- GRAFICO DELTA IN SAMPLE ----
plt.figure(figsize=(13,5))
plt.plot(x, delta_vals, marker="o", label="Delta (samples)")
plt.xlabel("Beat index")
plt.ylabel("Delta (samples)")
plt.title("Delta tra battiti (in samples)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

# ---- GRAFICO TEMPO TRA BATTTI (ms) ----
plt.figure(figsize=(13,5))
plt.plot(x, time_ms_vals, marker="o", label="Tempo tra battiti (ms)")
plt.xlabel("Beat index")
plt.ylabel("Tempo (ms)")
plt.title("Tempo reale tra i battiti")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()