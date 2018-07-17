#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<strings.h>
#include	"dbdefv32.h"

int main(int argc,char *argv[])
{

int	N,i;
struct  initdata	initdata;
struct  table  Tables; 
struct  pool 	Pool;


struct	attr	A[2];
char	T[4096];
char	filename[8]={"data.0"};

FILE	*f;

	if(argc!=2)
	  { fprintf(stderr,"ifxdb_create  Number_of_Pools ( should be between 2 and 10)\n");
			exit(4); }
	N=atoi(argv[1]); if(N<2 ||N>10) N=2;
	fprintf(stderr,"Number of pools : %i\n",N);
	bzero(&Pool,sizeof(struct pool));
	bzero(A,sizeof(A));
	bzero(T,4096);
	initdata.attr_qty=2;
	initdata.attr_offset=sizeof(initdata);
	initdata.t1_offset=sizeof(initdata)+sizeof(A);
	initdata.t2_offset=sizeof(initdata)+sizeof(A)+1000;
	initdata.t1_qty=0;
	initdata.p_qty=N;
	initdata.acc_qty=0;

	strcpy(A[0].name,"ATTRIBUTES");
	A[0].type=1;
	A[0].ald=0;
	A[0].pool=0;
	A[0].len=sizeof(A)/2;
	A[0].qty=4096/A[0].len;
	strcpy(A[1].name,"TABLES");
	A[1].type=2;
	A[1].ald=0;
	A[1].pool=0;
	A[1].len=sizeof(Tables);
	A[1].qty=4096/A[1].len;

	Pool.id=0;
	Pool.ipages=300;
	Pool.ihi=60;
	Pool.alloc.len=1024;
	Pool.alloc.type=10;
	Pool.alloc.qty=4;
	Pool.alloc.ald=0; 
	Pool.alloc.pool=0;

	f=fopen("maindata","wb");
	fwrite(&initdata,sizeof(initdata),1,f);
	fwrite(A,sizeof(A),1,f);
	fwrite(T,1000,1,f);
	for(i=0;i<N;i++)
	  {
		if(i>0)
		  { Pool.ihi=40; Pool.ipages=200;  }
		Pool.id=i;
		Pool.alloc.pool=i;
		fwrite(&Pool,sizeof(Pool),1,f);
	  }

	fclose(f);
	for(i=0;i<N;i++)
	  {
		filename[5]='0'+i;
		f=fopen(filename,"wb");
		fwrite(T,4096,1,f);
		fclose(f);
	  }
	exit(0);
}
