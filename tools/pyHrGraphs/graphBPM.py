import re
import matplotlib.pyplot as plt

# ---- CONFIG ----
FILEPATH = "../../serialmonitor.log"
SAMPLE_PERIOD_MS = 20.0   # 50 SPS → 20 ms per sample
FIXED_SAMPLES_PER_BEAT = 128
# -----------------

# ---- PARSING LOG ----
with open(FILEPATH, "r") as f:
    text = f.read()

# Legge solo gli AVG_BPM dal log
bpm_vals = [float(v) for v in re.findall(r"AVG_BPM:\s*([0-9.]+)", text)]

# La distanza tra BPM ora è fissa:
# tempo tra battiti = 128 sample × 20 ms = 2560 ms
fixed_time_ms = FIXED_SAMPLES_PER_BEAT * SAMPLE_PERIOD_MS

# Costruisce un asse temporale uniforme
time_ms_vals = [i * fixed_time_ms for i in range(len(bpm_vals))]

# Asse X → numero di beat
x = list(range(len(bpm_vals)))

# ---- GRAFICO BPM ----
plt.figure(figsize=(13,5))
plt.plot(x, bpm_vals, marker="o", label="AVG BPM")
plt.xlabel("Beat index")
plt.ylabel("BPM")
plt.title("Andamento del BPM (distanza fissa tra beat)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

# ---- GRAFICO TEMPO ASSOLUTO ----
plt.figure(figsize=(13,5))
plt.plot(time_ms_vals, bpm_vals, marker="o", label="BPM (vs tempo)")
plt.xlabel("Tempo (ms)")
plt.ylabel("BPM")
plt.title("BPM in funzione del tempo (distanza tra beat fissa)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()