/* by Slava Barsuk */

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

void	err(int);
int	find_attr(int, char *);

/* - - - - - - - - - - - - - - - - - - - - */
char    s_types[5][8]=
{
        "tiny",
        "small",
        "regular",
        "large",
        "wrong"
};

char    n_name[50][22];		/* attr names		*/  
char    t_name[22];		/* table name		*/
char	t_name_prev[22];	/* previous table name	*/
char    v_name[50][22];		/* assign list name	*/
char    v_value[50][80];	/* assign list value	*/
char    p_name[50][22];		/* predicate list name	*/
char    p_value[50][80];	/* predicate list value	*/
char    p_code[50];		/* predicate code	*/
char	n_mod[50];		/* attr output modifier */
int     Nn,Nv,Np;		/* counts for attr, assign and predicate */
int	flag=1;

/* - - - - - - - - - - - - - - - - - - - - */
/*==	looking for name in string p, copies it into a 	*/

char    *find_name(char *p,char *a)
{
int	i;
	i=0;
        while(i<22&& ( (*p>='a'&&*p<='z')||(*p>='A'&& *p<='Z')||
                (*p=='_')||(*p=='-')||(*p>='0'&&*p<='9')) )
                  { *(a++)=*(p++); i++; }
        *a=0;
        return(p);
}

/* - - - - - - - - - - - - - - - - - - - - */
/*== 	building attr list in n_name 		*/
/* attr output modifier capture - name[:M] */

int     list(char *a)
{
char    s;
int     i;

        i=0;
        do {
                a=find_name(a,n_name[i]);
		if(*a==':') { n_mod[i]=*(++a); a++; }
		i++;
                s=*(a++);
           } while(s==',');
        if(s=='*'&&i==1&&*a==0) return(0);
                else if(s==0&&n_name[i-1][0]!=0) return(i);
                else return(-1);

}

/* - - - - - - - - - - - - - - - - - - - - */
/*==	getting table name	*/

int     s_name(char *a)
{
        if(*find_name(a,t_name)!=0||t_name[0]==0) return(-1);
        else return(0);
}

/* - - - - - - - - - - - - - - - - - - - - */
/*==	recognizing keyword		*/

int     keyword(char *a)
{

static  char    keys[7][8]={
"select","delete","update","insert","from","where","into"};

static  int     i;
char    buf[30];

        if(*find_name(a,buf)!=0) return(-1);
        for(i=0;i<7;i++)
        if(strncmp(keys[i],buf,7)==0) break;
        if(i<7) return(i); else return(-1);
}

/* - - - - - - - - - - - - - - - - - - - - */
/*==	analyzing assign list		*/	

int     s_assign(char *a)
{
static  int     i,j;
static  char    s,*p;

        i=0;
        do {
        a=find_name(a,v_name[i]);
        s=*(a++);
        if(s!='=') return(-1);
        p=v_value[i++]; j=0;
        while( j<80 && *a!=0 && *a!=',') { *(p++)=*(a++); j++; }
        *p=0;
        } while( *(a++)==',');
        return(i);

}

/* - - - - - - - - - - - - - - - - - - - - */
/*	analyzing predicate list	*/

int     s_compare(char *a)
{
static  int     i,j;
static  char    s,*p;

        i=0;
        do {
        a=find_name(a,p_name[i]);
        s=*(a++);
        if(s!='='&&s!='>'&&s!='<') return(-1);
        p_code[i]=s;
        p=p_value[i]; j=0;
        while( j<80 && *a!=0 && *a!='&' && *a!='|') { *(p++)=*(a++); j++; }
        *p=0;
        s=*(a++);
	if(s=='|') p_code[i]+=10;	/* 050710 */
	i++;
        } while( s=='&' || s=='|');
        return(i);
}

/* - - - - - - - - - - - - - - - - - - - - - */
/*==	analyzing entered table name, etc	*/

int	check_tables()
{
int	i;

		for(i=0;i<Nn;i++)
		   if(find_attr(0,n_name[i])<0) return(-1);
		for(i=0;i<Nv;i++)
		   if(find_attr(0,v_name[i])<0) return(-1);
		for(i=0;i<Np;i++)
		   if(find_attr(0,p_name[i])<0) return(-1);
	return(0);
}

