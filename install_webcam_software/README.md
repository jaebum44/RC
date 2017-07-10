# nstall webcam software

Install guvcview webcam viewer to test if your webcam is working.

> sudo apt-get update
> sudo apt-get install guvcview

Start guvcview to check if your webcam is working fine. If not, change resolution thru GUI (320×240 should be fine). If you still got problem, it could be a permission/rights issue. Just type :

> sudo usermod -a -G video pi
> sudo modprobe uvcvideo

source: <https://thinkrpi.wordpress.com/2013/04/05/step-3-install-softwares-for-webcam-and-computer-vision/>
