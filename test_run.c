#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<strings.h>
#include	<stdint.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/sysinfo.h>
#include	<dirent.h>
#include	<netinet/in.h>
#include	<linux/netlink.h>
#include	<linux/rtnetlink.h>
#include	<linux/tcp.h>
#include	<linux/sock_diag.h>
#include	<linux/inet_diag.h>
#include	<arpa/inet.h>
#include	<errno.h>

#define	TCPF_ALL 0xFFF

int	main(int argc,char *argv[])
{
struct	sysinfo	SysInfo;
DIR	*DD,*DDP;
FILE	*fp;
struct	dirent	*PP,*pPP;
char	dir_path[512];
char	f_path[512];
char	buf[512];
char	*p;
char	name[40];
int	i;
int	TCK;
unsigned int	pid,ppid;
unsigned long	utime,stime,Rtime;
unsigned long long int	start;

int	netl_sock;
struct	sockaddr_nl		sa;
struct	inet_diag_req_v2	conn_req;
struct	iovec	iov[4];
struct	nlmsghdr	nlh,*nlp;
struct	msghdr		msg;
int	rc,len,loop;
struct	inet_diag_msg	*diag_msg;
char	ip1[30],ip2[30];
uint8_t	recv_buf[8192];

	TCK=sysconf(_SC_CLK_TCK);
	sysinfo(&SysInfo);
	fprintf(stdout,"TCK %i\n",TCK);
	DD=opendir("/proc");
	if(DD==NULL) fprintf(stderr,"DD is NULL,%i\n",errno);

	while((PP=readdir(DD))!=NULL)
	 {
	   if((*PP).d_type==4 && (*PP).d_name[0]>='1' && (*PP).d_name[0]<='9')
	     {
	/*	fprintf(stderr,"name %s %i\n",(*PP).d_name,(*PP).d_type); */
	        strcpy(dir_path,"/proc/"); strncat(dir_path,(*PP).d_name,256);
		DDP=opendir(dir_path);
	        if(DDP!=NULL)
		 {
		   while((pPP=readdir(DDP))!=NULL)
		    {
			if((*pPP).d_type==8 && strcmp((*pPP).d_name,"stat")==0)
			 {
		/*	   fprintf(stderr,"-->name %s %i\n",(*pPP).d_name,(*pPP).d_type); */
			   strcpy(f_path,dir_path); strcat(f_path,"/stat");
			   if((fp=fopen(f_path,"r"))!=NULL)
			     {
				bzero(buf,256);
				fgets(buf,256,fp);
				p=strtok(buf,"\t "); pid=atol(p);
				p=strtok(0,"\t "); strncpy(name,p,40);
				p=strtok(0,"\t "); p=strtok(0,"\t ");
				ppid=atol(p);

				for(i=0;i<10;i++) p=strtok(0,"\t "); /* +14 */
				utime=atol(p);
				p=strtok(0,"\t ");
				stime=atol(p);
				for(i=0;i<7;i++) p=strtok(0,"\t "); /* +22 */
				start=atoll(p);
				Rtime=SysInfo.uptime-start/TCK;

				if(ppid!=2)	/* kernel processes ? */ 
				  {
				     fprintf(stderr,"%u %u %s - %u %lu %lu %llu\n",pid,ppid,name,Rtime,utime,stime,start);
				  }
				fclose(fp);
			     }
			 }
		    }
		   closedir(DDP);
		 }
	     }
		
	 }
	closedir(DD);

	netl_sock=socket(AF_NETLINK,SOCK_DGRAM,NETLINK_INET_DIAG);
	if(netl_sock<0) { fprintf(stderr,"NETLINK socket %i\n",errno); exit(4); }
	
	sa.nl_family=AF_NETLINK;
	conn_req.sdiag_family=AF_INET;
	conn_req.sdiag_protocol=IPPROTO_TCP;
	conn_req.idiag_states=TCPF_ALL;

	nlh.nlmsg_len=NLMSG_LENGTH(sizeof(conn_req));
	nlh.nlmsg_flags=NLM_F_DUMP | NLM_F_REQUEST;
	nlh.nlmsg_type=SOCK_DIAG_BY_FAMILY;
	iov[0].iov_base=(void *)&nlh;
	iov[0].iov_len=sizeof(nlh);
	iov[1].iov_base=(void *)&conn_req;
	iov[1].iov_len=sizeof(conn_req);

	msg.msg_name=(void *)&sa;
	msg.msg_namelen=sizeof(sa);
	msg.msg_iov=iov;
	msg.msg_iovlen=4;

	rc=sendmsg(netl_sock,&msg,0);

	fprintf(stderr,"return code %i %i\n",rc,errno);

	loop=1;
	while(loop)
	 {
	   rc=recv(netl_sock,recv_buf,sizeof(recv_buf),0);
	   fprintf(stdout,"GOT %i %i\n",rc,errno);
	   nlp=(struct nlmsghdr *)recv_buf;
	   while(NLMSG_OK(nlp,rc))
	    {
		if((*nlp).nlmsg_type==NLMSG_DONE) { fprintf(stdout,"the end of the message\n"); loop=0;  break; }
		fprintf(stdout,"doing something here meanwhile\n");
		diag_msg=(struct inet_diag_msg *) NLMSG_DATA(nlp);
		len=(*nlp).nlmsg_len-NLMSG_LENGTH(sizeof(diag_msg));
		if((*diag_msg).idiag_family==AF_INET)
		  {
		     inet_ntop(AF_INET,(struct in_addr *)&((*diag_msg).id.idiag_src),ip1,INET_ADDRSTRLEN);
		     inet_ntop(AF_INET,(struct in_addr *)&((*diag_msg).id.idiag_dst),ip2,INET_ADDRSTRLEN);
		     fprintf(stdout,"DATA %s %u | %s %u\n",ip1,ntohs((*diag_msg).id.idiag_sport),
			ip2,ntohs((*diag_msg).id.idiag_dport));
		  }
		fprintf(stdout,"size %i\n",len);
		nlp=NLMSG_NEXT(nlp,rc);
	    }
	 }

	exit(0);

}