/********************************************************************/
int	pattr[50];	

int	TQ;

struct
{	char		name[20];
	unsigned char	attrs,id,pool,type,access;
	unsigned int	ST[5];	
	short int	in;
} 	tables[100];

struct
{	char			name[20];
	unsigned short int	size;
	unsigned short int	id;
	unsigned char		type;
	unsigned char		acc;
}	attrs[1200],*ts;

char	text[16384];
int	stat_array[20];

/* - - - - - - - - - - - - - - - - - - - - - - */

void	appendstring(char *a, char *b, int L)
{
static	int	i;

	while( *a!=0) a++;
	for(i=0;i<L;i++)
		if(*b!=0) *(a++)=*(b++);
		   else *(a++)=' ';
	*a=0;
} 
/* - - - - - - - - - - - - - - - - - - - - - - */
void	hex_in(char *p,char *d,int L)
{
char	a;
	while((L--)>0)
	  { a=*(p++); 
	    if(a>='0'&& a<='9') a-='0';
	    else if(a>='A' && a<='F') a-='A'-10;
	    else a=0;
	    *d=a<<4;
	    a=*(p++);
	    if(a>='0'&& a<='9') a-='0';
	    else if(a>='A' && a<='F') a-='A'-10;
	    else a=0;
	    *(d++)=*d+a;
	  }
}
/* - - - - - - - - - - - - - - - - - - - - - - */
void	hex_out(char *p,char *d,int L)
{
char	a;
	while((L--)>0)
	  {
		a=(*p)>>4; if(a>9) *(d++)=a-10+'A'; else *(d++)=a+'0';
		a=(*p++)&15; if(a>9) *(d++)=a-10+'A'; else *(d++)=a+'0';
	  }
	*d=0;
}
/* - - - - - - - - - - - - - - - - - - - - - - */

int	find_table(char *name)
{
int	i;

	for(i=0;i<TQ;i++)
	if(strncmp(tables[i].name,name,20)==0) return(i);
	return(-1);
}
/*******************************************************************/

int	find_attr(int tbl,char *name)
{
int	i,j;

	if((j=tables[tbl].in)<0) return(-2);
	for(i=0;i<tables[tbl].attrs;i++)
		if(strncmp(attrs[j+i].name,name,20)==0)
			return(i);
	return(-1); 
}
/*******************************************************************/

int	get_attr(int I,int tbl,char *data)
{
int	i,j,l,N,type,size;
long long int	LR;
char	*p;
time_t	tp;
double	D;
	
	i=find_attr(tbl,v_name[I]);
	j=tables[tbl].in;
	bzero(data,(int)attrs[j+i].size);
	p=v_value[I]; type=attrs[j+i].type; size=attrs[j+i].size;
	if(type==3)
	 { if(*p=='@') p++;
	   if(strcmp(p,"current")==0) N=time(&tp); else N=atoi(p);
	   bcopy((char *)&N,data,4); 
	 }
	else if(type==4) bcopy(p,data,size);
	else if(type==5) hex_in(p,data,size);
	else if(type==6) 
	 {
	   LR=atoll(p); bcopy(&LR,data,8);
	 }
	else if(type==7)
	 {
	    l=size/4;
	    while(l-->0)
	     {
		N=atoi(p); bcopy(&N,data,4); data+=4; 
		if(*p=='-') p++; while(*p>='0' && *p<='9') p++;
		if(*p==':') p++;
	     }
	 }
	else if(type==8)
	 {
	    D=strtod(p,NULL); bcopy(&D,data,8);
	 }
	else if(type==9)
	 {
	    l=size/8;
	    while(l-->0)
	     {
		D=strtod(p,&p); bcopy(&D,data,8); data+=8;
		if(p==0) break;
		if(*p==':') p++;
	     }
	 }
	return(i);

}

/* - - - - - - - - - - - - - - - - - - - - - */
/*=	getting predicate attributes         =*/

