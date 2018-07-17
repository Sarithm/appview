/* by Slava Barsuk */

#include	<pthread.h>
#include	<stdio.h>
#include	<stdint.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<time.h>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<sys/select.h>
#include	<sys/ioctl.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<strings.h>
#include	<string.h>
#include	<signal.h>
#include	<errno.h>
#include	"dbdefv32.h"

FILE	*flog;
pthread_cond_t  S_condT,S_condR,S_condP[10],S_condW;
pthread_mutex_t S_mutxT,S_mutxR,S_mutxP[10],S_mutxW;
pthread_t	p_read;
void	*read_processing();
extern void	modify_acc_list(int,int);

int	on=1,off=0,rt[2]={8,3}; /* rt - access denied return string */


extern	void	*t_routine();
extern	void	*p_routine();
extern	unsigned int OPSTAT[9];
extern	time_t	tp;
extern	struct	pool		POOL[10];
extern	struct	initdata	initdata;
extern	struct	XT		xt[50];
extern	int	ACC_LIST[200];
extern	int	last_flushT,last_flush[10];

char path[100];

struct	sockaddr_in	server,from;
unsigned long int	lfrom=sizeof(struct sockaddr_in);
int	sock,ws,count;
int	RQ_SIZE=100;
struct	RQ {
	uint64_t		ts;
	unsigned short int 	xx,bb,code,owner;
	unsigned char		*p,c,pool,t_id;
	int			ip,ws,inb[3];
} RQ[100];
struct	TF	fs[200],fp[10][200];
int	NQ=200,C_enable=0,E_enable=0,SYNC_R=0;

unsigned char	*P_BUFA[10],*P_BUFM[10];
int	P_BUFSTAT[10]={0,0,0,0,0,0,0,0,0,0};
unsigned int	pool_thrash[20];
unsigned int	pool_thrash_ts[20];
unsigned int	pool_w[20];
unsigned int	pool_w_ts[20];
unsigned int	pool_que[20];
unsigned int	pool_que_max[20];

int	checkaccess(int);
int	main_processing(struct RQ *);
int	main_p1(struct RQ *);

