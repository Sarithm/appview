#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<strings.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/sysinfo.h>
#include	<dirent.h>
#include	<errno.h>

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
	exit(0);
	
}
