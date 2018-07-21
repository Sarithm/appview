#!/bin/bash
while read pid name uptime l_ip l_port r_ip r_port
do
echo $name $pid
./ifxs test insert uptime=$uptime,l_ip=$l_ip,l_port=$l_port,r_ip=$r_ip,r_port=$r_port,ts=current into Procs where "pid=$pid&name=$name"
done
exit 0
