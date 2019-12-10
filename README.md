# 📸 instamatic
> 1960s Kodak technology meets the 21st century

Single click (S)FTP photography! (curl POST request in future?)

Notes on GPIO

+ IO4 = Flash light
+ IO16 = Crashes camera if held low when photo call initiated?

## Useful links

+ https://www.totalcardiagnostics.com/support/Knowledgebase/Article/View/92/20/prolific-usb-to-serial-fix-official-solution-to-code-10-error
+ https://www.banggood.com/Geekcreit-ESP32-CAM-WiFi-bluetooth-Camera-Module-Development-Board-ESP32-With-Camera-Module-OV2640-p-1394679.html

## Headless Pi Zero ##

Original attempt was implemented using a headless Pi Zero. Abandoned due to the slow boot time of the device, and the discovery of the ESP32-CAM. It did, however, work! And allowed push via SFTP.

Documented here for future information:

### Setup

Settings up a headless Pi Zero
https://desertbot.io/blog/headless-pi-zero-w-wifi-setup-windows

> **host:** instamatic.local
> **user:** pi
> **password:** see pw storage manager

Don't forget to enable the camera in the `raspi-config`

### Test camera

To test the camera, take a photo using `raspistill -v -o test.jpg`.

The picture can the be viewed by starting a simple webserver in the folder like this:

```bash
python -m SimpleHTTPServer 8080
```
*Idea from: https://unix.stackexchange.com/questions/35333/what-is-the-fastest-way-to-view-images-from-the-terminal*

On any PC on the network, see the image at: http://instamatic.local:8080/test.jpg

## Install packages

```bash
sudo apt-get install python-pip
sudo pip install PiCamera
sudo pip install RPi.GPIO

sudo apt-get install libffi-dev
sudo apt-get install libssl-dev
sudo pip install paramiko

sudo apt-get install -y supervisor

```

### Run on startup

All methods (`/etc/rc.local`, `sudo crontab -e`, `python-daemon`) seemed to fail. But this seemed to work well!

1. Follow instructions here: https://serversforhackers.com/c/monitoring-processes-with-supervisord
2. Run as `sudo supervisorctl` to ensure scripts run correctly.
3. If error as shown below apply chmod fix as shown here: https://github.com/Supervisor/supervisor/issues/173#issuecomment-16448908

```bash
error: <class 'socket.error'>, [Errno 13] Permission denied: file: /usr/lib/python2.7/socket.py line: 228
```

Can monitor processes from: http://instamatic.local:9001/ (currently no username/password)

To log errors to a script, run:
```bash
sudo python appname.py >> logfile.log 2>&1
```
From: https://stackoverflow.com/a/30295011/10240581

### SFTP 

Using paramiko to automatically get SSH host key:
https://www.reddit.com/r/Python/comments/5ib6bg/how_can_i_use_with_pysftp_and_set_missing_host/

## How to adjust the camera properties

```c++
// Change extra settings if required
sensor_t * s = esp_camera_sensor_get();
// s->set_vflip(s, 0);       //flip it back
s->set_brightness(s, 3);  //up the blightness just a bit
//s->set_saturation(s, -2); //lower the saturation
```