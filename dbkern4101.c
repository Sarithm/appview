
#include	<pthread.h>
#include	<stdio.h>
#include	<stdint.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<strings.h>
#include	<errno.h>
#include	<time.h>
#include	<sys/types.h>
#include	<sys/ioctl.h>
#include	<sys/socket.h>

#include	"dbdefv32.h"

void	*t_routine();
void	*p_routine();
void	*p_write();

extern	FILE	*flog;
pthread_t       p_thT,p_thP[10],p_thW;
extern	pthread_cond_t  S_condT,S_condP[10],S_condW;
extern	pthread_mutex_t S_mutxT,S_mutxP[10],S_mutxW; 
extern	struct TF	fs[200],fp[10][200];
int	t_p[10][20];

extern	int	NQ,C_enable,E_enable,SYNC_R;
extern	char	*P_BUFA[10],*P_BUFM[10];
extern  unsigned int	pool_w[20];
extern	unsigned int	pool_w_ts[20];
extern	unsigned int	pool_thrash[20];
extern	unsigned int	pool_thrash_ts[20];
extern	unsigned int	pool_que[20];
extern	unsigned int	pool_que_max[20];
char	LB[1000];
unsigned int	OPSTAT[9];
time_t	tp;
int	TABLE_ts;


FILE    *fmain;
FILE	*fpool;
char	path[100];
struct	pool	POOL[10];
int	in_last,last_flushT,last_flush[10];

unsigned int	R_STAT[10],W_STAT[10],H_STAT[10],C_STAT[10],A_STAT;
unsigned int	TABL_STAT,ACC_HIT_STAT,ACC_Q_STAT;

int	MAXPAGES;

struct	     
{
int			page;
unsigned short int	old;
short int		dirty;
} mmaps[10240];

struct	attr A[2];	/* - - three attributes  */

struct	initdata	initdata;

int	poolmap[10][1024];
char	*memory_pool;

short	int     t1[100];
int	ACC_LIST[200];

struct	ttc1
{
char	name[20];
int	type;
int	size;
};

struct	XT	xt[100];

struct	tt
{
char			name[20];
unsigned short int	size;
unsigned short int	id;
unsigned char 		type;
unsigned char		acc;
} x[1500];

struct uattr
{
int     *addr;
int     offset;
int     ttlc;
int     count;
char    fc;
char    *value;
};

struct	WMAP			
{
char	*B;
int	L,sock,free,used,s;
int	a1,a2;
} WMP[10];

int	buf_decode(char *,int,struct table *,struct uattr *,int, int *, char);

int	xmark_used(int i)
{
	return(mark_used(poolmap[i],1024));
}
/* - - - - - - - - - - - - - - - - - - - - - - - */

int	vmark_used(struct attr *a)
{
int	i,N;
int	*p;

	if((*a).ald==0)
		return(mark_used((int *)(*a).al,ALLOC_MAX/2)); 
	  else
		{
		for(i=0;i<ALLOC_MAX;i++)
		  if((*a).al[i]!=0 )
			{ 
			  p=(int *)translate_addr(&POOL[(int)(*a).pool].alloc,(*a).al[i],1);
			  N=mark_used(p,ALLOC_MAXL);
			  if(N>0) break;
			}
		   else 
		        { (*a).al[i] = mark_used((int *)POOL[(int)(*a).pool].alloc.al,ALLOC_MAX/2);
		 	  p=(int *)translate_addr(&POOL[(int)(*a).pool].alloc,(*a).al[i],1);
			  bzero(p,POOL[(int)(*a).pool].alloc.len);
			  N=mark_used(p,ALLOC_MAXL);
			  break;
			}
		return(N+ALLOC_MAXL*32*i);
		}
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

void	vmark_free(struct attr *a,int N)
{
register int	*p,i;

	N--;
	if((*a).ald==0)
		mark_free((*a).al,N);
	   else
		{
		i=N/8192;
		p=(int *)translate_addr(&POOL[(int)(*a).pool].alloc,(*a).al[i],1);
		mark_free(p,N%8192);
		}
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

int	lc_read(struct table *a,int N)
{
int	P,R,P1,R1;
unsigned	short	int	*p;
unsigned	int	*p1;

	if((*a).type==0) return((*a).lc[N]);
	  else
	if((*a).type==1 || (*a).type==2)
	{
	   P=N>>11; R=N&2047;
	   p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P],0);
	   return(*(p+R));
	}
	else
	  {
		P=N>>10; R=N&1023; P1=P>>11; R1=P&2047;
		p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P1],0);
		p1=(unsigned int *)map_segment((int)(*a).poolid,*(p+R1),0);
		return(*(p1+R));
	  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

void	lc_read_ln(struct table *a,int N,unsigned int *TTLC)
{
int	P,R,P1,R1,res,j,i;
unsigned short int *p;
unsigned int	   *p1;

	j=(*a).nc; N*=j;
	if((*a).type==0)
	  { for(i=0;i<j;i++) TTLC[i]=(*a).lc[N+i]; }
/*	  bcopy((char *)&((*a).lc[N]),(char *)TTLC,j*2); */
	else if((*a).type==1 || (*a).type==2)
	{
	P=N>>11; R=N&2047;
	p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P],0);
	res=R+j-2048;
	if(res>0) j-=res;
/*	bcopy(p+R,TTLC,j*2); */
	for(i=0;i<j;i++) TTLC[i]=*(p+R+i);
	if(res>0)
	   {
		p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P+1],0);
/*		bcopy(p,&TTLC[j],res*2);  */
		for(i=0;i<res;i++) TTLC[i+j]=*(p+i);
	   }
	}
	else
	{
	  P=N>>10; R=N&1023; P1=P>>11; R1=P&2047;
	  p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P1],0);	
	  p1=(unsigned int *)map_segment((int)(*a).poolid,*(p+R1),0);
	  res=R+j-1024;
	  if(res>0) j-=res;
	  for(i=0;i<j;i++) TTLC[i]=*(p1+R+i);
	  if(res>0)
	     {
		if(R1==2047)
		  {
			p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P1+1],0);
			R1=0;
		  }
		p1=(unsigned int *)map_segment((int)(*a).poolid,*(p+R1+1),0);
		for(i=0;i<res;i++) TTLC[i+j]=*(p1+i);
	     }
	}
}
/* - - - - - - - - - - - - - - - - - - - - - - - */

