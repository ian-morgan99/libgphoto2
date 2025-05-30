📷 Pentax K-Series Enhanced Driver for libgphoto2
This custom driver extends libgphoto2 to provide full remote control and image acquisition for modern Pentax DSLR and mirrorless cameras including:

Pentax K-3 III

Pentax K-1 II

Pentax K-70

*Note: The driver dynamically tailors the UI and available features (e.g., Live View zoom options, Drive Modes, White Balance choices, Image Format options) based on the specific Pentax/Ricoh camera model detected.*

✅ Key Features

Feature Area	Capabilities
🎥 Live View	Start/Stop Live View, Capture Live View Preview, Zoom (options are model-dependent, e.g., specific zooms for 645Z, K-3III, K-3, K-1; UI hidden for LS Models like G900SE).
📸 Image Capture	Standard and Bulb capture with programmable exposure time
🌓 Astrotracer Support	Enable/Disable + configurable timer (up to 5 minutes)
🧠 Exposure Controls	ISO, Shutter, Aperture, Exposure Compensation, Drive Mode (choices adjusted for models like 645Z, KP).
🎚 White Balance & Metering	Full white balance table + metering modes (Flash WB availability is model-dependent).
💾 Dual SD Card Support	Active card selection, write mode (Sequential, Parallel, RAW+JPEG Split)
🧱 Image Parameters	Quality (TIFF available for 645Z), Image Tone, CI Mode, Scene Mode, Exposure Mode
🎯 Focus Control	AF-S, AF-C, Manual, Touch AF, Face Detection, Peaking, Fine Focus Tuning
🔭 Advanced Modes	HDR (Auto, HDR1–3), Bracketing (steps), Pixel Shift + Motion Correction
🔧 Hardware Controls	LED toggle, Mirror Lock for Cleaning, GPS Sync (some DSLR-centric UI hidden for LS Models like G900SE).
🔋 Battery Monitoring	Battery level shown as percentage
📂 File Management	List, download, and delete files filtered by active SD card
🗂️ UI Sections & Controls
*The user interface dynamically adapts based on the connected camera model.*

Section	Controls
Astrotracer	Enable toggle, Time limit slider
Live View	Enable toggle, Interval slider (Section hidden for LS Models like G900SE)
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

Expanded Model Support (based on defined custom IDs and PTP feature checks):
✅ PENTAX 645D
✅ PENTAX K-3
✅ PENTAX 645Z
✅ PENTAX K-1
✅ PENTAX KP
✅ RICOH G900SE
✅ RICOH GR III

Expected to work on most PENTAX bodies that support USB tethering and Image Transmitter 2.

🛠️ Developer Notes
Uses PentaxLookupEntry[] tables for clean translation between strings and numeric camera values.

Supports dynamic camera_get_config() and camera_set_config() building with safety.

Live View, Dual Card Storage, and Advanced Scene Modes are fully interactive.

Focus modes match capabilities documented in the reverse-engineered Image Transmitter 2 codebase.

🧰 Future Enhancements (Optional)
More comprehensive video recording controls (e.g., full parameter query, start/stop if not fully implemented).
Implement retrieval of the currently active SD card slot (to complement the existing set functionality).
Verify and ensure all distinct USB Product IDs for the newly added Pentax/Ricoh models are covered for robust initial detection, supplementing the current custom ID mapping.
