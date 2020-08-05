#!/bin/sh

check_dpkg_installed() {
	echo -n "."
	if [ $(dpkg-query -W -f='${Status}' $1 2>/dev/null | grep -c "ok installed") -eq 0 ];
	then
		MISSING_PKGS="$MISSING_PKGS $1"
	fi
}

echo "Running checks for native build on Raspberry PI OS"

if [ -z `which sudo` ] ; then
    apt-get install -y sudo
fi

REQUIRED_PKGS_STRETCH="libva1 libssl1.0-dev"
REQUIRED_PKGS_BUSTER="libva2 libssl-dev"
REQUIRED_PKGS="ca-certificates git binutils libasound2-dev libpcre3-dev libidn11-dev libboost-dev libcairo2-dev libdvdread-dev libdbus-1-dev libssh-dev gcc g++ sed pkg-config"

MAJOR_VERSION=`lsb_release -c | sed 's/Codename:[ \t]//'`
if [ "$MAJOR_VERSION" = "buster" ]; then
	REQUIRED_PKGS="$REQUIRED_PKGS_BUSTER $REQUIRED_PKGS"
elif [ "$MAJOR_VERSION" = "stretch" ]; then
	REQUIRED_PKGS="$REQUIRED_PKGS_STRETCH $REQUIRED_PKGS"
else
	echo "This script does not support $major_version"
	exit 1
fi

echo "Checking dpkg database for required packages"
MISSING_PKGS=""
for pkg in $REQUIRED_PKGS
do
	check_dpkg_installed $pkg
done
echo ""
if [ ! -z "$MISSING_PKGS" ]; then
	echo "You are missing required packages."
	echo "Run sudo apt-get update && sudo apt-get install $MISSING_PKGS"
else
	echo "All required packages are installed."
fi
echo ""

echo "Checking for OMX development headers"
# These can either be supplied by dpkg or via rpi-update.
# First, check dpkg to avoid messing with dpkg-managed files!
REQUIRED_PKGS="libraspberrypi-dev libraspberrypi0 libraspberrypi-bin"
MISSING_PKGS=""
for pkg in $REQUIRED_PKGS
do
	check_dpkg_installed $pkg
done
echo ""
if [ ! -z "$MISSING_PKGS" ]; then
	echo "You are missing required packages."
	echo "Run sudo apt-get update && sudo apt-get install $MISSING_PKGS"
	echo "Alternative: install rpi-update with sudo wget http://goo.gl/1BOfJ -O /usr/local/bin/rpi-update && sudo chmod +x /usr/local/bin/rpi-update && sudo rpi-update"
else
	echo "All development headers are installed"
fi
echo ""

echo "Checking dpkg database for optional packages"
OPTIONAL_PKGS="libavutil-dev libswresample-dev libavcodec-dev libavformat-dev libswscale-dev"
MISSING_PKGS=""
for pkg in $OPTIONAL_PKGS
do
	check_dpkg_installed $pkg
done
echo ""
if [ ! -z "$MISSING_PKGS" ]; then
	echo "You are missing optional packages. These packages are needed"
	echo "if you wish to compile omxplayer using external libraries."
	echo "To install them you can run the following command:\n"
	echo "Run sudo apt-get update && sudo apt-get install $MISSING_PKGS\n"
	echo "Alternatively you can download and compile the libraries by running 'make ffmpeg'."
else
	echo "All optional packages are installed. If all the required packages are also"
	echo "installed, you may compile omxplayer using external ffmpeg libraries."
fi
echo ""

echo "Checking amount of RAM in system"
#We require ~230MB of total RAM
TOTAL_RAM=`grep MemTotal /proc/meminfo | awk '{print $2}'`
TOTAL_SWAP=`grep SwapTotal /proc/meminfo | awk '{print $2}'`
if [ "$TOTAL_RAM" -lt 230000 ]; then
	echo "Your system has $TOTAL_RAM kB RAM available, which is too low. Checking swap space."
	if [ "$TOTAL_SWAP" -lt 230000 ]; then
		echo "Your system has $TOTAL_SWAP kB swap available, which is too low."
		echo "In order to compile ffmpeg you need to set memory_split to 16 for 256MB RAM PIs (0 does not work) or to at most 256 for 512MB RAM PIs, respectively."
		echo "Otherwise there is not enough RAM to compile ffmpeg. Please do the apropriate in the raspi-config and select finish to reboot your RPi."
		echo "Warning: to run compiled omxplayer please start raspi-config again and set memory_split to at least 128."
	else
		echo "You have enough swap space to compile, but speed will be lower and SD card wear will be increased."
		echo "In order to compile ffmpeg you need to set memory_split to 16 for 256MB RAM PIs (0 does not work) or to at most 256 for 512MB RAM PIs, respectively."
		echo "Otherwise there is not enough RAM to compile ffmpeg. Please do the apropriate in the raspi-config and select finish to reboot your RPi."
		echo "Warning: to run compiled omxplayer please start raspi-config again and set memory_split to at least 128."
	fi
else
	echo "You should have enough RAM available to successfully compile and run omxplayer."
fi
