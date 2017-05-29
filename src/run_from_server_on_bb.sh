#!/bin/bash

##--------@Server: compile src--------##
make clean -C ~/casper/tracking/tracking-final -f arm.mk
make clean -C ~/casper/tracking/tracking-final-dsp -f arm.mk

make -C ~/casper/tracking/tracking-final -f arm.mk
make -C ~/casper/tracking/tracking-final-dsp -f arm.mk

##--------mv server-> bb--------##
scp -oKexAlgorithms=+diffie-hellman-group1-sha1 ~/casper/tracking/tracking-final/armMeanshiftExec root@192.168.0.202:~/casper/bb/
scp -oKexAlgorithms=+diffie-hellman-group1-sha1 ~/casper/tracking/tracking-final-dsp/Release/pool_notify.out root@192.168.0.202:~/casper/bb/

##--------@bb: exec()--------##
# ssh  -oKexAlgorithms=+diffie-hellman-group1-sha1 root@192.168.0.202 '/home/root/powercycle.sh && /home/root/casper/bb/armMeanshiftExec /home/root/casper/bb/car.avi /home/root/casper/bb/pool_notify.out'
