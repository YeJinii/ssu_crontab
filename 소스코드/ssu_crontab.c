#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include "ssu_crontab.h"

#define SECOND_TO_MICRO 1000000
#define PATHLEN 1024

FILE * lfp;

/*ssu_crontab.c*/
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	end_t->tv_sec -= begin_t->tv_sec; // 수행시간이 몇 초인지 계산

	//수행시간의 마이크로초보다 수행 시작 시각의 마이크로초가 크면 수행시간을 1초 줄이고, 1000000마이크로초 더해준다. 
	if(end_t->tv_usec < begin_t->tv_usec){
		end_t->tv_sec--;
		end_t->tv_usec += SECOND_TO_MICRO;
	}

	end_t->tv_usec -= begin_t->tv_usec; //수행시간이 몇 마이크로초인지 계산
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);
}

int main(void){
	
	struct timeval begin_t, end_t;


	FILE *fp;
	char * fname = "ssu_crontab_file";
	char filebuf[BUFFER_SIZE];
	char inputstr[MAXLEN];
	gettimeofday(&begin_t, NULL);
	
	memset(filebuf, '\0', BUFFER_SIZE);
	//ssu_crontab_file이 존재하면
	if(!access(fname, F_OK)){
		//파일 내용을 읽어와 출력
		if((fp = fopen(fname, "r+"))==NULL){//파일을 "r+"모드로 오픈 (O_RDWR)
			fprintf(stderr, "file open error for %s\n", fname);
			exit(1);
		}
		setbuf(fp, NULL);
	
		fread(filebuf, BUFFER_SIZE, 1 , fp);//파일에서 내용을 읽어와 버퍼에 저장하고 
		if(strlen(filebuf)!=0){
			printf("%s\n", filebuf);//화면에 출력한다
		}
	}

	char *lfname = "ssu_crontab_log";//로그 파일 이름

	if(access(lfname,F_OK)!=0){//로그파일이 존재하지 않으므로 파일 생성
			if((lfp=fopen(lfname,"w+"))==NULL){
				fprintf(stderr, "1open error for %s\n", lfname);
				exit(1);
			}

		}
		else{//로그 파일이 이미 존재할 때
			if((lfp = fopen(lfname, "a+"))==NULL){
				fprintf(stderr, "2open error for %s\n", lfname);
				exit(1);
			}

		}
		setbuf(lfp,NULL);


	while(1){
		
		memset(inputstr, '\0', MAXLEN);//입력받은 문자열을 저장할 배열 초기화	
		printf("20172626> ");
		scanf("%[^\n]s", inputstr);
		getchar();

		//엔터만 입력시 프롬프트 재출력
		if(inputstr[0]=='\n'){
			continue;
		}

		if(strncmp(inputstr, "add", 3)==0){
			if(access(fname,F_OK)==-1){//ssu_crontab_file이 존재하지 않으면 생성
				if((fp=fopen(fname, "w+"))==NULL){
					fprintf(stderr, "file open error for %s\n", fname);
					exit(1);
				}
				setbuf(fp,NULL);
			}
			addCommand(inputstr, fp);
			continue;
		}

		if(strncmp(inputstr, "remove", 6)==0){
			removeCommand(inputstr, fp);
			continue;
		}


		if(strncmp(inputstr, "exit", 4)==0){
			fclose(fp);
			gettimeofday(&end_t, NULL);
			ssu_runtime(&begin_t, &end_t);
			exit(0);
		}

		else{
			fprintf(stderr, "잘못된 명령어 입니다.\n");
			continue;
		}
	
	}

}