int	lc_write(struct table *a,int N,int NN)
{
int	P,R,P1,R1;
unsigned	short	int	*p;
unsigned	int		*p1;

	if((*a).type==0)
	    {
		P=(*a).lc[N];
		(*a).lc[N]=(unsigned short int)NN;
	    }
	 else if( (*a).type==1 || (*a).type==2)
	{

	P=N>>11; R=N&2047;
	if((*a).lc[P]==0)
		{
			(*a).lc[P]=mark_used(poolmap[(*a).poolid],1024);
			p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P],2);
			bzero(p,4096);
		}
	   else
		p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P],1);
	P=(int)*(p+R);
	*(p+R)=(unsigned short int)NN;
	}

	 else 		/* type is 3 */
	{
	  P=N>>10; R=N&1023; P1=P>>11; R1=P&2047;
	  if((*a).lc[P1]==0)
		{
			(*a).lc[P1]=mark_used(poolmap[(*a).poolid],1024);
			p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P1],2);
			bzero(p,4096);
		}
	  else p=(unsigned short int *)map_segment((int)(*a).poolid,(*a).lc[P1],1);
	  if(*(p+R1)==0)
		{
			*(p+R1)=mark_used(poolmap[(*a).poolid],1024);
			p1=(unsigned int *)map_segment((int)(*a).poolid,*(p+R1),2);
			bzero(p1,4096);
		}
	  else p1=(unsigned int *)map_segment((int)(*a).poolid,*(p+R1),1);
	  P=(int)*(p1+R); *(p1+R)=(unsigned int)NN;
	}
	return(P);
} 

/* - - - - - - - - - - - - - - - - - - - - - - - */

int	checkaccess(int ip)
{
register	int	i;
char		*p;
	
	if(ip==2130706433)	return(1);
	  else
	for(i=0;i<initdata.acc_qty;i++)
	 {
	   p=(char *)&ACC_LIST[i];
	   if(p[0]==0)
	    { if(ACC_LIST[i]==(ip&0xFFFFFF00)) return(0);
	    }
	  else 
	 if(ACC_LIST[i]==ip) return(0);
	 }
	return(-1);
}
/* - - - - - - - - - - - - - - - - - - - - - - - */
int	modify_acc_list(int v,int code)
{
register	int	i,j;

/*	bcopy(p,&v,4);  */
	for(i=0;i<initdata.acc_qty;i++) if(ACC_LIST[i]==v) break;
	if(i<initdata.acc_qty) 
	  {
	     if(code==15) return(0);
	       else if(code==16)
		 {
		    for(j=i;j<initdata.acc_qty-1;j++) ACC_LIST[j]=ACC_LIST[j+1];
		    initdata.acc_qty--;
		 }
	  }
	else
	 {
	     if(code==15 && initdata.acc_qty<200)
		{ ACC_LIST[i]=v; initdata.acc_qty++; }
	     else if(code==16) return(0);
	 }
	return(0);
}
/* - - - - - - - - - - - - - - - - - - - - - - - */ 

