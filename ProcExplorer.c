#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<unistd.h>
#include<termios.h>
#include<sys/inotify.h>
struct PrevHooks
{
char *prevHooks;
struct PrevHooks *next;
};
const E_CREATE=0,E_DELETE=1,E_MODIFY=2;
struct ChangeEvents
{
	char *name;
	unsigned short int eventType;
};
typedef ChangeEvents ChangeEvents;
struct PrevHooks *prevHooks;
struct ChangeEvents* watchFileForChange(char *file)
{
int fd;
char rd[4096];
fd=inotify_init();
if(fd)
}
void addHook(char *hk)
{
	struct PrevHooks *t;
	if(prevHooks==NULL)
	{
		prevHooks=malloc(sizeof(struct PrevHooks));
	prevHooks->next=NULL;
	prevHooks->prevHooks=malloc(sizeof(char *)*(strlen(hk)+1));
	strcpy(prevHooks->prevHooks,hk);
	return;
	}
	for(t=prevHooks;t->next!=NULL;t=t->next);
		t->next=malloc(sizeof(struct PrevHooks));
	t=t->next;
	t->next=NULL;
	t->prevHooks=malloc(sizeof(char *)*(strlen(hk)+1));
	strcpy(t->prevHooks,hk);
}
void emptyHooks()
{
	struct PrevHooks *t,*t1,*t2;
	if(prevHooks==NULL)
		return;
	t1=prevHooks->next;
	for(t=prevHooks;t!=NULL;t=t->next)
	{
		t2=t;
		t=t1;
		if(t1!=NULL)
		t1=t1->next;
		free(t2);
	}
	prevHooks=NULL;
}
void showProcList(char *);
int isDirectory(char *);
/*
pid cmdline cwd user start
*/
int getContents(char *file,char *,int);
int getSize(char *);
char* getPIDAt(int);
void showFDInfo(char *pid);
int main()
{
	char *pid;
	char ch,buf1[1024];
	int k=0;
	prevHooks=NULL;
	showProcList(NULL);
	ch='q';
do
{
ch=getchar();
switch(ch)
{
	case '/':system("clear");
	scanf("%s",buf1);
	showProcList(buf1);
	break;
	default:
	scanf("%i",&k);
	pid=getPIDAt(k);
	if(pid)
		showFDInfo(pid);
}
}
while(ch!='q');

return 0;
}
void showFDInfo(char *pid)
{
	int ss;
	char *dir,*ed;
	char elink[1024];
	struct dirent *dirInfo;
	DIR *p;
	dir=malloc(sizeof(char)*(strlen(pid)+10));
	sprintf(dir,"/proc/%s/fd",pid);
	if(access(dir,F_OK)!=-1)
	{
			p=opendir(dir);
	dirInfo=readdir(p);
		printf("\tFD Info:\n");
		while(dirInfo!=NULL)
		{
			ed=malloc(sizeof(char)*(strlen(dirInfo->d_name)+strlen(dir)+2));
			sprintf(ed,"%s/%s",dir,dirInfo->d_name);
		ss=readlink(ed,elink,1023);
		if(ss>0)
			elink[ss]='\0';
		printf("%s\n",elink);
			dirInfo=readdir(p);
		free(ed);
		}
	}
}
char* getPIDAt(int p)
{
	int i;
	struct PrevHooks *t=prevHooks;
	for(i=0;i<p && t!=NULL;i++)
		t=t->next;
		if(t==NULL)
			return NULL;
		else
			return t->prevHooks;
}
void showProcList(char *srch)
{
	char cwdPath[1024],cmdLinePath[1024];
	int ss=0,t,ct=0;
	char *dir,*cmdline,*cwd,*user,*start;
DIR *p=opendir("/proc");
struct dirent *d=readdir(p);
if(prevHooks)
emptyHooks();
printf("%4s %10s %10s %10s %10s %10s\n","SN","PID","cmdline","cwd","user","start");
while(d!=NULL)
{
	if(strcmp(d->d_name,".")==0 || strcmp(d->d_name,"..")==0)
	{
		d=readdir(p);
		continue;
	}
	dir=(char *)malloc(sizeof(char)*(strlen(d->d_name)+8));
	strcpy(dir,"/proc/");
	strcat(dir,d->d_name);
	if(!isDirectory(dir))
	{
		free(dir);
		d=readdir(p);
		continue;
	}
	cmdline=(char *)malloc(sizeof(char)*(strlen(dir)+9));
	strcpy(cmdline,dir);
	strcat(cmdline,"/cmdline");
	t=getContents(cmdline,cmdLinePath,1024);
	if(!t)
	{
		free(cmdline);
		free(dir);
		d=readdir(p);
		continue;
	}
	//	printf("%10s %10s %10s\n",dir,cmdline,cmdLinePath);
	cwd=(char *)malloc(sizeof(char)*(strlen(dir)+5));
	strcpy(cwd,dir);
	strcat(cwd,"/cwd");
	ss=readlink(cwd,cwdPath,1023);
	if(ss>0)
	{
		cwdPath[ss]='\0';
		if((!srch) || (strstr(d->d_name,srch) || strstr(cmdLinePath,srch) || strstr(cwdPath,srch)))
	{
	printf("%4d %10s %10s %10s\n",ct,d->d_name,cmdLinePath,cwdPath);
	addHook(d->d_name);
	ct++;
	}
	}
	free(cmdline);
	free(cwd);
	free(dir);
	d=readdir(p);
}
}
int isDirectory(char *f)
{
	struct stat s;
	stat(f,&s);
	return S_ISDIR(s.st_mode);
}
int getContents(char *file,char *buf,int len)
{
	FILE *f;
	int i=0;
	int sz=getSize(file);
	if(sz<0)
		return 0;
	else
			f=fopen(file,"r");
		i=fread(buf,sizeof(char),len-1,f);
		buf[i]='\0';
}
int getSize(char *file)
{
struct stat s;
if(stat(file,&s)==0)
return s.st_size;
else
return -1;
}
