# Fire_Smoke_Detection_ML_MODEL

### FireSight Guardian App - Vision-Based Smoke Detection & Alert System

This project was my final year university project.

An innovative smoke detection system prioritizing fast response times, affordability, and scalability. The system utilizes low-cost ESP32-CAM modules with built-in Wi-Fi and microcontrollers to capture live video streams, with basic image preprocessing handled on-device. These modules are strategically placed for comprehensive area coverage. It is not necessary to use an ESP32-CAM; any IP camera connected to the WLAN will suffice.

Smoke detection is performed using a lightweight YOLOv5s6 model processed on a central computer (with Jetson Nano as an option for real-world scenarios). The system delivers swift alerts via local alarms or smartphone notifications. Optional features for further development include GSM modules for uninterrupted transmission in weak Wi-Fi areas and solar power for extended outdoor deployments. Additional enhancements include flame location tracking, redundant detection frame removal, and intelligent delay mechanisms for improved accuracy and efficiency.

<img src="https://github.com/rnshalinda/FireSight_Guardian_ML_FireDetection_System/blob/main/App%20snaps/1.PNG"
     width="700px"
     height="750px"
     alt="App Screenshot">

