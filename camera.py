# Imports
import RPi.GPIO as GPIO
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

def take_picture():
	img_filename = strftime("cam-%Y-%m-%d_%H:%M:%S.png", gmtime())
	camera.capture(img_filename)

	print("PICTURE CAPTURED!")
	
#	with SSHClient() as client:
#		client.set_missing_host_key_policy(AutoAddPolicy())
#		client.connect(
#			creds.hostname,
#			port=22,
#			username=creds.username,
#			password=creds.password
#		)
	with client.open_sftp() as sftp:
		sftp.put(img_filename,creds.path + '/' + img_filename)

	print("IMAGE UPLOADED!")



# Open FTP connection
client = SSHClient()
client.set_missing_host_key_policy(AutoAddPolicy())
client.connect(
	creds.hostname,
	port=22,
	username=creds.username,
	password=creds.password
)

# Setup camera
camera = PiCamera()
camera.resolution = (800,600)

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
GPIO.cleanup()
