# instamatic
1960s Kodak technology meets the 21st century

Single push SFTP photography!



## Useful links ##

### Setup

Settings up a headless Pi Zero
https://desertbot.io/blog/headless-pi-zero-w-wifi-setup-windows

> **host:** instamatic.local
>
> **user:** pi
>
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

### SFTP 

Using paramiko to automatically get SSH host key:
https://www.reddit.com/r/Python/comments/5ib6bg/how_can_i_use_with_pysftp_and_set_missing_host/
