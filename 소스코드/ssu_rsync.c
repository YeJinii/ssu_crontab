#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <utime.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

#define PATHLEN 1024
#define BUFFER_SIZE 1024
#define SECOND_TO_MICRO 1000000

static int filter(const struct dirent *dirent){
	if(!(strcmp(dirent->d_name, ".")) || !(strcmp(dirent->d_name, ".."))) return 0;
	else if(dirent->d_name[0]=='.'){return 0;}
	else return 1;
}

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

/*ssu_rsync.c*/
int main(int argc, char * argv[]){

	setbuf(stdout,NULL);
	char srcpath[PATHLEN]; //source
	char tmppath[PATHLEN];
	char dstpath[PATHLEN]; //destination
	char logbuf[BUFFER_SIZE];
	char timebuf[PATHLEN];
	struct stat statbuf;
	struct stat statbuf2;
	struct utimbuf ubuf;
	time_t syntime;//
	FILE * lfp; //log file pointer

	struct dirent **namelist; //src가 디렉토리일때 디렉토리 안의 파일정보를 저장할 구조체

	int check=0;
	struct timeval begin_t, end_t;
	gettimeofday(&begin_t, NULL);

	if(argc>4 || argc<3){//인자 부족시 에러처리
		fprintf(stderr, "usage : %s [option] <src> <dst>\n",argv[0]);
		exit(1);
	}

	//사용할 버퍼를 모두 초기화
	memset(srcpath, '\0', PATHLEN);
	memset(tmppath, '\0', PATHLEN);
	memset(dstpath, '\0', PATHLEN);
	memset(logbuf, '\0', BUFFER_SIZE);
	memset(timebuf, '\0', PATHLEN);

	if(argc==3){//옵션 입력을 하지 않았을 때
		
		strcpy(srcpath, argv[1]);
		strcpy(dstpath, argv[2]);

		lstat(argv[1], &statbuf);

		if(access(argv[1], F_OK)!=0){//동기화할 파일을 찾을 수 없으면
			fprintf(stderr, "usage : %s [option] <src> <dst>\n", argv[0]);
			exit(1);
		}

		if(access(argv[2], F_OK)!=0){//인자로 입력받은 dst디렉토리를 찾을 수 없으면
				fprintf(stderr, "usage : %s [option] <src> <dst>\n", argv[0]);
				exit(1);
		}
		lstat(argv[2], &statbuf2);
			
		if(!S_ISDIR(statbuf2.st_mode)){//인자로 입력받은 dst가 디렉토리가 아닐 때
			fprintf(stderr, "usage : %s [option] <src> <dst>\n", argv[0]);
			exit(1);
		}

		//***동기화할 파일에 접근할수 없으면 usage 출력 후 프로그램 종료

		if(!S_ISDIR(statbuf.st_mode)){//src가 소스파일이면
			getcwd(tmppath,PATHLEN);
			sprintf(dstpath, "%s/%s/%s", tmppath, argv[2], argv[1]);

			lstat(argv[1], &statbuf);
			ubuf.actime = statbuf.st_atime;
			ubuf.modtime = statbuf.st_mtime;

			//동일한 파일이 있을 경우 동기화 안함
			if(access(dstpath, F_OK)==0){
				
				lstat(dstpath, &statbuf2);
				if(statbuf.st_mtime == statbuf2.st_mtime){
					fprintf(stderr, "이미 동기화된 파일이 존재함\n");
					exit(0);
				}

			}
			
			//동일한 파일이 없을 경우(동기화)
			//link(argv[1], dstpath);
			char tmpbuf[BUFFER_SIZE];
			memset(tmpbuf, '\0', BUFFER_SIZE);

			FILE * spf = fopen(argv[1], "r");
			fread(tmpbuf, BUFFER_SIZE, 1, spf);

			FILE * dpf = fopen(dstpath, "w+");
			fseek(dpf, 0, SEEK_SET);
			fwrite(tmpbuf, BUFFER_SIZE, 1, dpf);
			fclose(spf);
			fclose(dpf);
			syntime = time(NULL);//동기화 할때의 시간을 저장해놓음

			if(utime(dstpath, &ubuf)<0){
				fprintf(stderr, "%s: utime error\n", argv[1]);
				exit(1);
			}

			char * lfname = "ssu_rsync_log";

			if(access(lfname, F_OK)==0){//log 파일이 있을 경우
				if((lfp = fopen(lfname, "r+"))==NULL){//파일을 "r+"모드로 오픈 (O_RDWR)
					fprintf(stderr, "file open error for %s\n", lfname);
					exit(1);
				}
				fseek(lfp, 0, SEEK_END);
				//fputs('\n',lfp); //파일에 개행 입력
			}
			else{//log 파일이 없을 경우
				if((lfp = fopen(lfname, "w+"))==NULL){//파일을 "w+"모드로 오픈 (O_RDWR)
					fprintf(stderr, "file open error for %s\n", lfname);
					exit(1);
				}
				fseek(lfp, 0, SEEK_SET);
			}
			strcpy(timebuf, ctime(&syntime));
			for(int i=0; i<strlen(timebuf); i++){
				if(timebuf[i]=='\n'){
					timebuf[i]='\0';
				}
			}
			//localtime_r(&syntime, &t);
			sprintf(logbuf, "[%s] ssu_rsync %s %s\n          %s %ldbytes\n",
					timebuf,
					argv[1], argv[2],
					argv[1], statbuf.st_size);	
			//printf("%s\n", logbuf);
			fwrite(logbuf, strlen(logbuf), 1, lfp);

		}
		else{//****동기화할 src 가 디렉토리인 경우
			
			int count;
			
			if(access(argv[1], F_OK)!=0){//src 디렉토리가 없을 때
				fprintf(stderr, "usage :  %s [option] <src> <dst>\n", argv[0]);
				exit(1);
			}

			char srcdirpath[PATHLEN];
			memset(srcdirpath, '\0', PATHLEN);
			
			count = scandir(argv[1], &namelist, *filter, alphasort);
			for(int i=0; i<count; i++){
				
				memset(logbuf, '\0', BUFFER_SIZE);
				if(argv[1][0]=='/'){//절대경로 입력시
					sprintf(srcdirpath, "%s", argv[1]);
				}
				else{
					getcwd(srcdirpath, PATHLEN);
					sprintf(srcdirpath, "%s/%s", srcdirpath, argv[1]);
				}

				//src 디렉토리 안에있는 파일의 경로를 srcdirpath 배열에 저장
				sprintf(srcdirpath, "%s/%s", srcdirpath, namelist[i]->d_name);

				lstat(srcdirpath, &statbuf);

				if(S_ISDIR(statbuf.st_mode)!=0){
					fprintf(stderr, "usage : %s [option] <src> <dst>\n", argv[0]);
					exit(1);
				}

				memset(dstpath, '\0', PATHLEN);
				memset(tmppath, '\0', PATHLEN);

				getcwd(tmppath,PATHLEN);
				//dst 디렉토리 안에있는 파일의 경로를 dstpath배열에 저장
				sprintf(dstpath, "%s/%s/%s", tmppath, argv[2], namelist[i]->d_name);
				//printf("%s %s\n", tmppath, dstpath);

				lstat(srcdirpath, &statbuf);
								
				//디렉토리 안에 이미 동기화된 파일이 존재하면 동기화를 하지 않음
				if(access(dstpath, F_OK)==0){
					lstat(dstpath, &statbuf2);	
					if(statbuf.st_mtime == statbuf2.st_mtime){
						fprintf(stderr, "이미 동기화된 파일이 존재함\n");
						exit(1);
					}
				}

				//fopen(dstpath)
				char tmpbuf[BUFFER_SIZE];
				memset(tmpbuf, '\0', BUFFER_SIZE);

				FILE * spf = fopen(argv[1], "r");
				fread(tmpbuf, BUFFER_SIZE, 1, spf);

				FILE * dpf = fopen(dstpath, "w+");
				fseek(dpf, 0, SEEK_SET);
				fwrite(tmpbuf, BUFFER_SIZE, 1, dpf);
				fclose(spf);
				fclose(dpf);

				syntime = time(NULL);//동기화 할때의 시간을 저장해놓음
				
				ubuf.actime = statbuf.st_atime;
				ubuf.modtime = statbuf.st_mtime;
				//printf("%s\n", dstpath);
				if(utime(dstpath, &ubuf)<0){
					fprintf(stderr, "utime error\n");
					exit(1);
				}

				char * lfname = "ssu_rsync_log";

				if(access(lfname, F_OK)==0){//log 파일이 있을 경우
					if((lfp = fopen(lfname, "a+"))==NULL){//파일을 "r+"모드로 오픈 (O_RDWR)
						fprintf(stderr, "file open error for %s\n", lfname);
						exit(1);
					}
					fseek(lfp, 0, SEEK_END);
						//fputs('\n',lfp); //파일에 개행 입력
				}
				else{//log 파일이 없을 경우
					if((lfp = fopen(lfname, "w+"))==NULL){//파일을 "w+"모드로 오픈 (O_RDWR)
						fprintf(stderr, "file open error for %s\n", lfname);
						exit(1);
					}
					fseek(lfp, 0, SEEK_SET);
				}
				strcpy(timebuf, ctime(&syntime));
				for(int i=0; i<strlen(timebuf); i++){
					if(timebuf[i]=='\n'){
						timebuf[i]='\0';
					}
				}
				memset(logbuf, '\0', BUFFER_SIZE);
				//localtime_r(&syntime, &t);
				if(check==0){
					sprintf(logbuf, "[%s] ssu_rsync %s %s\n          %s %ldbytes\n",timebuf, argv[1], argv[2], namelist[i]->d_name, statbuf.st_size);
					check=1;
				}
				else{
					sprintf(logbuf,"          %s %ldbytes\n",namelist[i]->d_name, statbuf.st_size);	
				}
				char logpath[PATHLEN];
				memset(logpath, '\0', PATHLEN);
				getcwd(logpath, PATHLEN);
				sprintf(logpath, "%s/%s", logpath, lfname);

				fseek(lfp, 0, SEEK_END);
				long size=ftell(lfp);
				fseek(lfp, size, SEEK_SET);
				fwrite(logbuf, strlen(logbuf), 1, lfp);
				fclose(lfp);
			}
		}
		gettimeofday(&end_t, NULL);
		ssu_runtime(&begin_t, &end_t);
		
	}
			
	if(argc==4){//옵션 입력 시
		//argv[1]은 옵션 인자
		if(argv[1][0]!='-'){
			fprintf(stderr, "잘못된 옵션 입력\n");
			exit(1);
		}
		strcpy(srcpath, argv[2]);
		strcpy(dstpath, argv[3]);
	}
}
