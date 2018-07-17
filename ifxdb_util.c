#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <errno.h>

int	flag=1;

void	err(int i)
{
	exit(i);
}
/* - - - - - - - - - - - - - - - - - - - - - */

int	read_sock(int sock, char *buf,int len)
{
int	NB,sc,wt;
	sc=3; wt=0;
	while(len>0)
	  {
 		NB=read(sock,buf,len);
		if(NB==0) err(9);
		  else if(NB<0)
		   { if(wt>180000) err(8);
			else { usleep(sc*1000); if(sc<1000) sc*=2; }
		   }
		else { buf+=NB; len-=NB; sc=3; wt=0; }
		wt+=sc;
	  }
	return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - */

int	get_rec(int sock,char *text,int *size)
{
struct 
{	int	len;
	int	code;
} 	head;
		
		read_sock(sock,(char *)&head,sizeof(head));
		*size=head.len-sizeof(head);
		if(*size<0) { return(-1); }
		read_sock(sock,text,*size);
		return(head.code);
}

/**********************************************************************/


int	main(int argc,char *argv[])
{
char	hostname[50],portname[30];
struct	sockaddr_in server;
struct	hostent *hp, *gethostbyname();
struct	servent	*port;

int	sock;
int	NN=8,uid;
int	NB,type,size;

 
struct
{
int	len;
int	code;
char	text[16384];
} buf;


struct	tm *t_out;

	uid=getuid(); uid=0;
	if(argc<3)	err(2);
	bzero(hostname,sizeof(hostname)); bzero(portname,sizeof(portname));
	strcpy(hostname,"localhost"); strcpy(portname,"cwsdata"); strncat(portname,argv[1],10);
	if(portname[0]==0 || hostname[0]==0) err(1);

	port=getservbyname(portname,0);
	if(port==0)                             err(7);
	server.sin_family=AF_INET;
	/* server.sin_len=sizeof(server); */
	hp=gethostbyname(hostname); if(hp==0)   err(7);
	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	server.sin_port=port->s_port;

	sock=socket(AF_INET,SOCK_STREAM,0);
	if(connect(sock, (struct sockaddr *)&server,sizeof(server))<0) err(7);

	buf.code=19; buf.len=8;
	buf.code=buf.code|0xff030100;
	write(sock,&buf,buf.len);
/*	ioctl(sock,FIONBIO,&flag); */
	sleep(2);
	exit(0);

}
