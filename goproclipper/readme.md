# GoPro as webcam setup
https://github.com/jschmid1/gopro_as_webcam_on_linux

- Install gopro_as_webcam_on_linux:</br>
	`sudo su -c "bash <(wget -qO- https://cutt.ly/PjNkrzq)" root`

- Install dependencies:</br>
	`sudo apt install ffmpeg v4l2loopback-dkms curl vlc`

- Plug in gopro via usb and power it on (single press of power button on left). The screen should show a charging icon.
This process needs to continue running, so open a new terminal and type:</br>
	`sudo gopro webcam -n -a`

	This starts gopro in webcam mode using auto settings (-a), and no input from user (-n).
	
	Once everything gets initialized, near the bottom of the output, look for a line like this:</br>
	`Output #0, video4linux,v4l2, to '/dev/video42'`</br>
	This means my gopro's device id is 42. remember this.
  
# Install python libraries and run gopro_clipper

- install pip  
`sudo apt install python3-pip`

- install python libraries  
`pip install opencv-python numpy imutils readchar`

- Run gopro_clipper.py (if your device id is not 42, enter the correct device id instead)  
`python3 gopro_clipper.py -s 42`

- Press 'r' to start recording, 's' to stop, and 'q' or ESC to exit
