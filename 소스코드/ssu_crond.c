#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

int min[60];
int hour[60];
int day[32];
int month[13];
int week[7];
/*ssu_crond.c*/

FILE * ffp;//ssu_crondtab_file
FILE * lfp;//ssu_crondtab_log
char path[BUFFER_SIZE];
char filebuf[BUFFER_SIZE];

int cti(char c);
int ssu_daemon_init(void);
void ssu_crond(void);
void cal_cycle(char * line);
void run_command(char * passline, char * line);

int main(void)
{
	memset(path, '\0', BUFFER_SIZE);
	getcwd(path,BUFFER_SIZE);
	//printf("%s\n", path);

	setbuf(stdout, NULL);
	pid_t pid;

	pid = getpid();

	char * ffname ="ssu_crontab_file";

	if((ffp = fopen(ffname, "r"))==NULL){
		fprintf(stderr, "open error for %s\n", ffname);
		exit(1);
	}
	setbuf(ffp,NULL);

	//맨 처음 초기의 crontab 파일 내용을 읽어온다
	fseek(ffp, 0 , SEEK_SET);
	fread(filebuf, BUFFER_SIZE, 1, ffp);


	
	//daemon process 생성
	if(ssu_daemon_init() < 0) {
	fprintf(stderr, "ssu_daemon_init failed\n");
	exit(1);
	}
	exit(0);
}

