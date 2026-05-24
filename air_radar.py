import serial
import time
import matplotlib.pyplot as plt
import numpy as np

# --- CONFIG ---
SERIAL_PORT = 'COM6'
BAUD_RATE = 9600
MAX_RANGE = 40    # Focus on high-detail 40cm scan
FADE_TIME = 1.2   # How long detections stay on screen

plt.style.use('dark_background')
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, polar=True)

ax.set_thetamin(0)
ax.set_thetamax(180)
ax.set_theta_zero_location('E')
ax.set_theta_direction(1)
ax.set_ylim(0, MAX_RANGE)
ax.set_yticks([10, 20, 30, 40])
ax.set_yticklabels(['10cm', '20cm', '30cm', '40cm'], color='lime', size=8)
ax.set_facecolor('#000b00')
ax.grid(color='green', linestyle='--', alpha=0.4)

line, = ax.plot([], [], color='#00FF00', linewidth=3, alpha=0.6, zorder=3)
detections = []
scatter = ax.scatter([], [], c=[], cmap='Greens', s=80, edgecolors='none', zorder=2, vmin=0, vmax=1)

def update_radar():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        ser.reset_input_buffer()
        print("Professional Radar Active...")

        while True:
            current_time = time.time()
            if ser.in_waiting > 0:
                raw = ser.readline().decode('utf-8', errors='ignore').strip()
                if ',' in raw:
                    try:
                        angle, dist = map(float, raw.split(',')[:2])
                        theta = np.deg2rad(max(0, min(180, angle)))
                        line.set_data([theta, theta], [0, MAX_RANGE])

                        if 2 < dist <= MAX_RANGE:
                            detections.append([theta, dist, current_time])
                    except ValueError: continue

            # Remove old detections
            detections[:] = [d for d in detections if (current_time - d[2]) < FADE_TIME]

            if detections:
                d_thetas = [d[0] for d in detections]
                d_dists = [d[1] for d in detections]
                d_ages = [max(0.1, 1.0 - (current_time - d[2]) / FADE_TIME) for d in detections]
                scatter.set_offsets(np.c_[d_thetas, d_dists])
                scatter.set_array(np.array(d_ages))
            else:
                scatter.set_offsets(np.empty((0, 2)))

            plt.pause(0.005)

    except Exception as e: print(f"Error: {e}")
    finally: ser.close()

if __name__ == "__main__":
    update_radar()