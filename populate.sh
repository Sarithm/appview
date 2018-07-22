#!/bin/bash
hostid=$(echo `hostname`)
while read pid name uptime l_ip l_port r_ip r_port
do
echo $name $pid $hostid
./ifxs test insert uptime=$uptime,l_ip=$l_ip,l_port=$l_port,r_ip=$r_ip,r_port=$r_port,ts=current into Procs where "pid=$pid&name=$name&hostid=$hostid"
done
exit 0
