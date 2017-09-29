#! /bin/bash 

if [ $# != 2 ]
then
    echo "Need a name and a mode"
    echo "setbond name mode"
    exit -1
fi

if [ $2 -lt 0 ] || [ $2 -gt 6 ]
then
    echo "Invalid mode $2,should between 0~6"
    exit -1
fi

echo "
DEVICE=$1
NAME=$1
TYPE=Bond
BONDING_MASTER=yes
ONBOOT=yes
BOOTPROTO=none
BONDING_OPTS=\"mode=$2 miimon=100\"
" > "/etc/sysconfig/network-scripts/ifcfg-$1"


/sbin/ifdown $1 > /dev/null
/sbin/ifup $1 > /dev/null

ip addr add 172.16.10.68/24 dev $1
ip route add 172.16.6.0/24 via 172.16.10.254