void addCommand(char *inputstr, FILE * fp){
	
	time_t atime;
	char filebuf[BUFFER_SIZE];
	char addbuf[MAXLEN];
	int lcount=0;//명령어의 개수를 세는 변수
	char checkcycle[5][10]; //주기 저장 배열
	char *ptr;

	//배열 초기화
	memset(filebuf, '\0', BUFFER_SIZE);
	memset(addbuf, '\0', MAXLEN);

	fseek(fp, 0, SEEK_SET); //오프셋을 파일 앞으로 이동시킴
	fread(filebuf, BUFFER_SIZE, 1, fp); //파일의 내용을 읽어옴

	if(strlen(filebuf)==0){
		lcount=0;
	}
	else{
		//lcount+=1;
		for(int i=0; i<strlen(filebuf); i++){ //파일의 개행 수를 계산해  lcount변수에 저장
			if(filebuf[i]=='\n'){
				lcount++;
			}
		}
	}

	char linebuf[BUFFER_SIZE];
	memset(linebuf, '\0', BUFFER_SIZE);

	strcpy(linebuf, inputstr+4);
	//기록 형식에 따라 문자열 제작 (포인터 문자열 배열을 먼저 저장을 해놓음) // 나중에 해놓으면 왜 안되지...?
	sprintf(addbuf, "%d. %s", lcount, inputstr +4);
	

	//printf("%s\n", addbuf);
	//입력받은 주기를 자른다
	char * str =  inputstr +4;
	ptr = strtok(str, " ");

	for(int i=0; i<5; i++){
		strcpy(checkcycle[i], ptr);
		ptr = strtok(NULL, " ");
	}
	
	/*for(int i=0; i<5; i++){
		printf("%s\n", checkcycle[i]);
	}*/

	int n;
	//입력받은 주기가 올바른지 검사
	for(int i=0; i<5; i++){
		for(int j=0; j<strlen(checkcycle[i]); j++){
			if(isdigit(checkcycle[i][j])) {
				if(isdigit(checkcycle[i][j+1])){
					n = ((int)checkcycle[i][j]-48)*10 + (int)checkcycle[i][j+1]-48;
					//printf("%d %d\n",((int)checkcycle[i][j]-48)*10, (int)checkcycle[i][j+1]-48);
					j++;
				}
				else{
					n = (int)checkcycle[i][j]-48;
				}
				
				if(i==0){
					if(n<0||n>59) {
						fprintf(stderr, "분 실행 주기가 잘못 입력되었습니다.\n"); 
						return;
					}
					else continue;
				}
				else if(i==1){
					if(n<0||n>23) {
						fprintf(stderr, "시 실행 주기가 잘못 입력되었습니다.\n"); 
						return;
					}
					else continue;
				}
				else if(i==2){
					if(n<1||n>31) {
						fprintf(stderr, "일 실행 주기가 잘못 입력되었습니다.\n");
						return;
					}
					else continue;
				}
				else if(i==3){
					if(n<1||n>12) {
						fprintf(stderr, "월 실행 주기가 잘못 입력되었습니다.\n");
						return;
					}
					else continue;
				}
				else if(i==4){ //0은 일요일 1-6 순으로 월요일에서 토요일
					if(n<0||n>6) {
						fprintf(stderr, "요일 실행 주기가 잘못 입력되었습니다.\n");
						return;
					}
					else continue;
				}
				else continue;
			}
			else if(checkcycle[i][j]=='*') continue;
			else if(checkcycle[i][j]=='-') continue;
			else if(checkcycle[i][j]==',') continue;
			else if(checkcycle[i][j]=='/') continue;
			else{
				fprintf(stderr, "실행 주기가 잘못 입력되었습니다.\n");
				return;
			}

		}
	}

	if((strlen(filebuf)>0)&&(filebuf[strlen(filebuf)-1]!='\n')){
		filebuf[strlen(filebuf)]= '\n';
	}
	//파일에 실행주기와 명령어 기록
	addbuf[strlen(addbuf)]= '\n';
	
	memcpy(&filebuf[strlen(filebuf)], addbuf, strlen(addbuf));
	
	fseek(fp,0, SEEK_SET);

	atime= time(NULL);
	char timebuf[BUFFER_SIZE];
	memset(timebuf, '\0', BUFFER_SIZE);
	strcpy(timebuf, ctime(&atime));
		
	timebuf[strlen(timebuf)-1]=0;

	fwrite(filebuf, BUFFER_SIZE, 1, fp);//file.txt에 명령어 작성

	char logbuf[BUFFER_SIZE];
	memset(logbuf, '\0', BUFFER_SIZE);
	sprintf(logbuf, "[%s] add %s\n", timebuf, linebuf);
	fseek(lfp, 0, SEEK_END);
	fwrite(logbuf, strlen(logbuf), 1, lfp);

	printf("%s\n", filebuf);//화면에 출력한다

}