void	err(int code)
{
static	char msg[14][25]=
 {
	"other",
	"invalid startup string",
	"unable to open cfg file",  
	"invalid port name",
	"src sock problem",
	"unable to create sock",
	"bind problem",
	"main sock problem",
	"unable to open main file",
	"can't read main file",
	"bad main data file",
	"initdata corrupted",
	"unable to open pool file",
	" "
};
	fprintf(stderr,"--ifxdb error -- %s\n",msg[code]);
	exit(4);
}
/* - - - - - - - - - - - - - - - - - - - - - - - */
int	get_buf(struct RQ *qp)
{
register int	pool,i,size,L,L1;
unsigned char	*p,*pm;
	
	pool=(*qp).pool; size=((*qp).inb[0]>>7);
	pm=P_BUFM[pool]; L=32; p=P_BUFA[pool];  L1=0;
	while(size>0) { size=(size>>1); pm+=L; p+=4096; L=(L>>1); L1++; }
	P_BUFSTAT[L1]++;
/* fprintf(flog,"buf p %i, s %i, L1 %i\n",pool,(*qp).inb[0],L1); */
	for(i=0;i<L;i++) if(pm[i]==0) break;
	if(i<L)
	 {
	   pm[i]++;
	   (*qp).p=p+(i<<(7+L1));
	   bzero((*qp).p,(128<<L1));
	   return(0);
	 }
	 else return(-1);
} 
/* - - - - - - - - - - - - - - - - - - - - - - - */
void	release_buf(unsigned char *p,int pool)
{
int		i,j,L,L1;
unsigned char	*pm;

	i=p-P_BUFA[pool]; pm=P_BUFM[pool];
	if(i>=0 && i<16384) 
	 {
	L=32; L1=0;
	while(i>=4096) { pm+=L; i-=4096; L=(L>>1); L1++; }
	j=(i>>(7+L1));
	if(pm[j]==1) pm[j]=0;
	 else  fprintf(flog,"deallocation problem pool %i %i %i %i %i\n",pool,i,L,L1,j);
	 } else fprintf(flog,"deallocation out of range %i %i\n",pool,i);
	
}
/* - - - - - - - - - - - - - - - - - - - - - - - */
void	*read_processing()
{
uint64_t		ttp;
int			i,ct,ct1,ct2,ct3,NB,*inb,opcode,rc,sock,diff_c,diff_ms,ll,t_to_ts,t_to_lr;
unsigned char		*pp;
struct	RQ		*qp;
struct timespec		ttT;

    pthread_mutex_lock(&S_mutxR);
    t_to_ts=235; t_to_lr=100;

     while(1)
      {
/*	ttp=mach_absolute_time(); */
	clock_gettime(CLOCK_REALTIME,&ttT);
	ttp=ttT.tv_sec*1000000+ttT.tv_nsec/1000;
	ct=0; ct1=0; ct2=0; ct3=0;
	for(i=0;i<RQ_SIZE;i++)
	 {
	  qp=&RQ[i];
	  if((sock=(*qp).ws)>0)
	   { 
		ct++;
		diff_ms=(ttp-(*qp).ts)/1000;
		diff_c=diff_ms/1000;

		if(diff_c>t_to_ts)
		 {
		    (*qp).c=0; (*qp).code=1000; ct1++;
		 }

		else if(diff_ms<t_to_lr)
			{ ct1++; ct2++; }
		else
		 {
		   inb=(*qp).inb;
	           if((*qp).c==1) ll=8; else if((*qp).c==2) ll=12; else if((*qp).c==3) ll=10;
			else if((*qp).c==5)  ll=inb[0]; 
/* fprintf(flog,"R ll/c %i %i, sock %i\n",ll,(*qp).c,sock); */ 
		   NB=recv(sock,(*qp).p+(*qp).bb,ll-(*qp).bb,0);
		   if(NB==0) { (*qp).c=0; (*qp).code=1001; ct1++; }
		    else if(NB<0) { ct1++; ct3++;   (*qp).ts=ttp;   }
		    else
		     { 
			(*qp).bb+=NB; (*qp).ts=ttp-t_to_lr-2; 

			if((*qp).bb==ll)   
			 { 
			  if((*qp).c==1)
			   {
			    inb[0]-=8;
			    if( inb[0]>=0 && inb[0]<2048 &&
				( inb[1]&0xfff00000)==0xff000000 )
			     { 
				opcode=inb[1]&0x000000ff; (*qp).c=4;
				if((*qp).owner==0 && opcode<20 && opcode!=3)
				  { (*qp).c=0; (*qp).code=1002; }
				 else
				if(opcode==15 || opcode==16)
			          { if(inb[0]==4) (*qp).c=2; }
				 else
				if(opcode==2 || opcode==4 || opcode==6 ||
				   opcode==7 || (opcode>=20 && opcode<32))
				  { if(inb[0]>=2) (*qp).c=3; }
			     } else { (*qp).c=0; (*qp).code=1003; }

			    } else if((*qp).c==5)
			      {
				(*qp).c=6; 
			      }
			    else (*qp).c=4; 
			  }
			if((*qp).c==4)
			 {
			   if((rc=main_processing(qp))<0) { (*qp).code=1004; (*qp).c=0; }
			    else
			     if(rc>0)
			       {
				 (*qp).c=0; close(sock);
			       }
			    else
			     {
				if(inb[0]==0) (*qp).c=6;
				 else
				  {
				    (*qp).bb=0;
				    if((rc=get_buf(qp))<0)
				      {  (*qp).code=1006; (*qp).c=0; }
				    else (*qp).c=5;
				  }
			     } 
			 }
			if((*qp).c==6)
			 {
			      if(main_p1(qp)<0) (*qp).code=1005;
				else  (*qp).code=0;
			      (*qp).c=0;
			 }
		  
		   } /* else - sucessful read */
	        } /* else  - go for read */

	       if((*qp).c==0)
		{
		  if((*qp).code>0)
		   {
		     if( (*qp).code>=1002) send(sock,(char *)rt,8,0);
		     fprintf(flog,"ERR_CODE: %i %i %x %llu  \n",(*qp).ip,(*qp).code,(*qp).p,ttp);
		     close(sock);
		   }
		  bzero((char *)qp,sizeof(struct RQ));
		}

	    } /* if((sock== */
	  } /* for(i= */
	if(ct1>0)
	 { if(ct2>ct3) { if(t_to_lr>10) t_to_lr-=5; }
	   else { if(t_to_lr<400) t_to_lr+=5; }
	 }
	if(ct>0 && ct1==ct) usleep(3000);
	if(ct==0) pthread_cond_wait(&S_condR,&S_mutxR);
      }
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

int	main_processing(struct RQ *qp)
{
static	int	*inb,opcode,owner,ws;
register int	i,l,pool_id,t_id,th_id,role,ac;
static	char	*tbuf;
static	int	ra[2]={8,1};
struct	TF	*tf;

	t_id=-1; th_id=-1;
	ws=(*qp).ws; owner=(*qp).owner;
	inb=(int *)(*qp).inb;

	role=inb[1]&0x00000f00;
	opcode=inb[1]&0x000000ff;
/* fprintf(flog,"MAIN processing opcode %i\n",opcode); */

	if(opcode==19) dbstop();

	if(opcode==14)
	    {
		ra[0]=8+initdata.acc_qty*4; send(ws,ra,8,0);
		for(i=0;i<initdata.acc_qty;i++)
		  send(ws,&ACC_LIST[i],4,0);
		return(1);
	    }
	  else
	if(opcode==15 || opcode==16)
	    {
		if(inb[0]==4) (void)modify_acc_list(inb[2],opcode);
		ra[0]=8; send(ws,ra,8,0);
		return(1);
	    }
	  else
	if(opcode==6)
	    {
		if(inb[0]==2)
		 {
		  tbuf=(char *)&inb[2];
		  if(tbuf[0]==0) E_enable=1;
		   else if(tbuf[0]==1) E_enable=0;
		   else if(tbuf[0]==2) SYNC_R=1;
		   else if(tbuf[0]==3) SYNC_R=0;
		 }

		ra[0]=8; send(ws,ra,8,0); return(1);
	    }
	else if(inb[0]>=2 && (opcode==2 || opcode==4 || opcode==7  ||
		( opcode>=20 && opcode<32 )  )  )
	 {
	  tbuf=(char *)&inb[2]; inb[0]-=2;
	  l=0;
	  for(i=0;i<initdata.t1_qty;i++)
	    {
		while(xt[l].name[0]==0 && l<100) l++;
		if(tbuf[0]==xt[l].id && tbuf[1]==xt[l].qa) break;
		l++;
	    }
	  if(i==initdata.t1_qty)
	    {
	      if(opcode==30) l=0;
	      else
		{ fprintf(flog,"MP - invalid table id (%i,%i) at %i\n",tbuf[0],tbuf[1],(int)tp);
		  return(-1);
		}
	    }
	  i=l; t_id=i; (*qp).t_id=i;

	  if(role==0) ac=1; else ac=16;
	  if(owner==0 && ( ( (xt[i].access&ac)!=0 && opcode==22) ||
		( (xt[i].access&(ac*2))!=0 && opcode==21) ||
		( (xt[i].access&(ac*4))!=0 && opcode==23) ||
		( (xt[i].access&(ac*8))!=0 && opcode==24) ) )
	    { return(-1); }

	  if(opcode>=20 && opcode<30)
	   {
	     pool_id=xt[i].pool;
	     for(i=0;i<initdata.p_qty;i++) if(POOL[i].id==pool_id) break;
	     if(i==initdata.p_qty || i>10)
		{ fprintf(flog,"MP - invalid pool id at %i\n",(int)tp);
		  return(-1);
		}
	     th_id=i-1; (*qp).pool=i;
	   }
	  else (*qp).pool=0;
	 }
	else if(opcode<20 || opcode>=30)
	 {
	   (*qp).pool=0;
	 }
	else {
		  fprintf(flog,"MP - incomplete request at %i\n",(int)tp);
		  return(-1);
	     }
	
	return(0);
}
/* - - - - - - - - - - - - - - - - - - - - - - - */
int	main_p1(struct RQ *qp)
{
register int	i,pool,pc;
struct	TF	*tf,*tfp;
	
	tf=0; pc=0;

	if((pool=(*qp).pool)==0)
	    tfp=&fs[0];
	 else tfp=&fp[pool-1][0];
	for(i=0;i<NQ;i++)
	  {
	    if( (*tfp).s==0) {  if(tf==0) tf=tfp; }
	     else pc++;
	    tfp++;
	  }
	if(pc>pool_que_max[pool]) pool_que_max[pool]=pc;
	pool_que[pool]=pc;

	   if(tf!=0)
	    {
	      (*tf).p=(*qp).p; (*tf).code=(*qp).inb[1]; (*tf).size=(*qp).inb[0];
	      (*tf).owner=(*qp).owner; (*tf).t=(*qp).t_id; (*tf).s=(*qp).ws;
	      if(pool==0) pthread_cond_signal(&S_condT);
	       else pthread_cond_signal(&S_condP[pool-1]);
	    }
	   else { fprintf(flog,"MP - trashing connections for pool %i:%i at %i\n",pool,pc,(int)tp);
		  pool_thrash[pool]++; pool_thrash_ts[pool]=tp;
		  return(-1);
		}
  
	return(0);
}
/* - - - - - - - - - - - - - - - - - - - - - - - */

int	main(int argc,char *argv[])
{

pid_t	pid;
FILE	*cfg_file;
struct	servent *port;
char    buf[100];
int     l,length,i,owner,t_ts,t_low;
struct	sigaction	act;
char	portname[30],*p;
struct	RQ	*qp;
uint64_t	ttp;
struct	timespec	ttT;

	if(argc<2)	err(1);
	bzero(path,sizeof(path)); bzero(portname,sizeof(portname));
/*
	odm_initialize();
	if((int)(odm_attr=odm_mount_class("SAMGR"))==-1) err(2);
	strcpy(odm_search,"name="); strncat(odm_search,argv[1],16);
	i=(int)odm_get_first(odm_attr,odm_search,buf);
	while(i>0)
	  {
		if(strcmp(&buf[28],"port")==0) strncpy(portname,&buf[36],30);
	 	 else
		if(strcmp(&buf[28],"s_dir")==0) strncpy(path,&buf[36],64);
		i=(int)odm_get_next(odm_attr,buf);
	  }
*/
	strcpy(buf,"/etc/db_config/db_"); strncat(buf,argv[1],10); strcat(buf,".conf");
	strcpy(portname,"cwsdata");  strncat(portname,argv[1],10);
	if((cfg_file=fopen(buf,"r"))==NULL) err(2);
	fgets(path,100,cfg_file);  p=strtok(path," \n");
	fclose(cfg_file);

	if(argc>2 && strcmp(argv[2],"-D")==0)
	  { fprintf(stderr,"Dockers special, prompt will not quit. For messages check the log at /ifxlogs\n"); }
	else
	  {
	  pid=fork();
	  if(pid<0) { fprintf(stderr,"can't start ifxdb, errno %i\n",errno); exit(1); }
		else if(pid>0) { exit(0); }
	   fclose(stdout); fclose(stdin);
           if(setsid()<0) exit(2);
	  }
	strcpy(argv[0],"ifxdb  ");
	umask(0);
/*	fclose(stdout); fclose(stdin); */
	strcpy(buf,"/ifxlogs/ifxdb_"); strncat(buf,argv[1],10); strcat(buf,".log");
	flog=freopen(buf,"a",stderr);
	setvbuf(flog,NULL,_IONBF,1024);
/*	if(setsid()<0) exit(2); */

	fprintf(flog,"instance : %s, port name :%s, directory : %s\n",
		argv[1],portname,path);

	bzero((char *)fs,sizeof(fs)); bzero((char *)fp,sizeof(fp)); 
	bzero((char *)RQ,sizeof(RQ));

	for(i=0;i<10;i++)
	 {
	   pthread_cond_init(&S_condP[i],NULL);
	   pthread_mutex_init(&S_mutxP[i],NULL);
	 }
	pthread_cond_init(&S_condT,NULL);
	pthread_mutex_init(&S_mutxT,NULL);
	pthread_cond_init(&S_condR,NULL);
	pthread_mutex_init(&S_mutxR,NULL);
	pthread_cond_init(&S_condW,NULL);
	pthread_mutex_init(&S_mutxW,NULL);

	bzero(&act,sizeof(act));
	act.sa_handler=SIG_IGN;
	sigaction(SIGPIPE,&act,0);

	port=getservbyname(portname,0); if(port==0) err(3);
	if((sock=socket(AF_INET, SOCK_STREAM,0))<0)		err(5);

	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=port->s_port;
	if(bind(sock,(struct sockaddr *)&server, lfrom)<0)	err(6);
	if(getsockname(sock,(struct sockaddr *)&server, (socklen_t *)&lfrom)<0)	err(7);
	count=0;
	dbstart();

	pthread_create(&p_read,NULL,read_processing,NULL); 
	C_enable=1;

	listen(sock,100);

	while(1)
	 { 
	   if((ws=accept(sock,(struct sockaddr *)&from,(socklen_t *)&lfrom))>0) 
	       {
		 if(C_enable==0) close(ws);
		  else
		   {
		    /*  ttp=mach_absolute_time();  */
			clock_gettime(CLOCK_REALTIME,&ttT);
			ttp=ttT.tv_sec*1000000+ttT.tv_nsec/1000;

		  /*    tp=t_ts;  */
/* fprintf(stderr,"incoming connection %i %u , time %llu\n",ws,from.sin_addr.s_addr,ttp); */
		     count++;
		     owner=1; E_enable=1;
		     owner=checkaccess(ntohl(from.sin_addr.s_addr));
		     if(owner<0) { fprintf(flog,"(M)--- direct hit from ip %u at %i\n",
				      ntohl(from.sin_addr.s_addr),t_ts); close(ws);
				 }
		      else if(owner!=1 && E_enable==0) { send(ws,rt,8,0); close(ws); }
		      else
		       {

		    for(i=0;i<RQ_SIZE;i++) if(RQ[i].ws==0) break;
		    if(i<RQ_SIZE)
		      {
			ioctl(ws,FIONBIO,&on);
			qp=&RQ[i];
			(*qp).ip=from.sin_addr.s_addr;
			(*qp).c=1;
			(*qp).bb=0;
			(*qp).ts=ttp-10;
	/*		(*qp).lr=(t_low>>16);   */
			(*qp).owner=owner;
			(*qp).code=0;
			(*qp).p=(unsigned char *)&(*qp).inb;
			bzero((*qp).p,12);
			(*qp).ws=ws;
			pthread_cond_signal(&S_condR);
		      }
		    else {  fprintf(flog,"(M)--- original trashing...at %i\n",t_ts);
			    send(ws,(char *)rt,8,0); close(ws); }

		       }
		   }
	       } /* if((ws= */
	 } /* while(1 */
}
