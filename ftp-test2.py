from paramiko import SSHClient, AutoAddPolicy
import sftp as creds



with SSHClient() as client:
	client.set_missing_host_key_policy(AutoAddPolicy())
	client.connect(
		creds.hostname,
		port=22,
		username=creds.username,
		password=creds.password
	)
	with client.open_sftp() as sftp:
		sftp.put('camera.py','penthouse.designedbycave.co.uk/camera/camera2.py')
