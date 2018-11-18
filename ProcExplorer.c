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
#include<pthread.h>
pthread_mutex_t watcherDetailsListMutex=PTHREAD_MUTEX_INITIALIZER;
void runWatcher(void *details);
struct GList;
struct WatcherDetails
{
char *file;
struct GList *node;
volatile unsigned short int terminate;
};

struct GList
{
void *val;
struct GList *next;
};
struct GList* gListCreate()
{
	struct GList *list=malloc(sizeof(struct GList));
	list->val=NULL;
	list->next=NULL;
	return list;
}
void insertIntoGList(struct GList **list,void *val)
{
struct GList *head=*list,*t;
if(head==NULL)
{
t=gListCreate();
t->val=val;
*list=t;
}
else
{
	t=gListCreate();
	t->val=val;
	t->next=head;
	*list=t;
}
}
void clearGList(struct GList **list)
{
	struct GList *t,*node;
if(*list==NULL)
	return;
for(node=*list;node!=NULL;node=t)
{
	t=node->next;
	free(node->val);
	free(node);
}
*list=NULL;
}

struct PrevHooks
{
char *prevHooks;
struct PrevHooks *next;
};
const int  E_CREATE=0,E_DELETE=1,E_MODIFY=2;

struct ChangeEvents
{
	char *name;
	unsigned short int eventType;
	struct ChangeEvents *next;
};
void addChangeEvent(struct ChangeEvents *,char *name,unsigned short int eventType);
struct ChangeEvents* createChangeEvent(char *name,unsigned short int eventType);
struct ChangeEvents* createChangeEvent(char *name,unsigned short int eventType)
{
	struct ChangeEvents *r=(struct ChangeEvents *)malloc(sizeof(struct ChangeEvents));
	r->next=NULL;
	r->name=name;
	r->eventType=eventType;
	return r;
}
void addChangeEvent(struct ChangeEvents *r,char *name,unsigned short int eventType)
{
	if(r==NULL)
		return;
while(r->next!=NULL)
{
r=r->next;
}
r->next=createChangeEvent(name,eventType);
}
void emptyChangeEvents(struct ChangeEvents **r1)
{
struct ChangeEvents *r=*r1;
	struct ChangeEvents *t;
while(r!=NULL)
{
t=r;
r=r->next;
free(t->name);
free(t);
}
*r1=NULL;
}
typedef struct ChangeEvents ChangeEvents;
struct PrevHooks *prevHooks;
struct ChangeEvents* watchFileForChange(char *file)
{
int fd,wd,l,i=0,changeType;
char rd[4096]={0},*name;
struct ChangeEvents *r=NULL;
fd=inotify_init();
if(fd<0)
{
	return NULL;
}
wd=inotify_add_watch(fd,file,IN_CREATE | IN_MODIFY | IN_DELETE);
l=read(fd,rd,4096);
if(l<0)
return NULL;
while(i<l)
{
	struct inotify_event *e=(struct inotify_event *)&rd[i];
	if(e->len)
	{
		name=(char *)malloc(sizeof(char)*(strlen(e->name)+1));
		strcpy(name,e->name);
	}
	else
	{
		name=(char *)malloc(sizeof(char)*(strlen(file)+1));
		strcpy(name,file);
	}	
		if(e->mask & IN_CREATE)
			changeType=E_CREATE;
		else if(e->mask & IN_DELETE)
			changeType=E_DELETE;
		else
			changeType=E_MODIFY;
		
		if(r==NULL)
			r=createChangeEvent(name,changeType);
		else
			addChangeEvent(r,e->name,changeType);
		i+=sizeof(struct inotify_event)+e->len;
}
inotify_rm_watch(fd,wd);
close(fd);
return r;
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
void showMenu();
struct GList *watcherDetailsList;
void* startWatcherThread(void *file);
void showWatchers();
void showWatcherDetails(struct WatcherDetails * details);

int main(int argc,char *argv[])
{
	char *pid;
	char buf1[1024];
	int k,ch;
	prevHooks=NULL;
	showProcList(NULL);
	ch=4;
	watcherDetailsList=NULL;
do
{
printf("Input:\n1 :Find Process\n 2:Show files used by a process\n3.Show Files being watched\n4.Quit\n");
scanf("%i",&ch);
switch(ch)
{
	case 1:system("clear");
	scanf("%s",buf1);
	showProcList((strcmp(buf1,"/")==0?NULL:buf1));
	break;
	case 2:
	printf("Enter Process Number\n");
	scanf("%i",&k);
	pid=getPIDAt(k);
	printf("%s\n",pid);
	showFDInfo(pid);
	break;
	case 3:
	showWatchers();
	break;
	default:
	return 0;
	break;
}
}
while(ch!=4);

return 0;
}
void showWatchers()
{
	int i,chs,k;
	struct GList *t;
	for(i=0,t=watcherDetailsList;t!=NULL;t=t->next,i++)
	{
		printf("%i %s\n",i,((struct WatcherDetails *)t->val)->file);
	}
	printf("%d to quit\n",i);
	scanf("%d",&chs);
	if(chs<i)
		{
		for(k=0,t=watcherDetailsList;k<chs;k++,t=t->next);
			printf("%s\n",((struct WatcherDetails *)t->val)->file);
		showWatcherDetails((struct WatcherDetails *)t->val);
		}
		struct WatcherDetails *wDetails=(struct WatcherDetails *)t->val;
		printf("1.Remove watchers\n2.Continue\n");
		scanf("%i",&chs);
		switch(chs)
		{
			case 1:wDetails->terminate=1;
			break;
		}
}
void showFDInfo(char *pid)
{
	int ss,i=0,choice=0,k;
		struct GList *fileList=NULL,*temp=NULL;
	char *dir,*ed,*file;
	char elink[1024],*elink1;
	pthread_t thrd;
	struct dirent *dirInfo;
	DIR *p;
	dir=malloc(sizeof(char)*(strlen(pid)+10));
	sprintf(dir,"/proc/%s/fd",pid);
	printf("Files available to be watched\n");
	if(access(dir,F_OK)!=-1)
	{
			p=opendir(dir);
	dirInfo=readdir(p);
		printf("\tFD Info:\n");
		while(dirInfo!=NULL)
		{
			ed=malloc(sizeof(char)*(strlen(dirInfo->d_name)+strlen(dir)+2));
			sprintf(ed,"%s/%s",dir,dirInfo->d_name);
			//printf("%s\n",dirInfo->d_name);
		ss=readlink(ed,elink,1023);
		if(ss>0)
			elink[ss]='\0';
		else
		{
			elink[0]='\0';
		}
		elink1=(char *)malloc(strlen(elink)*sizeof(char));
		strcpy(elink1,elink);
		if(ss>0 && access(elink,F_OK)!=-1)
		{
		insertIntoGList(&fileList,elink1);
		printf("%d. %s\n",i++,elink1);
		}

					dirInfo=readdir(p);
		free(ed);
		}
		if(i==0)
			return;
		choice=0;
		while(choice!=i)
		{
		printf("Enter your choice(s) \n%d to quit:\n",i);
		scanf("%d",&choice);
		if(choice!=i)
			{
				for(k=i-1,temp=fileList;k>choice;k--,temp=temp->next);
					file=(char *)temp->val;
					pthread_create(&thrd,NULL,startWatcherThread,(void *)file);

			}
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

void runWatcher(void *d1)
{
struct WatcherDetails *details=(struct WatcherDetails *)d1;
struct ChangeEvents *changeEvents=NULL;
char *file=details->file;
details->node=NULL;
while(!details->terminate)
{
changeEvents=watchFileForChange(file);
if(changeEvents==NULL)
continue;
insertIntoGList(&(details->node),changeEvents);
}
}

/*void showMenu()
{
printf("procExplorer: Explore the /proc fs\nUsage: procExplorer\t[Options]\n-f FILE : watch FILE for changes\n-n PROCESS_NAME: search PROCESS_NAME to show details\n-p PID: show details of process with pid PID\n-w PID: watch process with pid PID for changes in the files\n\n");
}*/
void* startWatcherThread(void *f)
{
	char *file=(char *)f;
	struct WatcherDetails *details=malloc(sizeof(struct WatcherDetails));			details->terminate=0;
details->file=file;
printf("File %s \n",file);
pthread_mutex_lock(&watcherDetailsListMutex);
insertIntoGList(&watcherDetailsList,details);
pthread_mutex_unlock(&watcherDetailsListMutex);
		runWatcher(details);
}
void showWatcherDetails(struct WatcherDetails *details)
{
struct GList *list=details->node;
for(list=list;list!=NULL;list=list->next)
{
	struct ChangeEvents *changeEvents=(struct ChangeEvents *)list->val;
	printf("%s -------> %s\n",(changeEvents->eventType==E_CREATE?"File Created":changeEvents->eventType==E_MODIFY?"File Modified":"File Deleted"),changeEvents->name);
}
}
