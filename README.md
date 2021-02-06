# DellG5SE-Fan-Linux
The Dell G5SE-5505 laptop isn't working with usual fan managers, this script is a small utility to automatically set fan speed according to thermals.
## Requirements
This is a python script, you'll need python, and the `sys`, `os`, `time` python libraries.
You will also need to modprobe the dell smm kernel module, which is not loaded by default on this laptop.
```shell
$ sudo modprobe dell-smm-hwmon restricted=0 ignore_dmi=1
```
If you want this setting to stay upon reboot, you can create a `/etc/modprobe.d/dell-smm-hwmon.conf` file containing
```shell
# This file must be at /etc/modprobe.d/
options dell-smm-hwmon restricted=0 ignore_dmi=1
options i8k force=1
```
## Usage
```shell
$ sudo python DellG5SEFan.py lowtemp maxtemp timer
```
For instance, 
```shell
$ sudo python DellG5SEFan.py 50 65 10
```
will disable fans for temperature below 50°C, put fans at 50% between 50°C and 65°C and at 100% over 65°C, with a 10 second loop.
