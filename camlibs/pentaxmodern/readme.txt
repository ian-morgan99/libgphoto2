📷 Pentax K-Series Enhanced Driver for libgphoto2
This custom driver extends libgphoto2 to provide full remote control and image acquisition for modern Pentax DSLR and mirrorless cameras including:

Pentax K-3 III

Pentax K-1 II

Pentax K-70

✅ Key Features

Feature Area	Capabilities
🎥 Live View	Start/Stop Live View, Capture Live View Preview, Zoom (1x–8x)
📸 Image Capture	Standard and Bulb capture with programmable exposure time
🌓 Astrotracer Support	Enable/Disable + configurable timer (up to 5 minutes)
🧠 Exposure Controls	ISO, Shutter, Aperture, Exposure Compensation, Drive Mode
🎚 White Balance & Metering	Full white balance table + metering modes
💾 Dual SD Card Support	Active card selection, write mode (Sequential, Parallel, RAW+JPEG Split)
🧱 Image Parameters	Quality, Image Tone, CI Mode, Scene Mode, Exposure Mode
🎯 Focus Control	AF-S, AF-C, Manual, Touch AF, Face Detection, Peaking, Fine Focus Tuning
🔭 Advanced Modes	HDR (Auto, HDR1–3), Bracketing (steps), Pixel Shift + Motion Correction
🔧 Hardware Controls	LED toggle, Mirror Lock for Cleaning, GPS Sync
🔋 Battery Monitoring	Battery level shown as percentage
📂 File Management	List, download, and delete files filtered by active SD card
🗂️ UI Sections & Controls

Section	Controls
Astrotracer	Enable toggle, Time limit slider
Live View	Enable toggle, Interval slider
Capture Modes	Bulb Mode toggle
Exposure	ISO, Shutter, Aperture, Exposure Comp., Drive, HDR, Bracketing, Scene, Image Tone, CI Mode, Exp Mode
Focus & Autofocus	Focus Mode, Peaking, Face Detection, Live View Zoom, Fine Tune buttons, Zoom Assist Level
Storage Settings	Active SD Card (1/2), Write Mode
Hardware Controls	LED Toggle, GPS Sync, Mirror Clean Trigger
Battery Status	Text display of percentage
🔁 Supported Commands (PTP)
The driver implements dozens of custom PTP operations including:

Image Controls: GET/SET_ISO, GET/SET_SHUTTER, GET/SET_APERTURE, GET/SET_WHITE_BALANCE, etc.

Live View: START_LIVE_VIEW, GET_LV_FRAME, END_LIVE_VIEW, SET_ZOOM

Focus: TRIGGER_AF, GET/SET_FOCUS_MODE, FINE_TUNE_FOCUS

Advanced Modes: GET/SET_HDR_MODE, IMAGE_SHIFT, BRACKETING_MODE

System: LED_ON, GPS_SYNC, MIRROR_LOCK, BATTERY_LEVEL

All mapped through ptp_generic_no_data() or ptp_generic_get_uint32() for cross-compatibility.

🧪 Camera Compatibility
Tested models:

✅ K-3 III

✅ K-1 II

✅ K-70

Expected to work on most PENTAX bodies that support USB tethering and Image Transmitter 2.

🛠️ Developer Notes
Uses PentaxLookupEntry[] tables for clean translation between strings and numeric camera values.

Supports dynamic camera_get_config() and camera_set_config() building with safety.

Live View, Dual Card Storage, and Advanced Scene Modes are fully interactive.

Focus modes match capabilities documented in the reverse-engineered Image Transmitter 2 codebase.

🧰 Future Enhancements (Optional)
Add custom color profiles or film simulation modes
Integrate video recording (if supported by firmware)
Auto-import captured images into host gallery
Live View focus point adjustment
Bulb timer/long exposure automation
Focus assist (e.g. live view magnification)
Dual-card control
Exposure bracketing/drive mode selection
On-host playback image browsing
