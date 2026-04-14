# build GUI here
from tkinter import *
from tkinter import ttk, messagebox
import time


class GroundStationGUI:
    def __init__(self, root, command_callback, change_port_callback=None):
        self.root = root
        self.command_callback = command_callback
        self.change_port_callback = change_port_callback
        self.drones = {}
        self.selected_drone_id = None
        self.drone_widgets = {}
        self.heartbeat_counts = {}

        self.root.title("Drone Mesh Ground Control Station")
        self.root.geometry("900x650")

        self.setup_ui()

    def setup_ui(self):
        main_frame = Frame(self.root)
        main_frame.pack(fill=BOTH, expand=True, padx=10, pady=10)

        # Left panel - Drone list
        left_panel = Frame(main_frame, width=220)
        left_panel.pack(side=LEFT, fill=BOTH, padx=(0, 10))

        Label(left_panel, text="Active Drones", font=("Arial", 12, "bold")).pack()

        self.drone_list_frame = Frame(left_panel)
        self.drone_list_frame.pack(fill=BOTH, expand=True, pady=10)

        Button(left_panel, text="Remove Selected", bg="#f44336", fg="white",
               command=self.remove_selected_drone).pack(fill=X)

        # Right panel
        right_panel = Frame(main_frame)
        right_panel.pack(side=RIGHT, fill=BOTH, expand=True)

        # Telemetry display
        telem_frame = LabelFrame(right_panel, text="Telemetry", font=("Arial", 11, "bold"))
        telem_frame.pack(fill=BOTH, expand=True, pady=(0, 10))

        self.telem_labels = {}
        fields = [
            ('Latitude',   '°'),
            ('Longitude',  '°'),
            ('Altitude',   'm'),
            ('Roll',       '°'),
            ('Pitch',      '°'),
            ('Yaw',        '°'),
            ('Battery',    'V'),
            ('Satellites', ''),
            ('Armed',      ''),
            ('Video',      ''),
            ('Heartbeats', ''),
        ]

        for field, unit in fields:
            row_frame = Frame(telem_frame)
            row_frame.pack(fill=X, padx=10, pady=2)
            Label(row_frame, text=f"{field}:", width=12, anchor=W).pack(side=LEFT)
            value_label = Label(row_frame, text="--", width=15, anchor=W, font=("Courier", 10))
            value_label.pack(side=LEFT)
            Label(row_frame, text=unit, width=5, anchor=W).pack(side=LEFT)
            self.telem_labels[field] = value_label

        # Command panel
        cmd_frame = LabelFrame(right_panel, text="Commands", font=("Arial", 11, "bold"))
        cmd_frame.pack(fill=X, pady=(0, 10))

        btn_frame = Frame(cmd_frame)
        btn_frame.pack(padx=10, pady=10)

        Button(btn_frame, text="ARM", width=12, bg="#4CAF50", fg="white",
               command=self.cmd_arm).pack(side=LEFT, padx=5)
        Button(btn_frame, text="DISARM", width=12, bg="#f44336", fg="white",
               command=self.cmd_disarm).pack(side=LEFT, padx=5)
        Button(btn_frame, text="Request Image", width=12, bg="#2196F3", fg="white",
               command=self.cmd_request_image).pack(side=LEFT, padx=5)

        wp_frame = Frame(cmd_frame)
        wp_frame.pack(padx=10, pady=(0, 10))

        Label(wp_frame, text="Waypoint:").pack(side=LEFT)
        self.wp_lat = Entry(wp_frame, width=10)
        self.wp_lat.pack(side=LEFT, padx=2)
        self.wp_lat.insert(0, "lat")
        self.wp_lon = Entry(wp_frame, width=10)
        self.wp_lon.pack(side=LEFT, padx=2)
        self.wp_lon.insert(0, "lon")
        self.wp_alt = Entry(wp_frame, width=6)
        self.wp_alt.pack(side=LEFT, padx=2)
        self.wp_alt.insert(0, "alt")
        Button(wp_frame, text="Send Waypoint", bg="#FF9800", fg="white",
               command=self.cmd_waypoint).pack(side=LEFT, padx=5)

        # Bottom bar
        bottom_frame = Frame(self.root)
        bottom_frame.pack(side=BOTTOM, fill=X)

        self.status_label = Label(bottom_frame, text="Ready", bd=1, relief=SUNKEN, anchor=W)
        self.status_label.pack(side=LEFT, fill=X, expand=True)

        Button(bottom_frame, text="Change Port", command=self.on_change_port).pack(
            side=RIGHT, padx=5, pady=2)

    # ------------------------------------------------------------------
    #  Drone list management
    # ------------------------------------------------------------------

    def _add_drone_widget(self, drone_id):
        row = Frame(self.drone_list_frame, relief=GROOVE, bd=1)
        row.pack(fill=X, pady=2, padx=2)

        indicator = Label(row, text="●", fg="green", font=("Arial", 14))
        indicator.pack(side=LEFT, padx=4)

        label = Label(row, text=f"Drone {drone_id}", font=("Courier", 10), cursor="hand2")
        label.pack(side=LEFT, padx=4)

        for widget in (row, label, indicator):
            widget.bind("<Button-1>", lambda e, did=drone_id: self.select_drone(did))

        self.drone_widgets[drone_id] = {
            'frame':     row,
            'indicator': indicator,
            'label':     label,
        }
        self.heartbeat_counts[drone_id] = 0

    def select_drone(self, drone_id):
        bg_default = self.drone_list_frame.cget('bg')
        bg_selected = "#cce5ff"

        for did, w in self.drone_widgets.items():
            w['frame'].config(bg=bg_default)
            w['label'].config(bg=bg_default)
            w['indicator'].config(bg=bg_default)

        if drone_id in self.drone_widgets:
            self.drone_widgets[drone_id]['frame'].config(bg=bg_selected)
            self.drone_widgets[drone_id]['label'].config(bg=bg_selected)
            self.drone_widgets[drone_id]['indicator'].config(bg=bg_selected)

        self.selected_drone_id = drone_id
        if drone_id in self.drones:
            self._update_telemetry_display(self.drones[drone_id])

    def update_drone_indicator(self, drone_id, connected):
        if drone_id in self.drone_widgets:
            self.drone_widgets[drone_id]['indicator'].config(
                fg="green" if connected else "red"
            )

    def remove_selected_drone(self):
        if not self.selected_drone_id:
            messagebox.showwarning("No Drone Selected", "Please select a drone first.")
            return
        self.remove_drone(self.selected_drone_id)

    def remove_drone(self, drone_id):
        if drone_id not in self.drones:
            return
        if drone_id in self.drone_widgets:
            self.drone_widgets[drone_id]['frame'].destroy()
            del self.drone_widgets[drone_id]
        if drone_id in self.heartbeat_counts:
            del self.heartbeat_counts[drone_id]
        del self.drones[drone_id]
        if self.selected_drone_id == drone_id:
            self.selected_drone_id = None
            for label in self.telem_labels.values():
                label.config(text="--", fg="black")
        self.status_label.config(
            text=f"Drone {drone_id} removed | {time.strftime('%H:%M:%S')} | Drones: {len(self.drones)}"
        )

    # ------------------------------------------------------------------
    #  Data updates
    # ------------------------------------------------------------------

    def update_drone(self, drone_data):
        drone_id = drone_data['id']

        if drone_data.get('type') == 'flag':
            self._show_flag_alert(drone_data)
            return

        if drone_data.get('type') == 'photo_done':
            self.status_label.config(
                text=f"Photo transfer complete from Drone {drone_id} | {time.strftime('%H:%M:%S')}"
            )
            return

        if drone_id not in self.drones:
            self._add_drone_widget(drone_id)

        self.drones[drone_id] = drone_data
        self.drones[drone_id]['last_seen'] = time.time()

        # increment heartbeat counter
        self.heartbeat_counts[drone_id] = self.heartbeat_counts.get(drone_id, 0) + 1
        count = self.heartbeat_counts[drone_id]

        # update heartbeat in telemetry panel if this drone is selected
        if self.selected_drone_id == drone_id and 'Heartbeats' in self.telem_labels:
            self.telem_labels['Heartbeats'].config(text=str(count))

        self.update_drone_indicator(drone_id, True)

        if self.selected_drone_id == drone_id:
            self._update_telemetry_display(drone_data)

        self.status_label.config(
            text=f"Last update: {time.strftime('%H:%M:%S')} | Drones: {len(self.drones)}"
        )

    def _show_flag_alert(self, data):
        msg = (f"PERSON DETECTED\n\n"
               f"Drone:      {data['id']}\n"
               f"Confidence: {data['confidence']:.0%}\n"
               f"Lat:        {data['lat']:.5f}\n"
               f"Lon:        {data['lon']:.5f}\n"
               f"Alt:        {data['alt']:.1f}m")
        messagebox.showwarning("⚠ Person Detected!", msg)

    def _update_telemetry_display(self, data):
        if data.get('type') == 'telemetry':
            self.telem_labels['Latitude'].config(text=f"{data['lat']:.6f}")
            self.telem_labels['Longitude'].config(text=f"{data['lon']:.6f}")
            self.telem_labels['Altitude'].config(text=f"{data['alt']:.1f}")
            self.telem_labels['Roll'].config(text=f"{data['roll']:.1f}")
            self.telem_labels['Pitch'].config(text=f"{data['pitch']:.1f}")
            self.telem_labels['Yaw'].config(text=f"{data['yaw']:.1f}")
            self.telem_labels['Battery'].config(text=f"{data['battery']:.2f}")
            self.telem_labels['Satellites'].config(text=str(data['satellites']))
            self.telem_labels['Armed'].config(
                text="YES" if data['armed'] else "NO",
                fg="green" if data['armed'] else "red"
            )
            self.telem_labels['Video'].config(
                text="STREAMING" if data.get('streaming') else "OFF",
                fg="blue" if data.get('streaming') else "gray"
            )

    # ------------------------------------------------------------------
    #  Commands
    # ------------------------------------------------------------------

    def on_change_port(self):
        if self.change_port_callback:
            self.change_port_callback()

    def cmd_arm(self):
        if self.selected_drone_id:
            self.command_callback('arm', self.selected_drone_id)
        else:
            messagebox.showwarning("No Drone Selected", "Please select a drone first.")

    def cmd_disarm(self):
        if self.selected_drone_id:
            self.command_callback('disarm', self.selected_drone_id)
        else:
            messagebox.showwarning("No Drone Selected", "Please select a drone first.")

    def cmd_request_image(self):
        if self.selected_drone_id:
            self.command_callback('request_image', self.selected_drone_id)
        else:
            messagebox.showwarning("No Drone Selected", "Please select a drone first.")

    def cmd_waypoint(self):
        if not self.selected_drone_id:
            messagebox.showwarning("No Drone Selected", "Please select a drone first.")
            return
        try:
            lat = float(self.wp_lat.get())
            lon = float(self.wp_lon.get())
            alt = float(self.wp_alt.get())
            self.command_callback('waypoint', self.selected_drone_id, lat, lon, alt)
        except ValueError:
            messagebox.showerror("Invalid Input", "Please enter valid numeric coordinates.")