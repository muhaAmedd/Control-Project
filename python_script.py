import requests
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.widgets import Button

# ---------------- Config ----------------
API_KEY = "AXD57OBVPCKTLG00"  # replace with your ThingSpeak API key
CHANNEL_ID = "3041457"

# Field numbers (adjust as needed)
FIELD_TEMP = 1  # e.g., field1 for temperature
FIELD_STATE = 2  # e.g., field2 for state


# -------- Fetch Functions --------
def fetch_field_data(field_num, label):
    url = f"https://api.thingspeak.com/channels/{CHANNEL_ID}/fields/{field_num}.json?api_key={API_KEY}&results=50"
    try:
        r = requests.get(url)
        r.raise_for_status()
        feeds = r.json().get("feeds", [])
        if not feeds:
            print(f"No {label} data available.")
            return pd.DataFrame()

        data = pd.DataFrame(feeds)
        data["created_at"] = pd.to_datetime(data["created_at"])
        data[label] = pd.to_numeric(data[f"field{field_num}"], errors="coerce")
        return data[["created_at", label]].dropna()

    except Exception as e:
        print(f"Error fetching {label}: {e}")
        return pd.DataFrame()


# -------- Plot Function --------
def plot_data():
    global ax_temp, ax_state, fig

    # Fetch fresh data
    temp_data = fetch_field_data(FIELD_TEMP, "Temperature")
    state_data = fetch_field_data(FIELD_STATE, "State")

    ax_temp.clear()
    ax_state.clear()

    if not temp_data.empty:
        ax_temp.plot(
            temp_data["created_at"],
            temp_data["Temperature"],
            color="red",
            marker="o",
            label="Temperature (Â°C)",
        )
        ax_temp.set_ylabel("Temperature (Â°C)")
        ax_temp.legend()
        ax_temp.grid(True)

    if not state_data.empty:
        ax_state.step(
            state_data["created_at"], state_data["State"], color="blue", label="State"
        )
        ax_state.set_ylabel("State")
        ax_state.legend()
        ax_state.grid(True)

    plt.draw()


# -------- Button Callback --------
def refresh(event):
    print("Refreshing data...")
    plot_data()


# -------- Main --------
fig, (ax_temp, ax_state) = plt.subplots(2, 1, figsize=(10, 6), sharex=True)

# Place a refresh button
ax_button = plt.axes([0.81, 0.02, 0.1, 0.05])  # x, y, width, height
btn = Button(ax_button, "Refresh")
btn.on_clicked(refresh)

plot_data()
plt.xlabel("Time")
plt.suptitle("ThingSpeak Data Viewer", fontsize=14)
plt.tight_layout(rect=[0, 0.05, 1, 0.95])
plt.show()
