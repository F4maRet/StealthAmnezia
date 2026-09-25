#!/bin/bash

# This scripts copied from Amnezia client to Docker container to /opt/amnezia and launched every time container starts

echo "Container startup"
#ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up

# kill daemons in case of restart
wg-quick down /opt/amnezia/awg/wg0.conf

# start daemons if configured
if [ -f /opt/amnezia/awg/wg0.conf ]; then (wg-quick up /opt/amnezia/awg/wg0.conf); fi

# Idempotent iptables: add the rule only if it is not present yet,
# so container restarts do not pile up duplicate rules
ipt() { local t=(); if [ "$1" = "-t" ]; then t=(-t "$2"); shift 2; fi; iptables "${t[@]}" -C "$@" 2>/dev/null || iptables "${t[@]}" -A "$@"; }

# Allow traffic on the TUN interface.
ipt INPUT -i wg0 -j ACCEPT
ipt FORWARD -i wg0 -j ACCEPT
ipt OUTPUT -o wg0 -j ACCEPT

# Allow forwarding traffic only from the VPN.
ipt FORWARD -i wg0 -o eth0 -s $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -j ACCEPT
ipt FORWARD -i wg0 -o eth1 -s $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -j ACCEPT

ipt FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

ipt -t nat POSTROUTING -s $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -o eth0 -j MASQUERADE
ipt -t nat POSTROUTING -s $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -o eth1 -j MASQUERADE

tail -f /dev/null
