#build GUI here
from tkinter import *
from tkinter import ttk
import time

class GroundStationGUI:
    def __init__(self, root, command_callback):
        self.root = root
        self.command_callback = command_callback
        self.drones = {}
        self.selected_drone_id = None
        
        self.root.title("Drone Mesh Ground Control Station")
        self.root.geometry("800x600")
        
        self.setup_ui()
    
    def setup_ui(self):
        # Main container
        main_frame = Frame(self.root)
        main_frame.pack(fill=BOTH, expand=True, padx=10, pady=10)
        
        # Left panel - Drone list
        left_panel = Frame(main_frame, width=200)
        left_panel.pack(side=LEFT, fill=BOTH, padx=(0, 10))
        
        Label(left_panel, text="Active Drones", font=("Arial", 12, "bold")).pack()
        
        self.drone_listbox = Listbox(left_panel, font=("Courier", 10))
        self.drone_listbox.pack(fill=BOTH, expand=True, pady=10)
        self.drone_listbox.bind('<<ListboxSelect>>', self.on_drone_select)
        
        # Right panel - Telemetry and controls
        right_panel = Frame(main_frame)
        right_panel.pack(side=RIGHT, fill=BOTH, expand=True)
        
        # Telemetry display
        telem_frame = LabelFrame(right_panel, text="Telemetry", font=("Arial", 11, "bold"))
        telem_frame.pack(fill=BOTH, expand=True, pady=(0, 10))
        
        self.telem_labels = {}
        fields = [
            ('Latitude', '°'),
            ('Longitude', '°'),
            ('Altitude', 'm'),
            ('Roll', '°'),
            ('Pitch', '°'),
            ('Yaw', '°'),
            ('Battery', 'V'),
            ('Satellites', ''),
            ('Armed', ''),
            ('Video', '')
        ]
        
        for i, (field, unit) in enumerate(fields):
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
        Button(btn_frame, text="Start Video", width=12, bg="#2196F3", fg="white",
               command=self.cmd_start_video).pack(side=LEFT, padx=5)
        Button(btn_frame, text="Stop Video", width=12,
               command=self.cmd_stop_video).pack(side=LEFT, padx=5)
        
        # Status bar
        self.status_label = Label(self.root, text="Ready", bd=1, relief=SUNKEN, anchor=W)
        self.status_label.pack(side=BOTTOM, fill=X)
    
    def update_drone(self, drone_data):
        """Update drone data and GUI"""
        drone_id = drone_data['id']
        
        # Add to list if new
        if drone_id not in self.drones:
            self.drone_listbox.insert(END, f"Drone {drone_id}")
        
        self.drones[drone_id] = drone_data
        self.drones[drone_id]['last_seen'] = time.time()
        
        # Update display if this drone is selected
        if self.selected_drone_id == drone_id:
            self._update_telemetry_display(drone_data)
        
        self.status_label.config(text=f"Last update: {time.strftime('%H:%M:%S')} | Drones: {len(self.drones)}")
    
    def _update_telemetry_display(self, data):
        """Update telemetry labels with drone data"""
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
    
    def on_drone_select(self, event):
        """Handle drone selection from list"""
        selection = self.drone_listbox.curselection()
        if selection:
            drone_text = self.drone_listbox.get(selection[0])
            self.selected_drone_id = int(drone_text.split()[1])
            
            # Update display
            if self.selected_drone_id in self.drones:
                self._update_telemetry_display(self.drones[self.selected_drone_id])
    
    def cmd_arm(self):
        if self.selected_drone_id:
            self.command_callback('arm', self.selected_drone_id)
    
    def cmd_disarm(self):
        if self.selected_drone_id:
            self.command_callback('disarm', self.selected_drone_id)
    
    def cmd_start_video(self):
        if self.selected_drone_id:
            self.command_callback('start_video', self.selected_drone_id)
    
    def cmd_stop_video(self):
        if self.selected_drone_id:
            self.command_callback('stop_video', self.selected_drone_id)