void	snapshot(unsigned int *ret)  
{
static	int	i;

	ret[0]=initdata.t1_qty; 
	ret[10]=0; ret[11]=0; ret[12]=0; ret[13]=0;
	ret[1]=TABL_STAT;
	ret[2]=0;
	ret[3]=A_STAT; /* actual disk pages allocated */
	ret[4]=R_STAT[0];
	ret[5]=W_STAT[0];
	ret[6]=H_STAT[0];
	ret[7]=C_STAT[0];
	ret[8]=MAXPAGES;
	ret[9]=initdata.p_qty;
	ret[10]=ACC_HIT_STAT;
	ret[13]=ACC_Q_STAT;
	
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

struct	table *find_table_s(char *buf)
{
static	unsigned short int T;
static	struct	table *tbase;
static	int	Q;
	
	T=(unsigned short int)*(buf++); Q=(int)*buf;
	tbase=(struct table *)read_attr(&A[1],T,0);
	if((*tbase).nc==Q) return(tbase);
	return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

int	predicate(struct table *tbase,int l,struct uattr *Uattr,unsigned int *TTLC)
{
int	NP,j,rcc,cmpyes,cc;
struct	attr *abase;

	for(j=0;j<l;j++)
	   {
		NP=TTLC[Uattr[j].offset];
		if(NP!=0)
		   {
			abase=(struct attr *)Uattr[j].addr;
			rcc=cmpr_attr(abase,NP,Uattr[j].value);
			if(rcc<0) rcc=1; else if (rcc>0) rcc=-1;
			if(Uattr[j].fc>69) cc=71; else cc=61;	
			if(rcc+Uattr[j].fc==cc)
			  {  cmpyes=1; if(cc==71) break; }
			  else { cmpyes=0; if(cc==61) break; }
		   }
		  else { cmpyes=0; break; }
	    }
	return(cmpyes);

}

/* - - - - - - - - - - - - - - - - - - - - - - - */

int	empty_line(struct table *tbase,int i,unsigned int *TTLC)
{
int	k;

	lc_read_ln(tbase,i,TTLC);
	for(k=0;k<(*tbase).nc;k++)
	  if(TTLC[k]!=0) return(1);
	return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

void	move_line(struct table *tbase,int i,int j)
{
int	k;

	for(k=0;k<(*tbase).nc;k++)
	   lc_write(tbase,(*tbase).nc*i+k,lc_write(tbase,(*tbase).nc*j+k,0));
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

int	fflush_memory()
{

        fseek(fmain,0,SEEK_SET);
        fwrite(&initdata,sizeof(initdata),1,fmain);
        fseek(fmain,initdata.attr_offset,SEEK_SET);
        fwrite(A,sizeof(struct attr),initdata.attr_qty,fmain);
        fseek(fmain,initdata.t1_offset,SEEK_SET);
        fwrite(t1,sizeof(t1),1,fmain);
	fwrite(ACC_LIST,sizeof(ACC_LIST),1,fmain);
	fseek(fmain,initdata.t2_offset,SEEK_SET);
	fwrite(POOL,sizeof(POOL)/10,initdata.p_qty,fmain);
	fflush(fmain);
	return(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - */
void	fflush_pool(i)
{
int	j;

	fseek(POOL[i].f,0,SEEK_SET);
	fwrite((char *)poolmap[i],4096,1,POOL[i].f);

	for(j=POOL[i].map;j<POOL[i].mpages;j++)
	  if(mmaps[j].page!=0 && mmaps[j].dirty>0)
  		{
			
			fseek(POOL[i].f,4096*mmaps[j].page,SEEK_SET);
			fwrite(memory_pool+(j<<12),4096,1,POOL[i].f);
			mmaps[j].dirty=0;
			W_STAT[i]++;
  		}
	fflush(POOL[i].f);
	/* fprintf(flog,"(F)--- flushed pool %i at %i\n",i,tp); */
}

/* - - - - - - - - - - - - - - - - - - - - - - - */

char	*map_segment(int pool,int page,int w)
{
int	i,imax,mmax,HN,ifree,ihit,mfree;


	HN=page%POOL[pool].hi;
	C_STAT[pool]++; ifree=-1; ihit=-1; mfree=0; mmax=-1;

	for(i=POOL[pool].map+HN;i<POOL[pool].mpages;i+=POOL[pool].hi)
	   if(mmaps[i].page==0) ifree=i;
	      else
		if(mmaps[i].page==page)
		  {
			H_STAT[pool]++; 
			if(w>0) mmaps[i].dirty=w;
			ihit=i;
		  }
		  else
		    {
			mmaps[i].old++;
			if(ifree<0 && mmaps[i].old>mmax)
			  { mmax=mmaps[i].old; mfree=i; }
		    }
	if(ihit>=0)  return(memory_pool+(ihit<<12));
	if(ifree<0)
	  {
		if(mfree<0)
			{
				fprintf(flog,"ifree<0\n");
				fflush_memory();
				exit(10);
			}
		  else ifree=mfree;
		if(mmaps[ifree].dirty>0)
		 {
                      fseek(POOL[pool].f,mmaps[ifree].page<<12,SEEK_SET);
                      fwrite(memory_pool+(ifree<<12),4096,1,POOL[pool].f);
		      W_STAT[pool]++;
		 }
	  }
	  
	mmaps[ifree].page=page;	mmaps[ifree].old=0;
	mmaps[ifree].dirty=w;
	if(w<2)
	   {	
		fseek(POOL[pool].f,page<<12,SEEK_SET);
		fread(memory_pool+(ifree<<12),4096,1,POOL[pool].f);
		R_STAT[pool]++;
	   }
	return(memory_pool+(ifree<<12));
}

/* - - - - - - - - - - - - - - - - - - - - - - */

void	generate_table_list()
{
static	int	i,j,jj;
static	struct	table	*tbase;
static	struct	attr	*abase;
static	unsigned short int *T;

	TABL_STAT++;
	T=(unsigned short int *)t1; jj=0;
	for(i=0;i<initdata.t1_qty;i++)
	  {
		while(*T==0) T++;
		tbase=(struct table *)translate_addr(&A[1],*T,0);
		bcopy((*tbase).name,xt[i].name,20);
		xt[i].tbase=tbase;
		xt[i].id=(unsigned char)*T;
		xt[i].qa=(*tbase).nc;
		xt[i].pool=(*tbase).poolid;
		xt[i].type=(*tbase).type;
		xt[i].access=(*tbase).access;
		xt[i].in=jj;
		if(xt[i].ts==0) xt[i].ts=tp;
		for(j=0;j<(*tbase).nc;j++)
		 { 
		    abase=(struct attr *)translate_addr(&A[0],(int)(*tbase).attrs[j],0);
		    x[jj].type=(unsigned char)(*abase).type;
		    x[jj].size=(unsigned short int)(*abase).len;
		    x[jj].acc=(unsigned char)(*abase).access;
		    x[jj].id=(unsigned short int)(*tbase).attrs[j];
		    bcopy((*abase).name,x[jj].name,20);
		    jj++;
		 } T++;
	  }
	TABLE_ts=tp; in_last=jj;

}

/* - - - - - - - - - - - - - - - - - - - - - - */

int	create_table(struct ttc1 *y,int L,char *name)
{
static	char	ald;
static	int	i,l,N;
static	struct	table	base,*tbase;
	
	bzero((char *)&base,sizeof(base));
	base.nr=0; base.nc=L; base.poolid=*(name+20);
	base.type=*(name+21); strcpy(base.name,name);
	base.access=0;
	if(base.type==0|| base.type==1) ald=0; else ald=1;
	 
	for(i=0;i<initdata.p_qty;i++)
	  if(POOL[i].id==base.poolid) break;
	if(i==initdata.p_qty) return(-1);
	
/*	for(l=0;l<initdata.t1_qty;l++) if(xt[l].name[0]==0) break; */
	initdata.t1_qty++;
	i=0; while(t1[i]!=0) i++; 
	N=write_attr(&A[1],(char *)&base); t1[i]=N;
	tbase=(struct table *)read_attr(&A[1],N,0);
/*	xt[l].tbase=tbase; xt[l].id=(char)N;
	bcopy((*tbase).name,xt[l].name,20);
	xt[l].qa=(*tbase).nc; xt[l].pool=(*tbase).poolid;
	xt[l].type=(*tbase).type; xt[l].access=0; xt[l].ts=tp; */
	for(i=0;i<L;i++)
	    {
		(*tbase).attrs[i]=create_attr(y[i].name,
		   y[i].type,y[i].size,base.poolid,ald);
/*		x[l][i].type=y[i].type; x[l][i].size=y[i].size;
		x[l][i].acc=0; x[l][i].id=(*tbase).attrs[i];
		bcopy(y[i].name,x[l][i].name,20); */
	    }
	
	translate_addr(&A[1],N,1);
	generate_table_list();
	bzero((char *)xt[l].ST,20);
	return(0);

}

/* - - - - - - - - - - - - - - - - - - - - - - */

int	create_attr(char *name,char type,int size,char pool,char ald)
{
static	int	N,rc;
static	struct	attr base;

	bzero(&base,sizeof(base));
	strcpy(base.name,name);
	base.type=type;
	base.len=size;
	base.ald=ald;
	base.qty=4096/size;
	base.pool=pool;
	N=write_attr(&A[0],(char *)&base);
	return(N);
}

/* - - - - - - - - - - - - - - - - - - - - - - */

int	create_pool(char *buf)
{
static	int	i,id,hi,mpages;
char	file_name[100];

	id=(unsigned int)buf[0];
	for(i=0;i<initdata.p_qty;i++)
		if(POOL[i].id==id) return(-1);
	bzero((char *)&POOL[i],sizeof(struct pool));
	bzero((char *)poolmap[i],4096);
	POOL[i].id=id; POOL[i].ihi=(unsigned int)buf[1];
	bcopy(buf+2,&POOL[i].ipages,4);
	POOL[i].alloc.len=1024;
	POOL[i].alloc.type=10;
	POOL[i].alloc.qty=4;	
	POOL[i].alloc.ald=0;
	POOL[i].alloc.pool=id;
	sprintf(file_name,"%sdata.%i",path,POOL[i].id);
	POOL[i].f=fopen(file_name,"wb");
	fwrite((char *)poolmap[i],4096,1,POOL[i].f);
	initdata.p_qty++;
	return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - */
void	deallocate_page(int pool_id,int N)
{
int	j;

	mark_free(poolmap[pool_id],N-1);
	for(j=POOL[pool_id].map;j<POOL[pool_id].mpages;j++)
		if(mmaps[j].page==N) {  mmaps[j].page=0; break; }
}
/* - - - - - - - - - - - - - - - - - - - - - - */
int	send_to_mem(int K,char *buf,int size)		
{
int	free_size,L,free,used;
	
	if(WMP[K].a2==-2) return(-2);	
	if(size<=0 || size>32678) return(-1);
	free=WMP[K].free; used=WMP[K].used; L=WMP[K].L;
	if(used>free) free+=L;
	free_size=L-(free-used);
	if(free_size<0)  return(-4); 
	if(size>free_size-10)  return(-3);
	while(size>0)
	 {
	   WMP[K].B[free%L]=*(buf++);
	   free++;
	   size--;
	 }
	WMP[K].free=free%L;
	pthread_cond_signal(&S_condW);
	return(0);
}
/* - - - - - - - - - - - - - - - - - - - - - - */
void	*p_write()				
{
int	K,sock,L,i,rc;
unsigned int	used,free;
char	*p;

	pthread_mutex_lock(&S_mutxW);
	while(1)
	 {
	   for(K=0;K<initdata.p_qty-1;K++) if(WMP[K].a2!=0) break;
	   if(K==initdata.p_qty-1) pthread_cond_wait(&S_condW,&S_mutxW);
	   for(K=0;K<initdata.p_qty-1;K++)
	    if(WMP[K].a2==1)
	     {

		sock=WMP[K].sock;
		if(sock>0)
		  {
		L=WMP[K].L;
		  
	        if(WMP[K].s==0)
		 {
		   if(WMP[K].a1==0 && WMP[K].used==WMP[K].free)
		    {
/* fprintf(flog,"(W) -free/used %i %i\n",WMP[K].free,WMP[K].used); */

		      close(sock); WMP[K].sock=0; 
	 	      WMP[K].a2=0;
		    }
		   else
		    {
		      used=WMP[K].used; free=WMP[K].free;
		      if(used>free)  free+=L; 
		      WMP[K].s=free-used; if(WMP[K].s>2048) WMP[K].s=2048;
		      if(WMP[K].s>L-used) WMP[K].s=L-used;
		    }
		 }
		if(WMP[K].s>0)
		 { 
		   p=&WMP[K].B[WMP[K].used];
		   rc=send(sock,p,WMP[K].s,0);	
		   if(rc>0)
		        {  /*
			  if(WMP[K].s!=rc)
			   {
			      fprintf(flog,"(W) - partial write %i %i %i at %i\n",K+1,rc,WMP[K].s,(int)tp);
			   } */
			  WMP[K].s-=rc;  WMP[K].used=(WMP[K].used+rc)%L;
			}
		     else if(errno!=11)
			{ fprintf(flog,"(W) - aborting %i %i at %i\n",K+1,errno,(int)tp); 
			   pool_w[K+1]++; pool_w_ts[K+1]=tp;
			   WMP[K].a2=2; WMP[K].sock=0; WMP[K].s=0; 
			  close(sock);
		        }
			else  {}
		/*       usleep(2000); */
		 }
		}  /* if(sock>0 */
		else { WMP[K].a2=0;
		 fprintf(flog,"(W) 0 socket for pool %i\n",K+1); }
	     }

	 } /* while(1 */
}
/* - - - - - - - - - - - - - - - - - - - - - - */

void dbstart()
{
int	i;
char	file_name[100],*p;

	bzero(A,sizeof(A)); bzero(file_name,sizeof(file_name));
	bzero(POOL,sizeof(POOL)); bzero(poolmap,sizeof(poolmap));
	bzero(t_p,sizeof(t_p)); bzero((char *)xt,sizeof(xt));	
	bzero((char *)LB,sizeof(LB));
	TABLE_ts=0; in_last=0;

	fprintf(flog,"path: %s\n",path);

	strcpy(file_name,path); strcat(file_name,"maindata");
	fmain=fopen(file_name,"rb+");
		if(fmain==0) err(8);

	if(fread(&initdata,sizeof(initdata),1,fmain) != 1) err(9);

	if(initdata.attr_qty<=0) err(10);
	fprintf(flog,"attibutes to read : %i\n",initdata.attr_qty);

	fseek(fmain,initdata.attr_offset,SEEK_SET);
	i=fread(A,sizeof(struct attr),initdata.attr_qty,fmain);
	if(i != initdata.attr_qty) err(11);

	fprintf(flog," pools - %i, tables - %i, attributes - %i\n",
	initdata.p_qty,initdata.t1_qty,initdata.attr_qty);
	
	fseek(fmain,initdata.t1_offset,SEEK_SET);
	if(fread(t1,sizeof(t1),1,fmain) !=1) 	err(11); 
	if(fread(ACC_LIST,sizeof(ACC_LIST),1,fmain)!=1)	err(11);
	if(initdata.acc_qty<0 || initdata.acc_qty>=200)	err(11);

	fseek(fmain,initdata.t2_offset,SEEK_SET);
	if(fread(POOL,sizeof(struct pool),initdata.p_qty,fmain) != initdata.p_qty)
						err(11);

	MAXPAGES=0;
	for(i=0;i<initdata.p_qty;i++)
	  {
		POOL[i].map=MAXPAGES;
		POOL[i].hi=POOL[i].ihi;
		MAXPAGES+=POOL[i].ipages;
		POOL[i].mpages=POOL[i].ipages+POOL[i].map;
		sprintf(file_name,"%sdata.%i",path,POOL[i].id);
		fprintf(flog,"pool id : %i (%i:%i), file name : %s : %i %i\n",
			POOL[i].id,POOL[i].ipages,POOL[i].hi,file_name,
			POOL[i].map,POOL[i].mpages);
		if((POOL[i].f=fopen(file_name,"rb+"))==0) err(12);
		if(fread((char *)poolmap[i],4096,1,POOL[i].f)!=1) err(12);
	  }

	memory_pool=(char *)malloc(MAXPAGES*4096);
        bzero(memory_pool,sizeof(MAXPAGES*4096));
        for(i=0;i<MAXPAGES;i++)
        {
                mmaps[i].dirty=0;
		mmaps[i].page=0;
		mmaps[i].old=0;
        }

	p=(char *)malloc((initdata.p_qty-1)*2097152);	/* 1048576  or 2097152 */
	for(i=0;i<initdata.p_qty-1;i++)			
	 {
		WMP[i].L=2097152;		/* 1024 * 1024 */
		WMP[i].B=p+i*WMP[i].L;
		WMP[i].free=0; WMP[i].used=0; WMP[i].s=0;
		WMP[i].sock=0; WMP[i].a1=0; WMP[i].a2=0;
	 }

	p=(char *)malloc(initdata.p_qty*16384);
	for(i=0;i<initdata.p_qty;i++)
	 {
	   P_BUFA[i]=p+i*16384;
	   P_BUFM[i]=&LB[i*60];
	 }

	pthread_create(&p_thT,NULL,t_routine,NULL);
	for(i=0;i<initdata.p_qty-1;i++) pthread_create(&p_thP[i],NULL,p_routine,(void *)(intptr_t)i);
	pthread_create(&p_thW,NULL,p_write,NULL);	
	sleep(1);
	tp=time(&tp);

/*
		
	R_STAT=0; W_STAT=0; H_STAT=0; C_STAT=0; A_STAT=0;
	TABL_STAT=0; ACC_HIT_STAT=0; ACC_Q_STAT=0;

*/
	generate_table_list();
	C_enable=1;  sleep(5); E_enable=1;
}

void dbstop()
{
int	i;

	tp=time(&tp);
	fprintf(flog,"(S)--- stopping database engine at %i\n",(int)tp);
	C_enable=0;
	sleep(20);
	for(i=0;i<initdata.p_qty-1;i++)
	    {
		last_flush[i]=0;
		pthread_cond_signal(&S_condP[i]);
		sleep(5);
		if(last_flush[i]>0) fprintf(flog,"(S)--- pool %i was flushed\n",i+1);
	    }
	last_flushT=0;
	pthread_cond_signal(&S_condT);
	sleep(5);
	if(last_flushT>0) fprintf(flog,"(S) --- pool 0 was flushed\n");
	fflush_memory();
	fclose(fmain);
	for(i=0;i<initdata.p_qty;i++)
	fclose(POOL[i].f);
	sleep(1);
	exit(0);
}
/* - - - - - - - - - - - - - - - - - - - - - - -*/

void	*t_routine()
{
int	K,i,j,l,NN,RL,sock,todo,NB,owner,p_id,role,version;
int	ct,rc,il,i1,jj,t_id;
time_t	tp1;
unsigned short int	attrid;
struct	table	*tbase;
struct	attr	*abase;
struct timespec S_timeT;
struct 
{
int     len;
int     code;
char    text[2048];
} rettable;
char	*buf,*p,ald;


	fprintf(flog,"thread T started\n");

	pthread_mutex_lock(&S_mutxT);

	while(1)
	 {
	  tp1=time(&tp1); tp=tp1;
	  if(tp1-last_flushT>300) {
		 fflush_memory(); fflush_pool(0); last_flushT=tp1; }
	  ct=0;
	  for(K=0;K<NQ;K++)
	   {
	     if((sock=fs[K].s)>0)		/* X loop */
	      {  
		ct++;
		todo=fs[K].code;  NB=fs[K].size;  owner=fs[K].owner; t_id=fs[K].t;
		buf=fs[K].p;

		version=(todo&0x000f0000)>>16;
		role=todo&0x00000f00;
		todo=todo&0x000000ff; rettable.len=8;

		if (todo==3 || ( (todo==8 || todo==11)&&(owner!=0) ) )
		 {
		  l=0;
                  for(i=0;i<initdata.t1_qty;i++)
                   {
		   while(xt[l].name[0]==0 && l<100) l++;
                   if(NB==0 || strncmp(buf,xt[l].name,20)==0)
                    {
                     bzero(&rettable,sizeof(rettable));
                     strcpy(rettable.text,xt[l].name);
		     p=&rettable.text[20]; rettable.code=2;
		     if(todo==11)
			{
			  *(p++)=xt[l].pool; *(p++)=xt[l].type;
			  bcopy((char *)xt[l].ST,p,20);
			  rettable.len=50;
			}
		     else 
		     {
		    rettable.len=29+26*xt[l].qa;
                    *(p++)=xt[l].id;
		    if(todo>3)
		      { 
			*(p++)=xt[l].pool; *(p++)=xt[l].type;
			*(p++)=xt[l].access; rettable.len+=3;
		      }
                    bcopy(&x[xt[l].in],p,26*xt[l].qa);
		     }
                    write(sock,&rettable,rettable.len);
                    if(NB>0) break;
                   }
		   l++;
                  }
                if(NB==0 && todo<=8)
                   {
                        snapshot((unsigned int *)&rettable.text[0]);
                        rettable.len=64;
                   } else rettable.len=8;
          }

		else if (todo==5)	/* create pool  */
		 {
		   create_pool(buf);
		 }

		else if (todo==9)	/* get pool stat */
		 {
		   p=rettable.text;
		   for(i=0;i<initdata.p_qty;i++)
		     {
			p_id=POOL[i].id;
			*(p++)=POOL[i].id;
			*(p++)=POOL[i].ihi;
			bcopy(&POOL[i].ipages,p,4);	p+=4;
			bcopy(&C_STAT[p_id],p,4);	p+=4;
			bcopy(&R_STAT[p_id],p,4);	p+=4;
			bcopy(&W_STAT[p_id],p,4);	p+=4;
			bcopy(&H_STAT[p_id],p,4);	p+=4;
			NN=0;
		 	for(j=POOL[i].map;j<POOL[i].mpages;j++)
			  if(mmaps[j].page!=0) NN++;
			bcopy(&NN,p,4);			p+=4;
			bcopy(&pool_thrash[i],p,4);	p+=4;
			bcopy(&pool_thrash_ts[i],p,4);	p+=4;
			bcopy(&pool_w[i],p,4);		p+=4;
			bcopy(&pool_w_ts[i],p,4);	p+=4;
			bcopy(&pool_que[i],p,4);	p+=4;
			bcopy(&pool_que_max[i],p,4);	p+=4;
		     }
		   rettable.len=8+50*initdata.p_qty;
		 }

		else if(todo==10)	/* update pool parameters */
		 {
		   for(i=0;i<initdata.p_qty;i++)
			if(POOL[i].id==buf[0]) break;
		   if(i<initdata.p_qty)
		    {	POOL[i].ihi=buf[1];
			bcopy(&buf[2],&j,4);
			if(j>0 &&j<=512) POOL[i].ipages=j;
		    }
		 }

		else if(todo==1)	/* create table */
		 {
		   NN=(NB-22)/28;
		   create_table((struct ttc1 *)(buf+22),NN,buf);
		   TABLE_ts=tp;
		 }

		else if(todo==2)	/* delete table */
		 {
		   l=t_id; TABLE_ts=tp;
/*
		   for(i=0;i<initdata.t1_qty;i++)
		    {  
		     while(xt[l].name[0]==0 &&l<100) l++;
		     if(buf[0]==xt[l].id && buf[1]==xt[l].qa) break;
		     l++;
		    }
		   if(i<initdata.t1_qty)
		    { */
		     i=l; xt[i].name[0]=0; initdata.t1_qty--;
		     p_id=xt[i].pool-1;
		     for(j=0;j<20;j++) if(t_p[p_id][j]==0) break;
		     if(j<20) t_p[p_id][j]=i|0x02000000;
		     pthread_cond_signal(&S_condP[p_id]);
		 /*   } */
		 }

		else if(todo==4)	/* add attribute */
		 {
		   l=t_id; TABLE_ts=tp;
/*
		   for(i=0;i<initdata.t1_qty;i++)
		    {
			while(xt[l].name[0]==0 &&l<100) l++;
			if(buf[0]==xt[l].id && buf[1]==xt[l].qa) break;
			l++;
		    }
		   if(i<initdata.t1_qty)
		    {  */
		      i=l;
		      p_id=xt[i].pool-1; tbase=xt[i].tbase; j=xt[i].qa;
		      bcopy(buf+20,&NN,4); bcopy(buf+24,&RL,4);
		      if(xt[i].type==0 || xt[i].type==1) ald=0; else ald=1;
		      xt[i].ts=tp;
		      (*tbase).attrs[j]=create_attr(buf,NN,RL,p_id+1,ald);
		
		      jj=xt[i].in;
		      for(i1=0;i1<j;i1++)
		       {
			 bcopy((char *)&x[jj+i1],(char *)&x[in_last+i1],sizeof(struct tt));
		       }
		      xt[i].in=in_last; in_last+=(j+1);
		      jj=xt[i].in+j;
		      x[jj].type=NN; x[jj].size=RL; x[jj].acc=0;
		      x[jj].id=(unsigned short int)(*tbase).attrs[j];
		      bcopy(buf,x[jj].name,20);
		       

		/* wake up pool thread */
		      for(j=0;j<20;j++) if(t_p[p_id][j]==0) break;
		      if(j<20) t_p[p_id][j]=i|0x01000000;
		      pthread_cond_signal(&S_condP[p_id]);
/*		    } */
		 }

		else if(todo==30 || todo==31)	/* misc table parameters */
		 {
		l=t_id;
/*
		for(i=0;i<initdata.t1_qty;i++)
		 {
		   while(xt[l].name[0]==0 &&l<100) l++;
		   if(buf[0]==xt[l].id && buf[1]==xt[l].qa) break;
		   l++;
		 }
		if(i<initdata.t1_qty)
		   { */
			i=l;
			if(todo==30) j=xt[i].ts;
			 else j=(*(xt[i].tbase)).nr;
		/*   } 
		else j=-1; */
		bcopy(&j,&rettable.text[0],4);
		bcopy(&tp,&rettable.text[4],4);
		bcopy(&TABLE_ts,&rettable.text[8],4);
		rettable.len=20;
		 }

		else if(todo==7)	/* change table and attr acls */
		 {
		   l=t_id;
/*
		   for(i=0;i<initdata.t1_qty;i++)
		    {
			while(xt[l].name[0]==0 &&l<100) l++;
			if(buf[0]==xt[l].id && buf[1]==xt[l].qa) break;
			l++;
		    }
		   if(i<initdata.t1_qty)
		    { */
			i=l;
			if(NB==4)
			 {
			   (*(xt[i].tbase)).access=buf[3];
			   xt[i].access=buf[3];
			   translate_addr(&A[1],xt[i].id,1);
			 }
			else if(NB==6 && buf[2]<xt[i].qa)
			 {
			   j=buf[2];
			   p=(char *)&attrid; *p=buf[3]; *(p+1)=buf[4];
			   if(attrid==x[xt[i].in+j].id)
			    {
			      x[xt[i].in+j].acc=buf[5];
			      abase=(struct attr *)translate_addr(&A[0],(int)attrid,1);
			      (*abase).access=buf[5];
			    }
		         }
/*		    } */

		 }
		else { fprintf(flog,"T thread invalid opcode %i at %i\n",todo,tp1); }
 
		rettable.code=1;
		send(sock,&rettable,rettable.len,0);

		if(NB>0)
		 {
		   release_buf((char *)buf,0);
		 }
		fs[K].code=0; fs[K].size=0; fs[K].t=0; fs[K].owner=0; fs[K].s=0;
		close(sock);

	      } /* ending loop X */
	     } /* for(K= */
	    if(ct==0)
		{  S_timeT.tv_sec=tp1+60; S_timeT.tv_nsec=0;
		   rc=pthread_cond_timedwait(&S_condT,&S_mutxT,&S_timeT); 
		}

	  } /* while(1  */
}
/* - - - - - - - - - - - - - - - - - - - - - - - */
void    *p_routine(void *IP)
{
time_t		tp1;
uint64_t	ttp1,ttp2;
struct	timespec	ttT;
struct	timespec	S_timeP;
unsigned int	TTLC[100];
struct	uattr	Uattr[200];
struct	table	*tbase;
struct	attr	*abase;
int     K,I,i,j,l,jj,NN,sock,todo,NB,owner,k,t_id,sec,nsec,msec,ct;
int	uw,cmpyes,rc,badrccount,UF1,*UF2,RL,off=0;
int	table_max_size,transmitted,sleep_value,pool_id,role,version;
unsigned short int	*lc_p; /* used for large tables deallocation */
int	L[40],inscount[20];
struct
{
int     len;
int     code;
char    text[4096];
} rettable;
char    *ibuf,*p,*pp,*buf,role_a;

	I=(intptr_t)IP; last_flush[I]=0; pool_id=POOL[I+1].id;

        fprintf(flog,"thread P(%i) started, pool id:%i\n",I,pool_id);

        pthread_mutex_lock(&S_mutxP[I]);

        while(1)
	 {
	   ct=0; tp1=time(&tp1);
	
	   if(tp1-last_flush[I]>300) { fflush_pool(pool_id); last_flush[I]=tp1; }
	
	for(i=0;i<20;i++)
	  if((NN=t_p[I][i])!=0)
	    {
	      t_id=NN&0xffff;
	      tbase=xt[t_id].tbase;
	      if((NN&0x01000000)!=0)		/* add attribute completion */
	       {
		for(j=(*tbase).nr-1;j>0; j--)
		  {
		     lc_write(tbase,(*tbase).nc*(j+1)+j,0);
		     for(jj=(*tbase).nc-1;jj>=0;jj--)
		        {
			   NN=(*tbase).nc*j+jj;
			   lc_write(tbase,NN+j,lc_read(tbase,NN));
			}
		  }
		lc_write(tbase,(*tbase).nc,0);
		(*tbase).nc++; xt[t_id].qa=(*tbase).nc;
		translate_addr(&A[1],xt[t_id].id,1);
	       }
	      else if((NN&0x02000000)!=0)	/* drop table completion */
	       {
		for(j=0;j<(*tbase).nc;j++)
		  {
			abase=(struct attr *)translate_addr(&A[0],(int)(*tbase).attrs[j],0);
			for(l=0;l<PAGES_MAX;l++)
			 {
			  if((*abase).lc[l]!=0)
				deallocate_page(pool_id,(int)(*abase).lc[l]);
			 }
			if((*abase).ald!=0)
			  {
			    for(l=0;l<ALLOC_MAX;l++)
			      if((*abase).al[l]!=0)
				vmark_free(&POOL[pool_id].alloc,(*abase).al[l]);
			  }
			vmark_free(&A[0],(int)(*tbase).attrs[j]);
		  }
		 if((*tbase).type>0)
		  {
		    for(l=0;l<T_MAX;l++)
		     {
		      if((*tbase).lc[l]!=0)
			{
			  if((*tbase).type==3)
			    {
				lc_p=(unsigned short int *)map_segment(pool_id,(*tbase).lc[l],0);
				for(jj=0;jj<1024;jj++)
				  if(lc_p[jj]!=0) deallocate_page(pool_id,(int)lc_p[jj]);
			    }
			  deallocate_page(pool_id,(int)(*tbase).lc[l]);
			}
		     }
			
		  }
		vmark_free(&A[1],(int)xt[t_id].id);
		for(l=0;l<100;l++) if(t1[l]==xt[t_id].id) { t1[l]=0; break; }
	       }
	      t_p[I][i]=0;
	    }


	   for(K=0;K<NQ;K++)
	    {
	     if((sock=fp[I][K].s)>0)	/* starting loop X */
	      {
		ct++;

		todo=fp[I][K].code;  NB=fp[I][K].size;  owner=fp[I][K].owner;
		version=(todo&0x000f0000)>>16; t_id=xt[fp[I][K].t].id;
		role=todo&0x00000f00;
		todo=todo&0x000000ff; 
		ibuf=fp[I][K].p;

		if(todo==22 && WMP[I].a2==1) {  continue; }
/*
	if(todo==22)
	 {
	  for(i=1;i<NQ;i++)
	   {  j=(K+i)%NQ;
	      if((fpcode[I][j]&0x0ff)==todo && xt[fpt[I][j]].id==t_id)
		{
		  fprintf(flog,"record %i %i %i\n",K,j,t_id);
		}
	   }
	  }

*/
	buf=ibuf; /* bzero(ibuf,sizeof(ibuf)); */
	tbase=xt[fp[I][K].t].tbase;
	if(todo>=21 && todo<=24) xt[fp[I][K].t].ST[todo-21]++;

	rc=100;

	if(SYNC_R!=0) ioctl(sock,FIONBIO,&off);

	if((*tbase).type==0) { table_max_size=T_MAX/(*tbase).nc;
			       if(table_max_size>1023) table_max_size=1023; }
	 else if((*tbase).type==1) table_max_size=1023;
	 else if((*tbase).type==2) { table_max_size=T_MAX*2048/(*tbase).nc;
				if(table_max_size>65535) table_max_size=65535; }
	 else table_max_size=524287;

	/* ttp1=mach_absolute_time(); */
	clock_gettime(CLOCK_REALTIME,&ttT);

	i=0; role_a=0;
	if(todo==22) { i=1; role_a=5; } else if(todo==21 || todo==23) role_a=10;
	if(owner==1) role_a=0; else if(role==0) role_a=role_a&3; else role_a=role_a&12;

	if(todo==21 || todo==23 || todo==24) xt[fp[I][K].t].ts=tp1;

	k=buf_decode(buf,NB,tbase,Uattr,i,L,role_a);
/* fprintf(flog,"P THREAD k/L %i, %i %i, %i %i, %i %i\n",k,L[0],L[1],L[2],L[3],L[4],L[5]); */

	if(k>=0 && xt[fp[I][K].t].name[0]!=0)
	{

	if (todo==23 || todo==21)  /* update record - insert record */
         {
		rettable.code=1; rettable.len=8;
		send(sock,(char *)&rettable,rettable.len,0);
		close(sock);

                uw=0; l=0; for(jj=0;jj<k;jj++) { inscount[jj]=0; l+=L[jj*2+1]; }

                if(todo==23 || l>0)
                {
                  for(i=0;i<(*tbase).nr;i++)
                   {
		     lc_read_ln(tbase,i,TTLC); l=0;
		     for(jj=0;jj<k;jj++)
		      {
                        if(L[jj*2+1]>0)
                                cmpyes=predicate(tbase,L[jj*2+1],&Uattr[l+L[jj*2]],TTLC);
			  else	if(todo==23) cmpyes=1; else cmpyes=-1;

                        if(cmpyes==1)
                        {
                          inscount[jj]++;
                          for(j=0;j<L[jj*2];j++)
                           {
                                NN=TTLC[Uattr[l+j].offset];
                                abase=(struct attr *)Uattr[l+j].addr;
                                if(NN!=0)
                                 {
                                  if(Uattr[l+j].fc==1 && (*abase).type==3)
                                   {
                                    UF2=(int *)translate_addr(abase,NN,1);
                                    bcopy(Uattr[l+j].value,&UF1,4);
                                    (*UF2)+=UF1;
                                   }
                                  else
                                   update_attr(abase,NN,Uattr[l+j].value);
                                 }
                                else
                                 {
                                  lc_write(tbase,(*tbase).nc*i+Uattr[l+j].offset,write_attr(abase,Uattr[l+j].value));
                                  uw++;
                                  Uattr[l+j].count++;
                                 }

			   }
                        }
		       l+=L[jj*2]+L[jj*2+1];
                      }
		    } /* for(i=0;i< */
                }


         if(todo==21 && (*tbase).nr<table_max_size )
                { l=0;
		  for(jj=0;jj<k;jj++)
		   {
		     if(inscount[jj]==0)
		      {
                        uw++;
                        for(j=0;j<L[jj*2]+L[jj*2+1];j++)
			 {
				abase=(struct attr *)Uattr[l+j].addr;
				NN=write_attr(abase,Uattr[l+j].value);
				lc_write(tbase,(*tbase).nr*(*tbase).nc+Uattr[l+j].offset,NN);
				Uattr[l+j].count++;
                         }
                        (*tbase).nr++;
		      }
		     l+=L[jj*2]+L[jj*2+1];
		   }
                }
	 l=0; for(jj=0;jj<k;jj++)
		{
		  for(j=0;j<L[jj*2]+L[jj*2+1]; j++)
		    if(Uattr[l+j].count!=0) translate_addr(&A[0],Uattr[l+j].ttlc,1);
		  l+=L[jj*2]+L[jj*2+1];
		}
         if(uw!=0) translate_addr(&A[1],t_id,1);

/*	rettable.code=1;	*/
         }

	else if(todo==22)
	 {
		transmitted=0; sleep_value=0; badrccount=0;
		WMP[I].sock=sock;	
		WMP[I].free=0; WMP[I].used=0; WMP[I].s=0;
		WMP[I].a1=1; WMP[I].a2=1;
		rc=0;

                pp=rettable.text;
                rettable.code=2;

                for(i=0;i<(*tbase).nr;i++)
                     {
                        lc_read_ln(tbase,i,TTLC);
                        if(L[1]>0)
                             cmpyes=predicate(tbase,L[1],&Uattr[L[0]],TTLC);
			  else cmpyes=1;

                        if(cmpyes==1)
                        {
                          rettable.len=8; pp=rettable.text;
                          for(j=0;j<L[0];j++)
                          {
                                NN=TTLC[Uattr[j].offset];
                                if(NN!=0)
                                {
                                  *(pp++)=(char)Uattr[j].offset;
                                  abase=(struct attr *)Uattr[j].addr;
                                  rc=copy_attr(abase,NN,pp); pp+=rc;
                                  rettable.len+=rc+1;
                                }
                          }
			  transmitted+=rettable.len; badrccount=0;
			  while((rc=send_to_mem(I,(char *)&rettable,rettable.len))==-3	
				&& (++badrccount)<=500 )
			   {

                        /*  rc=write(sock,&rettable,rettable.len); */
			     sleep_value=sleep_value*badrccount/2;
			     if(sleep_value<2000) sleep_value=2000;
				else if(sleep_value>20000) sleep_value=20000;
                             usleep(sleep_value);
                       /*      if(badrccount>99)
				fprintf(flog,"%i attempts to write failed : %u - %s %i\n",
				 badrccount,tp,(*tbase).name,rc);	*/
			  }	/* while((rc=send */
			if(rc<0) 
			 { 	/* 043010 */
			   fprintf(flog,"(P) unable to send_to_mem %i  %u %s %i\n",
				I,tp,(*tbase).name,rc);
			   break;
			 }

                        }
                      }

		rettable.len=8;
		if(rc>=0) rettable.code=1; else rettable.code=3;	
		send_to_mem(I,(char *)&rettable,rettable.len);	
		WMP[I].a1=0;
                }

        else if (todo==24)      /* delete record */
        {
	if(k>0 || owner==1)
	{
	   rettable.code=1; rettable.len=8;
	   send(sock,(char *)&rettable,rettable.len,0); close(sock);
        for(j=0;j<(*tbase).nc;j++)
	   {
		Uattr[j+L[1]].addr=(int *)read_attr(&A[0],(int)(*tbase).attrs[j],0);
		Uattr[j+L[1]].ttlc=(*tbase).attrs[j];
	   }
        RL=0;
        for(i=0;i<(*tbase).nr;i++)
            {
                lc_read_ln(tbase,i,TTLC);
                if(L[1]>0) cmpyes=predicate(tbase,L[1],Uattr,TTLC);
		  else	cmpyes=1;
                if(cmpyes==1)
                  {
                        RL++;
                        for(j=0;j<(*tbase).nc;j++)
                          {
                                NN=TTLC[j];
                                if(NN!=0)
                                {
                                  abase=(struct attr *)Uattr[j+L[1]].addr;
                                  free_attr(abase,NN);
                                  lc_write(tbase,(*tbase).nc*i+j,0);
                                }

                          }
                  }
            }
	if(RL>0)
	{
		for(i=0;i<(*tbase).nc;i++)
		  translate_addr(&A[0],Uattr[i+L[1]].ttlc,1);
        jj=0;
        for(i=0;i<(*tbase).nr-1;i++)
          {
                if(empty_line(tbase,i,TTLC)==0)
                 {
                  if(i>=jj) jj=i+1;
                  for(j=jj;j<(*tbase).nr;j++)
                     if(empty_line(tbase,j,TTLC)!=0)
                        {  move_line(tbase,i,j); break; }
                  jj=j+1;
                 }
                if(jj>(*tbase).nr) break;
          }
        (*tbase).nr-=RL;
        translate_addr(&A[1],t_id,1);
	}

        } else 
	 {  rettable.code=3; rettable.len=8;
	    send(sock,(char *)&rettable,rettable.len,0); close(sock);
	 }
	} /* if(todo==24 */

	} else {
	  rettable.code=3;		/*  042310 - might need to adjust if(todo!=22) */
	  rettable.len=8;
	  send(sock,(char *)&rettable,rettable.len,0);
	  close(sock);
	       }
	release_buf((char *)ibuf,I+1);
		
/*	ttp2=mach_absolute_time(); */
	clock_gettime(CLOCK_REALTIME,&ttT);
	msec=(ttp2-ttp1)/100000; /* msec*10 */

	if(xt[fp[I][K].t].ST[4]==0) xt[fp[I][K].t].ST[4]=msec;
	 else xt[fp[I][K].t].ST[4]=(xt[fp[I][K].t].ST[4]+msec)/2;

			fp[I][K].code=0; fp[I][K].size=0; fp[I][K].owner=0;
			fp[I][K].t=-1; fp[I][K].s=0;
		
		  } /* ending loop X */

	     } /* K loop ends */
	    if(ct==0)
	      { S_timeP.tv_sec=tp1+60; S_timeP.tv_nsec=0;
		rc=pthread_cond_timedwait(&S_condP[I],&S_mutxP[I],&S_timeP);
	      }
	  }  /* while(1	*/
}
/* - - - - - - - - - - - - - - - - - - - - - - */
int	buf_decode(char *buf,int NB,struct table *tbase,
		struct uattr *Uattr,int code,int *L,char role)
{
char			*pattrid,ccode;
unsigned short int	attrid;
unsigned int		i,k,pl;
struct	attr		*abase;


	pattrid=(char *)&attrid;
	k=0; pl=0; L[pl]=0;
	while(NB>0)
	  {
		if( (*buf>59 && *buf<63) || (*buf>69 && *buf<73) )
		  { 
		    if(pl%2==0) { L[++pl]=0; code=2; }
		    ccode=*(buf++);
		  }
		 else { if(pl%2!=0) { L[++pl]=0; code=0; } }

		i=(int)(*(buf++)); *pattrid=*(buf++); *(pattrid+1)=*(buf++);
		if(attrid!=(*tbase).attrs[i])	return(-1);
		abase=(struct attr *)read_attr(&A[0],(int)attrid,0);
		if( ((*abase).access&role)!=0)	return(-2);
		Uattr[k].addr=(int *)abase;
		Uattr[k].offset=i;
		Uattr[k].ttlc=attrid;
		Uattr[k].count=0;
		if((code&1)==0)
		   {
			if(code==0) Uattr[k].fc=*(buf++);
				else Uattr[k].fc=ccode;
			Uattr[k].value=buf;
			buf+=(*abase).len;
			NB-=(*abase).len+4;
		   }
		  else NB-=3;
		L[pl]++; k++;
	  }
	if(pl%2==0) L[++pl]=0;
	return((pl+1)/2);
}

/* - - - - - - - - - - - - - - - - - - - - - - */
