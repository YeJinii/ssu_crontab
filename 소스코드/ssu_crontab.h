#define BUFFER_SIZE 1024
#define MAXLEN 500 //명령어 저장 최대 길이 
#define LINEMAX 100 //명령어 최대 갯수
void addCommand(char * inputstr, FILE *fp);
void removeCommand(char * inputstr, FILE *fp);