int	get_pred(int I,int tbl,char *data)
{
int	i,j,N,type;
long long int	LR;
time_t	tp;
double	D;

	i=find_attr(tbl,p_name[I]);
	j=tables[tbl].in;
	bzero(data,(int)attrs[j+i].size);
	type=attrs[j+i].type;
	if(type==3)
	 {
	   if(strcmp(p_value[I],"current")==0) N=time(&tp); else N=atoi(p_value[I]);
	   bcopy((char *)&N,data,4); 
	 }
	else if(type==4) bcopy(p_value[I],data,(int)attrs[j+i].size);
	else if(type==5) hex_in(p_value[I],data,(int)attrs[j+i].size);
	else if(type==6)
	 {
	   LR=atoll(p_value[I]); bcopy(&LR,data,8);
	 }
	else if(type==8)
	 {
	   D=strtod(p_value[I],NULL); bcopy(&D,data,8);
	 }

	return(i);

}
/* - - - - - - - - - - - - - - - - - - - - - */
int	get_tables(int sock)
{
static	int	buf[7]={8,0xff030003,0,0,0,0,0}; 
int	len,rc,i,j;
	
	if(strcmp(t_name,"TABLES")==0)
	  len=8 ; 
	 else { len=28; bcopy(t_name,&buf[2],20); }
	buf[0]=len;
	send(sock,buf,len,0);
	ioctl(sock,FIONBIO,&flag);
	i=0; j=0; len=0;
	do
	{
	if( len>0 )
	    {
		bcopy(text,tables[i].name,20); tables[i].id=(unsigned char)text[20];
	/*	tables[i].pool=(unsigned char)text[21];
		tables[i].type=(unsigned char)text[22];
		if(tables[i].type<0 || tables[i].type>4) tables[i].type=4;
		tables[i].access=(unsigned char)text[23]; */
		ts=(void *)&text[21]; tables[i].in=j;
		len-=21;
		while(len>0)
		{
		bcopy((*ts).name,attrs[j].name,20);
		attrs[j].type=(*ts).type; attrs[j].size=(*ts).size;
		attrs[j].acc=(*ts).acc; attrs[j].id=(*ts).id;
		ts++; len-=26; j++;
		}
		tables[i].attrs=j-tables[i].in; i++;
	    }
	  bzero(text,sizeof(text));
	  rc=get_rec(sock,text,&len);

	} while(rc==2);
	TQ=i;
	if(rc>0 && len>0)
	    { bcopy(text,stat_array,len); i=1000; }
	return(i);

}

/************************************************************************/

int	attr_decode(int I,int *type,int *size)
{

	if(v_value[I][0]=='i'&&v_value[I][1]==0)
	 	{ *type=3; *size=4; return(1); }
	else
	if(v_value[I][0]=='i'&&v_value[I][1]==':')
		{ *size=atoi(&v_value[I][2]);
		  if( *size>0 && *size<100) { *type=7; *size*=4; return(1); }
			else return(0);
		}
	if(v_value[I][0]=='s'&&v_value[I][1]==':')
		{ *size=atoi(&v_value[I][2]);
		  if( *size>0 ) { *type=4;  return(1); }
			else return(0);
		}
	else
	if(v_value[I][0]=='x'&&v_value[I][1]==':')
		{ *size=atoi(&v_value[I][2]);
		  if( *size>0 ) { *type=5;  return(1); }
			else return(0);
		}
	else
	if(v_value[I][0]=='l'&&v_value[I][1]==0)
		{ *type=6; *size=8; return(1); }
	 
	else
	if(v_value[I][0]=='f')
	  { 
	    if(v_value[I][1]==0) { *type=8; *size=8; return(1); }
	     else if(v_value[I][1]==':')
	      {
		 *size=atoi(&v_value[I][2]); 
		if( *size>0 && *size<100) { *type=9; *size*=8; return(1); }
		  else return(0);
	      }
	     else return(0);
	  }
	else return(0);

}

/* - - - - - - - - - - - - - - - - - - - - - - - - */

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

