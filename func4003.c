/* by Slava Barsuk */

#include	<stdio.h>
#include	<stdlib.h>
#include	<strings.h>
#include	<string.h>
#include	"dbdefv32.h"



int	mark_free(int *A,int N)
{
int	j,k;

	j=N/32; k=N%32;
	A=A+j;
	*A=*A&(~(1<<k));
	return(N);
}


int	mark_used(int *A,int L)
{
int	i,k;
	
	for(k=0;k<L;k++)
	if( ~(*A)==0 ) A++;
	  else
		for(i=0;i<32;i++)
		  if((*A&(1<<i))==0) { *A=*A|(1<<i); i++; return(i+k*32); }
	return(0);
}

char 	*translate_addr(struct attr *A,int N,int w)
{
int	page,res;
	
	page=(N-1)/(*A).qty;
	res=((N-1)%(*A).qty)*(*A).len;

	if( w>0 && (*A).lc[page]==0 )
		 { (*A).lc[page]=xmark_used((*A).pool); w++; }

	return((char *)map_segment((int)(*A).pool,(*A).lc[page],w)+res);
}

int	copy_attr(struct attr *A,int N,unsigned char *buf)
{
int		i;
unsigned char	*p,*p1;

	p=translate_addr(A,N,0);
	if((*A).type==4)
	  {	
	    p1=buf+1;
	    for(i=0;i<(*A).len;i++)
		{ if( ( *(p1++)=*(p++) )==0) break; }
            *buf=(unsigned char)i;
	    return(i+1);
	  }
	else bcopy(p,buf,(*A).len);
	return((*A).len);
}	

char *	read_attr(struct attr *A,int N,char *buf)
{
char	*p;

	p=translate_addr(A,N,0);
	if(buf!=0) 
		bcopy(p,buf,(*A).len);
	return(p);
}

int	cmpr_attr(struct attr *A,int N,char *buf)
{
char	*p;
int	N1,N2,type,size;
long long int	LR1,LR2;
double	D1,D2;

	p=translate_addr(A,N,0);
	type=(*A).type; size=(*A).len;
	if(type==3)
	   {
		bcopy(p,&N1,size); bcopy(buf,&N2,size);
		if(N1<N2) return(-1);
		else if(N1==N2) return(0);
		else return(1);
	   }
	else if(type==5 || type==7 || type==9)
	   {	
		return(bcmp(p,buf,size));
	   }
	 else if(type==4)
	   {
		return(strncmp(p,buf,size));
	   } 
	 else if(type==6)
	  {
		bcopy(p,&LR1,size); bcopy(buf,&LR2,size);
		if(LR1<LR2) return(-1);
		else if(LR1==LR2) return(0);
		else return(1);
	  }
	 else if(type==8)
	  {
		bcopy(p,&D1,size); bcopy(buf,&D2,size);
		if(D1<D2) return(-1);
		else if(D1==D2) return(0);
		else return(1);
	  }
	else return(0);
}

int	write_attr(struct attr *A,char *buf)
{
char	*p;
int	N,rc;

	N=vmark_used(A);
	if(N<=0) return(0);
	p=translate_addr(A,N,1);
	bcopy(buf,p,(*A).len);
	return(N); 
}

char *	update_attr(struct attr *A,int N,char *buf)
{
char	*p;
int	rc;

	p=translate_addr(A,N,1);
	bcopy(buf,p,(*A).len);
	return(p);
}

int	free_attr(struct attr *A,int N)
{
	return(vmark_free(A,N));
}
