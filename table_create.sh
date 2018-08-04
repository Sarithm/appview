NAME=$1
./ifxad $NAME insert table=Procs,hostid=s:30,pid=i,name=s:30,ppid=i,uptime=i,l_ip=i,l_port=i,r_ip=i,r_port=i,type=s:2,ts=i into TABLES where "pool=1&type=regular"
