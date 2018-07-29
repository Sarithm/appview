/* by Slava Barsuk 071918  */

#define		_GNU_SOURCE

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<strings.h>
#include	<stdint.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<sched.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/sysinfo.h>
#include	<sys/stat.h>
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

int	NLnumber,NListener,Nnetns;
struct	Nlink
{	
	unsigned int	l_ip,r_ip;
	unsigned short	int l_port,r_port;
	unsigned int	inode;
	unsigned int	nsinode;
} 	Nlinks[20000];

struct	Listener
{
	unsigned int		ls_ip;
	unsigned int		nsinode;
	unsigned short int	ls_port;
	unsigned short int	connect;
}	Listener[2000];

struct	nsnet
{
	char	proc[20];
	unsigned int Inode;
}	Nnet[1000];

int	main(int argc,char *argv[])
{
struct	sysinfo	SysInfo;
DIR	*DD,*DDP;
FILE	*fp;
struct	dirent	*PP,*pPP;
char	dir_path[512];
char	d1_path[512];
char	f_path[512];
char	buf[512];
char	*p,*p1;
char	name[40];
int	i,j,Ok,ll;
int	TCK;
int	fd;
unsigned int	pid,ppid,Inode;
unsigned long	utime,stime,Rtime,vsize,rss;
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
char	f_link[100];
char	NCODE;

	TCK=sysconf(_SC_CLK_TCK);
	sysinfo(&SysInfo);

	Nnetns=0;	bzero((void *)Nnet,sizeof(Nnet));
	NListener=0;	bzero((void *)Listener,sizeof(Listener));
	NLnumber=0;	bzero((void *)Nlinks,sizeof(Nlinks));

	DD=opendir("/proc");
	if(DD==NULL) { fprintf(stderr,"DD is NULL,%i\n",errno); exit(4); }

	while((PP=readdir(DD))!=NULL)
	 {
	   if((*PP).d_type==4 && (*PP).d_name[0]>='1' && (*PP).d_name[0]<='9')
	     {
	        strcpy(dir_path,"/proc/"); strncat(dir_path,(*PP).d_name,256); strcat(dir_path,"/ns/net");
		bzero(f_link,100); 
		readlink(dir_path,f_link,99);
		if(strncmp(f_link,"net:[",5)==0)
		 {
		   p=strtok(f_link,"["); p=strtok(0,"[");
		   Inode=atol(p);
		   for(i=0;i<Nnetns;i++)
		     if(Inode==Nnet[i].Inode) break;
		   if(i==Nnetns &&Nnetns<1000)
		    {
			strncpy(Nnet[i].proc,(*PP).d_name,20);
			Nnet[i].Inode=Inode;
			Nnetns++;
		    }
		 }
		 
		else fprintf(stdout,"---> PROBLEM %s %s\n",(*PP).d_name,f_link);
		
	     }
	 }
	closedir(DD);
/* 	for(i=0;i<Nnetns;i++) fprintf(stdout,"NET %s %u\n",Nnet[i].proc,Nnet[i].Inode); */

	for(ll=0;ll<Nnetns;ll++)
	  {

	sprintf(f_link,"/proc/%s/ns/net",Nnet[ll].proc);
	fd=open(f_link,O_RDONLY);
	rc=setns(fd,0); 
/* fprintf(stdout,"SETNS %s %i %i %i\n",f_link,fd,rc,errno); */

	   for(j=0;j<2;j++)
	    {

		netl_sock=socket(AF_NETLINK,SOCK_DGRAM,NETLINK_INET_DIAG);
		if(netl_sock<0) { fprintf(stdout,"NETLINK socket %i\n",errno); exit(4); }
	
		sa.nl_family=AF_NETLINK;
		if(j==0) conn_req.sdiag_family=AF_INET;
		  else conn_req.sdiag_family=AF_INET6;
	/*	conn_req.sdiag_family=AF_INET;  */
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

/*	fprintf(stdout,"return code %i %i\n",rc,errno); */

		loop=1;
		while(loop)
	 	 {
	   	   rc=recv(netl_sock,recv_buf,sizeof(recv_buf),0);
	   	   nlp=(struct nlmsghdr *)recv_buf;
	   	   while(NLMSG_OK(nlp,rc))
	    	    {
			if((*nlp).nlmsg_type==NLMSG_DONE) { loop=0;  break; }
			diag_msg=(struct inet_diag_msg *) NLMSG_DATA(nlp);
			len=(*nlp).nlmsg_len-NLMSG_LENGTH(sizeof(diag_msg));
			if((*diag_msg).idiag_family==AF_INET || (*diag_msg).idiag_family==AF_INET6)
		  	 {
		    	  if(NLnumber<20000)
		      	   {
				Nlinks[NLnumber].l_ip=ntohl((*diag_msg).id.idiag_src[0]);
				Nlinks[NLnumber].r_ip=ntohl((*diag_msg).id.idiag_dst[0]);
				Nlinks[NLnumber].l_port=ntohs((*diag_msg).id.idiag_sport);
				Nlinks[NLnumber].r_port=ntohs((*diag_msg).id.idiag_dport);
				Nlinks[NLnumber].inode=(*diag_msg).idiag_inode;
				Nlinks[NLnumber].nsinode=Nnet[ll].Inode;
				if(Nlinks[NLnumber].r_ip==0 && Nlinks[NLnumber].r_port==0)
			  	 {
			    	   for(i=0;i<NListener;i++)
			     	   if(Listener[i].nsinode==Nlinks[NLnumber].nsinode &&
				     Listener[i].ls_ip==Nlinks[NLnumber].l_ip && Listener[i].ls_port==Nlinks[NLnumber].l_port) break;
			    	   if(i==NListener && NListener<10000)
			     	    {
					Listener[i].ls_ip=Nlinks[NLnumber].l_ip;
					Listener[i].ls_port=Nlinks[NLnumber].l_port;
					Listener[i].nsinode=Nlinks[NLnumber].nsinode;
					NListener++;
			     	    }
			 	  }
				NLnumber++;
		      	   }
		  	 }
			else
		  	 {
			   fprintf(stdout,"other protocol %u\n",(*diag_msg).idiag_family);
			 }
			nlp=NLMSG_NEXT(nlp,rc);
	     	    }
	 	 }
fprintf(stdout,"NET COUNT %u %u\n",Nnet[ll].Inode,NLnumber);
		close(netl_sock);
	    }
		close(fd);
	}
	
/*
	for(i=0;i<NListener;i++)
	 fprintf(stderr,"LL %u %u\n",Listener[i].ls_ip,Listener[i].ls_port);
*/

	DD=opendir("/proc");
	if(DD==NULL) { fprintf(stderr,"DD is NULL,%i\n",errno); exit(4); }

	while((PP=readdir(DD))!=NULL)
	 {
	   if((*PP).d_type==4 && (*PP).d_name[0]>='1' && (*PP).d_name[0]<='9')
	     {
	        strcpy(dir_path,"/proc/"); strncat(dir_path,(*PP).d_name,256);
		Ok=0;
		
			   strcpy(f_path,dir_path); strcat(f_path,"/stat");
			   if((fp=fopen(f_path,"r"))!=NULL)
			     {
				bzero(buf,256);
				fgets(buf,256,fp);
				p=strtok(buf,"\t "); pid=atol(p);
				p=strtok(0,"\t "); p++; p1=name; i=0;
				while(*p!=')' && *p!=0 && i<40) *(p1++)=*(p++); *p1=0;
				
			/* 	 strncpy(name,p,40); */
				p=strtok(0,"\t "); p=strtok(0,"\t ");
				ppid=atol(p);

				for(i=0;i<10;i++) p=strtok(0,"\t "); /* +14 */
				utime=atol(p);
				p=strtok(0,"\t ");
				stime=atol(p);
				for(i=0;i<7;i++) p=strtok(0,"\t "); /* +22 */
				start=atoll(p);
				Rtime=SysInfo.uptime-start/TCK;
				p=strtok(0,"\t ");
				vsize=atol(p)/4096;
				p=strtok(0,"\t ");
				rss=atol(p);

				if(ppid!=2)	/* kernel processes ? */ 
				  {
				     Ok=1;
				 /*    fprintf(stdout,"%u %u %s - %u %lu %lu %llu - %lu %lu\n"
					,pid,ppid,name,Rtime,utime,stime,start,vsize,rss);
				 */
				  }
				fclose(fp);
			     }
		
		if(Ok>0)
		 {
		strncpy(d1_path,dir_path,512); strcat(d1_path,"/fd");
		DDP=opendir(d1_path);
		if(DDP!=NULL)
		 {
		   while((pPP=readdir(DDP))!=NULL)
		    {
			if((*pPP).d_type==DT_LNK)	/* 10 */
			 {
				
			strncpy(f_path,d1_path,512); strcat(f_path,"/");
			strncat(f_path,(*pPP).d_name,512);
			bzero(f_link,100);
			rc=readlink(f_path,f_link,99);
			if(strncmp(f_link,"socket:[",8)==0)
			  {
				p=strtok(f_link,"[");  p=strtok(0,"[");
				Inode=atol(p);
				for(i=0;i<NLnumber;i++)
				 {
				   if(Nlinks[i].inode==Inode)
				    {
				      if(Nlinks[i].r_ip==0 && Nlinks[i].r_port==0)
				       {
					NCODE='L';
				       }
				      else
				       {
					  for(j=0;j<NListener;j++)
					    if(Nlinks[i].nsinode==Listener[j].nsinode && Nlinks[i].l_port==Listener[j].ls_port &&
						(Listener[j].ls_ip==0 || Nlinks[i].l_ip==Listener[j].ls_ip) )  break;
					    if(j<NListener)
						NCODE='S';
					    else NCODE='C';
				       }
				      Ok++;
				      fprintf(stdout,"%u %s %u %u %c   %u %u %u %u\n",
				        pid,name,Nlinks[i].nsinode,Rtime,NCODE,Nlinks[i].l_ip,Nlinks[i].l_port,Nlinks[i].r_ip,Nlinks[i].r_port);
				      break;
				    }
				 }
	/*		if(i==NLnumber)
				   fprintf(stdout,"%u %s %u X %u\n",pid,name,Rtime,Inode);
	*/
			  }
			 }
		    }
		   closedir(DDP);
		 }
	     } /* Ok */
	    if(Ok==1) 	/* no network connections found */
	     {
		fprintf(stdout,"%u %s %u %u N %u %u %u %u\n",
			pid,name,0,Rtime,0,0,0,0);
	     }
	  } 
		
	 }
	closedir(DD);
/*
	fprintf(stdout," NLnumber %i\n",NLnumber);
	for(i=0;i<NLnumber;i++)
	 fprintf(stdout,"%u: %u(%u) - %u(%u)\n",Nlinks[i].inode,Nlinks[i].l_ip,Nlinks[i].l_port,
		Nlinks[i].r_ip,Nlinks[i].r_port);
*/
	exit(0);

}