int ssu_daemon_init(void){
	int fd, maxfd;
	pid_t pid;

	if((pid = fork())<0){
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if(pid !=0)
		exit(0);

	pid = getpid();

	setsid();

	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();
	for(fd = 0; fd < maxfd; fd++)
		close(fd);

	umask(0);

	chdir("/");
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	chdir(path);
	while(1){
		
		char * lfname = "ssu_crontab_log";

		if(access(lfname,F_OK)!=0){//로그파일이 존재하지 않으므로 파일 생성
			if((lfp=fopen(lfname,"w+"))==NULL){
				fprintf(stderr, "open error for %s\n", lfname);
				exit(1);
			}
		}
		else{//로그 파일이 이미 존재할 때
			if((lfp = fopen(lfname, "a+"))==NULL){
				fprintf(stderr, "open error for %s\n", lfname);
				exit(1);
			}
		}
		setbuf(lfp,NULL);
		ssu_crond();
		sleep(60);
		fclose(lfp);
	
	}
	return 0;
}

void ssu_crond(void){

	int count=0;
	for (int i=0; i<strlen(filebuf); i++){
		if(filebuf[i]=='\n') count++;
	}
	int l=0;
	int j=0;
	char linebuf[count][BUFFER_SIZE];
	memset(linebuf, '\0', sizeof(linebuf));

	for(int i=0; i<strlen(filebuf); i++){
		if(filebuf[i]=='\n'){
			l++;
			j=0;

		}
		else{
			linebuf[l][j]= filebuf[i];
			j++;
		}
	}

	char tbuf[BUFFER_SIZE];

	//문자열에서 앞에 숫자부분을 빼주는 코드
	for(int i=0; i<count; i++){
		memset(tbuf, '\0', BUFFER_SIZE);
		strcpy(tbuf, linebuf[i]);
		//printf("%s\n", tbuf);
		for(int j=0; j<strlen(linebuf[i]); j++){
			if(linebuf[i][j]==' '){
				memset(linebuf[i], '\0', strlen(linebuf[i]));
				strcpy(linebuf[i], &tbuf[j+1]);
				break;
			}
		}
	}

	for(int i=0; i<count; i++){
		memset(min, 0, sizeof(min));
		memset(hour, 0, sizeof(hour));
		memset(day, 0, sizeof(day));
		memset(month, 0, sizeof(month));
		memset(week, 0, sizeof(week));
		cal_cycle(linebuf[i]);
		
		//printf("%d\n", i);

		
	}
}

//문자를 정수로 바꿔주는 함수
int cti(char c){
	return (int)c-48;
}

void cal_cycle(char * line){

	char token[5][10];
	memset(token, '\0', sizeof(token));
	char * p;

	char linearr[BUFFER_SIZE];
	memset(linearr, '\0', BUFFER_SIZE);

	char passline[BUFFER_SIZE];
	memset(passline, '\0', BUFFER_SIZE);

	strcpy(passline, line);

	strcpy(linearr, line);

	p = strtok(linearr, " ");

	//주기 토큰 자르기
	int w=0;
	strcpy(token[w],p);
	p=strtok(NULL, " ");
	
	while(w<4){
		w++;
		strcpy(token[w], p);
		p = strtok(NULL, " ");
	}
	
	char command[BUFFER_SIZE];
	memset(command, '\0', BUFFER_SIZE);
	while(p!=NULL){
		strcpy(&command[strlen(command)], p);
		command[strlen(command)]=' ';
		//printf("%s\n", command);	
		p = strtok(NULL, " ");
	}
	/*
	for(int i=0; i<5; i++){
		printf("%s\n", token[i]);
	}*/

	int num1=1;
	int num2=0;
	int num3=1;

	for(int i=0; i<5; i++){
		for(int j=0; j<strlen(token[i]); j++){

			//숫자로 시작
			if(isdigit(token[i][j])){
				if(isdigit(token[i][j+1])){//두자리 숫자이면
					num1 = cti(token[i][j])*10 + cti(token[i][j+1]);
					j++;
				}
				else num1 = cti(token[i][j]);

				if(token[i][j+1]=='-'){//숫자 뒤에 -가 오면
					j+=2;
					if(isdigit(token[i][j])){//-뒤에 숫자이면
						if(isdigit(token[i][j+1])){//두자리 숫자가 오면
							num2 = cti(token[i][j])*10 + cti(token[i][j+1]);
							j++;
						}
						else num2 = cti(token[i][j]);
					}
					if(isdigit(token[i][j+1]=='/')){
						j+=2;
						if(isdigit(token[i][j])){
							if(isdigit(token[i][j+1])){
								num3 = cti(token[i][j])*10 + cti(token[i][j+1]);
								j++;
							}
							else num3 = cti(token[i][j]);
						}

						for(int a=0; num1<=num2; a++){//숫자 - 숫자 / 숫자
							if((a+1)%num3==0){
								if(i==0){
									min[i+num1]=1;
								}
								if(i==1){
									hour[i+num1]=1;
								}
								if(i==2){
									day[i+num1]=1;
								}
								if(i==3){
									month[i+num1]=1;
								}
								if(i==4){
									week[i+num1]=1;
								}
							}
						}
					}
					else{//숫자-숫자 뒤에 /가 안오면 (범위)
						for(int a=num1; a<=num2; a++){
							if(i==0){
								min[a]=1;
							}
							if(i==1){
								hour[a]=1;
							}
							if(i==2){
								day[a]==1;
							}
							if(i==3){
								month[a]==1;
							}
							if(i==4){
								week[a]==1;
							}
						}
					}		
				}
				else{//-가 안오고 숫자만 온다
					if(i==0){//분필드일때
						min[num1]=1;
					}
					if(i==1){//시 필드일때
						hour[num1]=1;
					}
					if(i==2){
						day[num1]=1;
					}
					if(i==3){
						month[num1]=1;
					}
					if(i==4){
						week[num1]=1;
					}
				}	
			}
			else if(token[i][j]=='*'){//맨 처음에 *이 온다
				
				if(token[i][j+1]=='/'){//*뒤 /가 올 때
					j+=2;
					if(isdigit(token[i][j])){
						if(isdigit(token[i][j+1])){//두자리 숫자이면
							num1 = cti(token[i][j])*10 + cti(token[i][j+1]);
							j++;
						}
						else num1 = cti(token[i][j]);
						if(i==0){//분필드일때=
							for(int k=0; k<=sizeof(min)/sizeof(int); k++){
								if((k+1)%num1==0) min[k]=1;

							}
						}
						if(i==1){//시 필드일때
							for(int k=0; k<=sizeof(hour)/sizeof(int); k++){
								if((k+1)%num1==0) hour[k]=1;
							}
						}
						if(i==2){
							for(int k=0; k+1<=sizeof(day)/sizeof(int); k++){
								if((k+1)%num1==0) day[k+1]=1;
							}
						}
						if(i==3){
							for(int k=0; k+1<=sizeof(month)/sizeof(int); k++){
								if((k+1)%num1==0) month[k+1]=1;
							}
						}
						if(i==4){
							for(int k=0; k<=sizeof(week)/sizeof(int); k++){
								if((k+1)%num1==0) week[k]=1;
							}
						}

					}
				}
				else{//*만 올때
					if(i==0){//분필드일때
						for(int k=0; k<=sizeof(min)/sizeof(int); k++){
							min[k]=1;
						}
					}
					if(i==1){//시 필드일때
						for(int k=0; k<=sizeof(hour)/sizeof(int); k++){
							hour[k]=1;
						}
					}
					if(i==2){
						for(int k=0; k+1<=sizeof(day)/sizeof(int); k++){
							day[k]=1;
						}
					}
					if(i==3){
						for(int k=0; k+1<=sizeof(month)/sizeof(int); k++){
							month[k]=1;
						}
					}
					if(i==4){
						for(int k=0; k<=sizeof(week)/sizeof(int); k++){
							week[k]=1;
						}
					}				
				}
			}

		}
	}

	run_command(passline, command);
}

void run_command(char * passline, char * line){

	char command[BUFFER_SIZE];
	memset(command, '\0', BUFFER_SIZE);
	struct tm t;
	time_t curtime = time(NULL);
	localtime_r(&curtime, &t);
	strcpy(command, line);
	//printf("command : %s\n" , command);
	if(min[t.tm_min]&&hour[t.tm_hour]&&day[t.tm_mday]&&month[t.tm_mon+1]&&week[t.tm_wday]){
		//printf("hello\n");
	
		system(command);
		char passbuf[BUFFER_SIZE];

		memset(passbuf, '\0', BUFFER_SIZE);

		strcpy(passbuf, passline);

		char timebuf[BUFFER_SIZE];
		memset(timebuf, '\0', BUFFER_SIZE);
		strcpy(timebuf, ctime(&curtime));
		for(int i=0; i<strlen(timebuf); i++){
			if(timebuf[i]=='\n'){
				timebuf[i]='\0';
				break;
			}
		}

		char logbuf[BUFFER_SIZE];
		memset(logbuf, '\0', BUFFER_SIZE);

		sprintf(logbuf, "[%s] run %s\n", timebuf, passbuf);
		fseek(lfp, 0, SEEK_END);
		fwrite(logbuf, strlen(logbuf), 1, lfp);
	}

	


}