void	err(int code)
{
char msg[13][30]=
 { 
   "missing/wrong db_name",
   "invalid qty of arguments",
   "invalid keyword",
   "invalid name format",
   "invalid table/attribute name",
   "name already in use",
   "unable to talk to ifxdb",
   "authorization failure",
   "broken communication",
   "something else",
   "index already exists",
   "index doesn't exist",
   "access denied" };

	code--; if(code<0 || code>13) code=9;
	fprintf(stderr," ifxad -- %s\n",msg[code]);
	exit(4);
}

/**********************************************************************/

int	main(int argc,char *argv[])
{
char	hostname[50],portname[30];
struct	sockaddr_in server;
struct	hostent *hp, *gethostbyname();
struct	servent	*port;

char	ITYPES[8][6]=
{ "Int","Char","Hex","Long","Int","Float","Float","Wrong"};

char	s_text[14][20]=
{
	"Qty of Tables",
	"N/A",
	"Memory Pages in use",
	"Qty of Requests",
	"file reads",
	"file writes",
	"Cached reads",
	"buf reads",
	"Memory Pages",
	"Hash",
	"? Pages",
	"Attr Pages",
	"Table Pages",
	"?? Pages"
};
char	mode_key[4][8]=
{	"enable",
	"disable",
	"sync",
	"async"
};
	
int	sock;
int	NN=8,uid;
int	NB,type,size;

char	name[50];
struct
{
char	name[20];
int	type;
int	size;
} xx[50],*ts;
 
struct
{
int	len;
int	code;
char	text[16384];
} buf;
unsigned char	*pp,*pR;
int	i,j,jj,l,code,N1,len,rc,TP,TA,PA,IT,IA,LOOP,awc;
int	R,lines;
time_t	tR;
long long int LR;
double	D;
char	WB[1024];
char	OB[4096];
char	aword[6][2048];
char	WWB[20];

int     jc,jr;
char    *p,TPERMS[10],APERMS[10];
int     cc[4][6]={ {20,14,30,15,50,0},
                   {14,30,15,50,0,0},
                   {40,14,30,15,50,0},
                   {40,16,30,15,50,0}
                 };

struct	tm *t_out;
FILE	*f_conf;
char	f_conf_name[60]="/etc/db_config/db_name_";

	if(argc<3)	err(2);
	bzero(hostname,sizeof(hostname)); bzero(portname,sizeof(portname));
	strncat(f_conf_name,argv[1],20);
	if((f_conf=fopen(f_conf_name,"r"))==NULL) err(1);
	fgets(WB,100,f_conf);
	p=strtok(WB," \t\n"); strncat(portname,p,20);
	p=strtok(0," \t\n"); strncat(hostname,p,50);
/* 	strcpy(hostname,"localhost"); strcpy(portname,"cwsdata"); strncat(portname,argv[1],10); */
	if(portname[0]==0 || hostname[0]==0) err(1);

	port=getservbyname(portname,0);
	if(port==0)                             err(7);
	server.sin_family=AF_INET;
	hp=gethostbyname(hostname); if(hp==0)   err(7);
	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	server.sin_port=port->s_port;

	bzero(aword,sizeof(aword)); t_name_prev[0]=0;
	if(argc==3 && strcmp(argv[2],"stdin")==0) LOOP=1;
	  else	
	   {
	     awc=argc-2; if(awc>6) awc=6;
		 for(i=0;i<awc;i++) strcpy(aword[i],argv[i+2]);
	     awc=argc-2; LOOP=0;
	   }
	do
	{
	if(LOOP==1)
	  {
		if(fgets(text,16380,stdin)==0 ) break;
		bzero(aword,sizeof(aword));
		p=strtok(text,"\t\n \"\'"); awc=0;
		while(p!=0 && awc<6)
	   	   {
			strcpy(aword[awc++],p);
			p=strtok(0,"\t\n \"\'");
	  	   }
	   }
	bzero(n_mod,sizeof(n_mod));

        j=keyword(aword[0]);
        if(j<0||j>4) 	err(3); 

        if(awc==4 && j!=1) cc[j][3]=0;
	if(awc==3 && j==1) cc[j][2]=0;


        for(i=0;i<6;i++)
        {
                if(cc[j][i]==0) break;
                jc=cc[j][i]/10;
                jr=cc[j][i]%10;
                if(jc==1)
                        { rc=keyword(aword[i+1]);
                          if(rc!=jr)  err(3);
                         }
                else if(jc==2)
                        { rc=list(aword[i+1]);
                          if(rc<0)	err(4);
                          Nn=rc; }

                else if(jc==3)
                         { rc=s_name(aword[i+1]);
                           if(rc<0)	err(4);
                         }
                else if(jc==4)
                        { rc=s_assign(aword[i+1]);
                          if(rc<0)	err(4);
                        Nv=rc;
                        }
                else if(jc==5)
                        { rc=s_compare(aword[i+1]);
                          if(rc<0)	err(4);
                          Np=rc;
                         }

        }
	if(strncmp(t_name,t_name_prev,22)!=0)
	  {
		sock=socket(AF_INET,SOCK_STREAM,0);
		if(connect(sock, (struct sockaddr *)&server,sizeof(server))<0)	 err(7);
		IT=get_tables(sock);
		close(sock);
	  }
	strcpy(t_name_prev,t_name);
	if(IT<1000 && ( (TQ==0) || check_tables()<0))			err(5);
	code=0;
	if(j>0&&uid!=0)			err(8);
	if(j==0&&IT==1000) code=3; else 	/* getting TABLES  */
	if(j==0&&IT<1000) code=22; else		/* getting records */
	if(j==3&&IT<1000) code=21; else		/* insert record   */
	if(j==2&&IT<1000) code=23; else		/* update record   */
	if(j==1&&IT<1000) code=24; else		/* delete record   */
	if(code==0)			err(20);

	if(code==3)
	{
	if(strcmp(n_name[0],"stat")==0)
		{
		  printf("Statistics\n_________________________\n");
		  for(i=0;i<14;i++) printf("%-20s %15i\n",s_text[i],stat_array[i]);
		}
 	  else if(Np==0)
	  {
	    for(i=0;i<TQ;i++)
	      {
		jj=tables[i].in;
		pp=(unsigned char *)TPERMS;  rc=tables[i].access;
		if((rc&1)!=0) *(pp++)='R';
		if((rc&2)!=0) *(pp++)='W';
		if((rc&4)!=0) *(pp++)='U';
		if((rc&8)!=0) *(pp++)='D';
		if((rc&16)!=0) *(pp++)='r';
		if((rc&32)!=0) *(pp++)='w';
		if((rc&64)!=0) *(pp++)='u';
		if((rc&128)!=0) *(pp++)='d';
		*(pp++)=0;
	    for(j=0;j<tables[i].attrs;j++)
	      { 
			pp=(unsigned char *)APERMS;
			rc=attrs[jj+j].acc;
			if((rc&1)!=0) *(pp++)='R';
			if((rc&2)!=0) *(pp++)='U';
			if((rc&4)!=0) *(pp++)='r';
			if((rc&8)!=0) *(pp++)='u'; *(pp++)=0;
		 
		l=attrs[jj+j].type-3; if(l<0 ||l>4) l=5;
		printf("%-20s %3i %-7s %-8s | %-20s %-5s:%4i %-4s\n",
		  tables[i].name,tables[i].pool,s_types[tables[i].type],TPERMS,
		  attrs[jj+j].name,ITYPES[l],attrs[jj+j].size,APERMS);
	      }
	    }
          }
		close(sock);
		if(Np==0) exit(0);
	}
	sock=socket(AF_INET,SOCK_STREAM,0);
	if(connect(sock, (struct sockaddr *)&server,sizeof(server))<0) err(7);

	if(code==1)
	{ 
	    if(strcmp(v_name[0],"pool")==0)	/* creating pool */
	     {
		if(Nv!=3 || strcmp(v_name[1],"hi")!=0 ||
			strcmp(v_name[2],"mpages")!=0 )	err(3);
		buf.code=5; buf.len=14;
		i=atoi(v_value[0]);
		if(i<1 || i>10)				err(6);
		buf.text[0]=(char)i;
		i=atoi(v_value[1]);
		if(i<1 || i>256)			err(6);
		buf.text[1]=(char)i;
		i=atoi(v_value[2]);
		if(i<1 || i>512)			err(6);
		bcopy(&i,&buf.text[2],4);
	     }
	    else if(strcmp(v_name[0],"access_list")==0)
	     {
		buf.code=15; buf.len=12;
		i=atoi(v_value[0]);
		bcopy(&i,buf.text,4);
	     }
	    else if(strcmp(v_name[0],"table")==0)
	     {
		buf.code=code; buf.len=30;
		if(find_table(v_value[0])>=0)		err(6);
		strcpy(buf.text,v_value[0]);
		buf.text[20]=1; buf.text[21]=1; i=0;
		if(Np>0)
		{
		if(strcmp(p_name[i],"pool")==0)
		  {
			j=atoi(p_value[i]);
			if(j<0 || j >10)		err(6);
			buf.text[20]=(char)j;
			i++;
		  }
		if( ((i+1)==Np)&&strcmp(p_name[i],"type")==0)
		  {
			for(j=0;j<4;j++) if(strcmp(p_value[i],s_types[j])==0) break;
			if(j<4) buf.text[21]=j; else 	err(6);
		  }
		  else	if(Np!=1)			err(3);
		}

	for(i=1;i<Nv;i++)
	  {
		rc=attr_decode(i,&type,&size);
		if(rc==0) err(3);
		  else	{
			  strcpy(xx[i-1].name,v_name[i]);
			  xx[i-1].type=type; xx[i-1].size=size; buf.len+=28;
    		 	}
   	  }
	bcopy(xx,&buf.text[22],buf.len-30);
     }
   else	err(3);
	}

	else if(code==2)
	{
	   if(strcmp(p_name[0],"table")==0)
	     {
		buf.len=10;
		IT=find_table(p_value[0]);
		if(IT<0)				err(3);
		buf.text[0]=(unsigned char)tables[IT].id;
		buf.text[1]=(unsigned char)tables[IT].attrs;
		buf.code=code;
	     }
	   else if(strcmp(p_name[0],"access_list")==0)
	     {
		buf.len=12;
		i=atoi(p_value[0]); bcopy(&i,buf.text,4);
		buf.code=16;
	     }
	   else	err(3);
	    
	}
	else if(code==3)
	 {
		buf.len=10;
		if(strcmp(p_name[0],"table")!=0)	err(3);
		if((IT=find_table(p_value[0]))<0)	err(3);
		buf.text[0]=(unsigned char)tables[IT].id;
		buf.text[1]=(unsigned char)tables[IT].attrs;
		if(Nn==1&&strcmp(n_name[0],"timestamp")==0)  code=30;
		else 
		if(Nn==1&&strcmp(n_name[0],"rows")==0) code=31;
		else 					err(3);
		buf.code=code;
	 }
	
	else if(code==4)
	{
	   buf.code=code;
	   if(Np==0 && Nv==1 && strcmp(v_name[0],"mode")==0)
	    {
		for(i=0;i<4;i++) if(strcmp(v_value[0],mode_key[i])==0) break;
		if(i==4)		err(3);
		buf.code=6; buf.text[0]=i; buf.text[1]=i; buf.len=10;
	    }
	   else if(strcmp(p_name[0],"pool")==0)
	    {			/* changing pool parameters */
		if(Nv!=2 || strcmp(v_name[0],"hi")!=0 ||
			strcmp(v_name[1],"mpages")!=0 ) err(3);
		buf.code=10; buf.len=14;
		i=atoi(p_value[0]);
		if(i<0 || i>10)				err(3);
		buf.text[0]=(char)i;
		i=atoi(v_value[0]);
		if(i<1 || i>256)			err(3);
		buf.text[1]=(char)i;
		i=atoi(v_value[1]);
		if(i<1 || i>512)			err(3);
		bcopy(&i,&buf.text[2],4);
	    }
	   else 
	   {
	
	   if(strcmp(p_name[0],"table")!=0)	err(3);
	   IT=find_table(p_value[0]);
	   jj=tables[IT].in;
	   if(IT<0)				err(3);
	   if(strcmp(v_name[0],"ACCESS")==0)
	      {
		buf.code=7;
		buf.text[0]=(unsigned char)tables[IT].id;
		buf.text[1]=(unsigned char)tables[IT].attrs;
		if(Np==1)
		 {				/* modifying table access */
		  pp=(unsigned char *)v_value[0]; buf.text[2]=0; buf.text[3]=0;
		  while ( *pp!=0)
		   {
			if(*pp=='R') buf.text[3]=(buf.text[3]|1); 
			 else
			if(*pp=='W') buf.text[3]=(buf.text[3]|2);
			 else
			if(*pp=='U') buf.text[3]=(buf.text[3]|4);
			 else
			if(*pp=='D') buf.text[3]=(buf.text[3]|8);
			 else
			if(*pp=='r') buf.text[3]=(buf.text[3]|16);
			 else
			if(*pp=='w') buf.text[3]=(buf.text[3]|32);
			 else
			if(*pp=='u') buf.text[3]=(buf.text[3]|64);
			 else
			if(*pp=='d') buf.text[3]=(buf.text[3]|128);
			 else break;
			pp++;
		   }
		  if( *pp!=0) err(5);
		  buf.len=12;
		 }
		else if(Np==2&strcmp(p_name[1],"attribute")==0)
		 {
		   if((TA=find_attr(IT,p_value[1]))<0) { fprintf(stderr,"X: %i %i\n",IT,TA);	err(5); }
		   buf.text[2]=(char)TA; bcopy(&attrs[jj+TA].id,&buf.text[3],2);
		   pp=(unsigned char *)v_value[0]; buf.text[5]=0;
		   while( *pp!=0)
		     {
			if(*pp=='R') buf.text[5]=(buf.text[5]|1);
			  else
			if(*pp=='U') buf.text[5]=(buf.text[5]|2);
			  else
			if(*pp=='r') buf.text[5]=(buf.text[5]|4);
			  else
			if(*pp=='u') buf.text[5]=(buf.text[5]|8);
			  else break;
			pp++;
		     }
		   if(*pp!=0) err(5);
		   buf.len=14;
		 }
		else err(3);
	      }
	   else
		{ 
	   	  if(attr_decode(0,&type,&size)==0)	err(4);
	   	  if(find_attr(IT,v_name[0])>=0)	err(6);
		  buf.text[0]=(unsigned char)tables[IT].id;
		  buf.text[1]=(unsigned char)tables[IT].attrs;
	   	  strcpy(xx[0].name,v_name[0]);
	   	  xx[0].type=type; xx[0].size=size; buf.len=38;
	   	  bcopy(xx,&buf.text[2],28);
		}
	  }
	}

	else if(code>=21 && code<=24)
	{
	  buf.code=code; buf.len=10;
	  TP=0;
	  pp=buf.text; *(pp++)=(unsigned char)tables[TP].id;
	  *(pp++)=(unsigned char)tables[TP].attrs;
	  jj=tables[TP].in;

	  if(code==22)
	    {
		PA=0;
		if(Nn==0)
	   	 for(i=0;i<tables[TP].attrs;i++)
	     	  {
		    *(pp++)=(char)i; bcopy(&attrs[jj+i].id,pp,2); pp+=2;
		    buf.len+=3; pattr[PA++]=i;
	          }
		 else
	  	for(i=0;i<Nn;i++)
	          {
		    TA=find_attr(TP,n_name[i]);
		    *(pp++)=(char)TA; bcopy(&attrs[jj+TA].id,pp,2); pp+=2;
		    buf.len+=3; pattr[PA++]=TA; 
	          }  
	    }
	
	  else if(code==23 || code==21)
	    {
          	for(i=0;i<Nv;i++)
            	 {
                   TA=get_attr(i,TP,text);
                   if(TA<0) { close(sock); err(11); }
		   size=attrs[jj+TA].size;
                   *(pp++)=(char)TA; bcopy(&attrs[jj+TA].id,pp,2); pp+=2;
		   if(v_value[i][0]=='@') *(pp++)=1; else *(pp++)=0;
                   bcopy(text,pp,size);
                   pp+=size; buf.len+=size+4;
                 }
	    }

	 	for(i=0;i<Np;i++)
	   	 {
		   TA=get_pred(i,TP,text);
		   if(TA<0) { close(sock); err(11); }
		   size=attrs[jj+TA].size;
		   *(pp++)=p_code[i]; *(pp++)=(char)TA;
		   bcopy(&attrs[jj+TA].id,pp,2); pp+=2;
		   bcopy(text,pp,size);
		   pp+=size; buf.len+=size+4;
	  	 }

	}

	else err(12);

/* beginning of the request */

/* fprintf(stderr,"sending the request, code %i\n",buf.code); */
	buf.code=buf.code|0xff030100;
	send(sock,&buf,buf.len,0);
	ioctl(sock,FIONBIO,&flag);
	lines=0;
	
	do {
		bzero(text,sizeof(text));
		rc=get_rec(sock,text,&len);

		if(code==22 && len>0) 
		 {	
			OB[0]=0; lines++;
			pp=text;
			for(i=0;i<PA;i++)
			{
			   if(pattr[i]==*pp && len>0)
			     {
			   	pp++; 
				TA=pattr[i]; size=attrs[jj+TA].size; type=attrs[jj+TA].type;
				/* len-=size+1; */
				if(type==3)
				  {
				     bcopy(pp,&R,size);
				     if(n_mod[i]=='D')
				      {
					tR=(time_t)R; t_out=localtime(&tR);
					strftime(WB,20,"%D-%T",t_out);
					j=22;
				      }
				     else if(n_mod[i]=='A')
				      {
				 	pR=pp;
					sprintf(WB,"%u.%u.%u.%u ",*(pR++),*(pR++),*(pR++),*(pR++));
					j=18;
				      }
				     else if(n_mod[i]=='U')
				      {
					sprintf(WB,"%12u ",R);
					j=16;
				      }
				     else 
				      {
					sprintf(WB,"%12i ",R);
					j=16;
				      }
				  }
			  	else if(type==5)
				  {
					j=size*2+2;
					hex_out(pp,WB,size);
				  }
				else if(type==4)
				  {
					j=*(pp++);
					bcopy(pp,WB,j); WB[j]=0; pp+=j;
					j=size+2; size=0; 
				  }
				else if(type==6)
				  {
					bcopy(pp,&LR,size);
					sprintf(WB,"%20llu ",LR); j=24;
				  }
				else if(type==7)
				  {
					l=size/4;
					WB[0]='{'; WB[1]=0;
					for(j=0;j<l;j++)
					 {
					   bcopy(pp,&R,4); sprintf(WWB,"%i:",R); pp+=4; 
					   strcat(WB,WWB);
					 }
					strcat(WB,"}");
					size=0; j=strlen(WB)+2;
				  }
				else if(type==8)
				  {
					bcopy(pp,&D,size);
					sprintf(WB,"%20e ",D); j=24;
				  }
				else if(type==9)
				  {
					l=size/8;
					WB[0]='{'; WB[1]=0;
					for(j=0;j<l;j++)
					 {
					   bcopy(pp,&D,8); sprintf(WWB,"%e:",D); pp+=8;
					   strcat(WB,WWB);
					 }
					strcat(WB,"}");
					size=0; j=strlen(WB)+2;
				  }
				pp+=size;
				appendstring(OB,WB,j);
			     }
			     else
			     {
			     appendstring(OB,"NULL",5);
			     }
			
			}
			puts(OB);
		}	
		else if(code==30 || code==31)
			{ bcopy(text,&i,4); printf("%i\n",i); }
	} while(rc==2);
	if(rc!=1) err(13);

	close(sock);
	if(code!=22 || lines>0) rc=0; else rc=1;
	if(LOOP==0) break;
	} while(1); 
	exit(rc);
}
