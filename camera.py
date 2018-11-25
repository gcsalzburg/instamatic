# Imports
import RPi.GPIO as GPIO
import os

from picamera import PiCamera
from time import gmtime,strftime
from paramiko import SSHClient, AutoAddPolicy
import sftp as creds

# Global vars
global img_filename
global client

# Button callbacks
def button1_callback(channel):
	print("CAMERA BUTTON!")
	take_picture()

def button2_callback(channel):
	print("BUTTON 2")

# SFTP file upload callback
def sftp_callback(xfer, to_be_xfer):
	print ('Uploading: {0:.0f} %'.format((xfer / to_be_xfer) * 100))


# Picture capture fn
def take_picture():
	img_filename = strftime("cam-%Y-%m-%d_%H:%M:%S.png", gmtime())
	camera.capture(img_filename)

	print("PICTURE CAPTURED!")

	with client.open_sftp() as sftp:
		sftp.put(img_filename,creds.path + '/' + img_filename, callback=sftp_callback)
		sftp.close()

	print("IMAGE UPLOADED!")
	os.remove(img_filename)



print("Starting camera app...")
print("Opening FTP...")

# Open FTP connection
client = SSHClient()
client.set_missing_host_key_policy(AutoAddPolicy())
client.connect(
	creds.hostname,
	port=22,
	username=creds.username,
	password=creds.password
)

print("Starting camera...")

# Setup camera
camera = PiCamera()
camera.resolution = (800,600)

print("Setting up GPIO...")

# Setup IO
button1 = 2
button2 = 3
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(button1, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(button2, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.add_event_detect(button1,GPIO.FALLING,callback=button1_callback)
GPIO.add_event_detect(button2,GPIO.FALLING,callback=button2_callback)


# Message output
print("Camera app started!")
message=input("Press enter to quit \n\n")
client.close()
GPIO.cleanup()
