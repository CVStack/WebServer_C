
#define MAX  128
#define path_length 512
#define type_length 10

struct Node { //트라이의 노드 구조체    
	char path[path_length]; //가르키는 데이터
	char type[type_length]; //type을 저장
    int path_len; //path 길이 
	struct Node* childs[MAX]; //알파벳 수만큼
};

struct Node * create_Node(); //노드생성

void getRequstURL(char * dest, char * str);  //실제 클라이언트가 요청한 url을 얻음

void check_indexFile(char * dest, char* str); //디렉토리 있는 형태의 url로 바꿔줌

void insert_n(struct Node* tree, char* str, char * type,int len); //트라이에 노드 삽입

int search_n(struct Node* tree, char * outputRequestURL, char * outputType, char * outputPath, char * outputParameter, int * outputParameterIndex, char* inputPath, int len);
//트라이에서 해당 inputPath 탐색