void removeCommand(char * inputstr, FILE *fp){
	
	time_t rtime;
	int num;
	char filebuf[BUFFER_SIZE];
	char linebuf[LINEMAX][MAXLEN];
	char tmpbuf[BUFFER_SIZE];
	int lcount=0;
	char * ptr;

	if(isdigit(*&inputstr[7])){

		if(isdigit(*&inputstr[8])){
			num = (char)((*&inputstr[7])*10)-48 + *&inputstr[8]-48;
		}
		else{
			num = (char)(*&inputstr[7]-48);
		}
		
	}
	else{	
		fprintf(stderr, "잘못된 COMMAND_NUMBER\n");
		return;
	}

	memset(filebuf, '\0', BUFFER_SIZE);
	memset(tmpbuf, '\0', BUFFER_SIZE);

	fseek(fp, 0, SEEK_SET);
	fread(filebuf, BUFFER_SIZE, 1, fp);

	//파일의 개행 개수를 센다
	if(strlen(filebuf)==0){
		lcount=0;
	}
	else{
		for(int i=0; i<strlen(filebuf); i++){ //파일의 개행 수를 계산해  lcount변수에 저장
			if(filebuf[i]=='\n'){
				lcount++;
			}
		}
	}
	lcount-=1;//파일 끝 개행 처리

	//filebuf에 저장된 파일 내용을 개행을 나누어 linebuf에 저장
	int N=0;
	ptr = strtok(filebuf, "\n");
	while(1){
		if(N>lcount) break;
		strcpy(linebuf[N], ptr);
			ptr = strtok(NULL, "\n");
		N++;
	}

	lcount+=1;

	if(num > lcount){//존재하지 않는 번호이면
		fprintf(stderr, "잘못된 COMMAND_NUMBER\n");
		return;
	}

	int l=0;
	int offset=0;

	char deletebuf[BUFFER_SIZE];
	memset(deletebuf, '\0', BUFFER_SIZE);
	int d=0;

	while(l<lcount){
		if(l==num){
			memcpy(deletebuf, linebuf[l], strlen(linebuf[l]));
			l++; 
			continue;
		}
		else if(l>num){
			linebuf[l][0]=(char)l+47;
			if(offset!=0){
				tmpbuf[offset]='\n';
				offset+=1;
			}
			memcpy(&tmpbuf[offset], linebuf[l], strlen(linebuf[l]));
			offset += strlen(linebuf[l]);
			l++;
		}
		else {
			if(offset!=0){
				tmpbuf[offset]='\n';
				offset+=1;
			}
			memcpy(&tmpbuf[offset], linebuf[l], strlen(linebuf[l]));
			offset += strlen(linebuf[l]);
			l++;
		}
	}
	tmpbuf[strlen(tmpbuf)]='\n';

	char logbuf[BUFFER_SIZE];
	memset(logbuf, '\0', BUFFER_SIZE);

	rtime = time(NULL);
	char timebuf[BUFFER_SIZE];
	memset(timebuf, '\0', BUFFER_SIZE);
	strcpy(timebuf, ctime(&rtime));

	timebuf[strlen(timebuf)-1]=0;

	for(int i=0; i<strlen(deletebuf); i++){
		if(deletebuf[i]==' '){
			char tbuf[BUFFER_SIZE];
			memset(tbuf, '\0', BUFFER_SIZE);
			strcpy(tbuf, deletebuf);
			memset(deletebuf, '\0', BUFFER_SIZE);
			strcpy(deletebuf,&tbuf[i+1]);
			break;
		}
	}
	sprintf(logbuf, "[%s] remove %s\n", timebuf, deletebuf);
	fseek(lfp, 0, SEEK_END);
	fwrite(logbuf, strlen(logbuf), 1, lfp);

	if(num==0){
		fseek(fp, 0, SEEK_SET);
		memset(tmpbuf, '\0', BUFFER_SIZE);
		fwrite(tmpbuf, BUFFER_SIZE, 1, fp);
	}
	else{
		fseek(fp, 0, SEEK_SET);
		fwrite(tmpbuf, BUFFER_SIZE, 1, fp);
		printf("%s\n", tmpbuf);//화면에 출력한다
	}
}
