#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FileTree.h"

struct Node * create_Node() { //노드를 생성

	struct Node* n = (struct Node*) malloc(sizeof(struct Node)); //노드 동적할당

	for (int x = 0; x < MAX; x++) { //노드에서 다른노드로 갈수있는 경로(자식)들 NULL로 초기화
		n->childs[x] = NULL;
	}
	n->path_len = 0; //경로 길이 0으로 초기화 
	return n;
}

void getRequstURL(char * dest, char * str) { //실제 클라이언트가 요청한 url을 얻음 --> 트라이에 없는 정보 요청시 필요

	int x;

	for(x = 0 ; str[x] != ' '; x++) {
	}
	str[x] = 0;
	// printf("%s\n",str);
	strcpy(dest, str); //dest에 requsturl 저장
}

void check_indexFile(char * dest, char* str) { //해당 경로에 디렉토리가 있는지 검사

	int x = 0;
	int count = -1; // '/' 갯수

	char buf[MAX]; //임시 버퍼
	int buf_index = 0; // 버퍼인덱스

	char dirList[MAX][MAX]; // '/' 사이에 있는 문자열 저장

	while (str[x] != ' ') { // 공백만나면 url부분 끝

		if (str[x] == '/') { 

			count++;
			if (count > 0) { //처음 / 은 무시, 다음 / 만날때부터 문자열 저장
				buf[buf_index] = 0;
				strcpy(dirList[count - 1], buf); // dirlist에 저장
				buf_index = 0; //buf 초기화
			}
		}
		else {
			buf[buf_index++] = str[x];
		}
		x++;
	}
	char path[MAX] = ""; //디렉토리있는형태로 만들떄 사용되는 변수
	if (count > 0) {
		
		for (int x = 0; x < count; x++) {
			sprintf(path, "%s/%s", path, dirList[x]); //디렉토리 있는 형태로 바꿔줌
		}
	}

	// printf("count : %d\n",count);
	// printf("%s\n", path);
	strcpy(dest, path); //복사
}

void insert_n(struct Node* tree, char* str, char * type,int len) {

	struct Node * temp = tree; //트라이 root부터 시작 
	for (int x = 0; x < len; x++) { //맨 처음 단어부터 탐색 시작
		int index = str[x];
		if (temp->childs[index] == NULL) { //구간 노드가 생성안되었다면
			temp->childs[index] = create_Node(); //구간노드 생성
		}
		temp = temp->childs[index]; //다음 구간노드로 이동
	}
	//맨 마지막 노드는 데이터 노드 --> path, type을 저장함.
	strcpy(temp->path, str); //path 저장
    strcpy(temp->type, type); //type 저장
	temp->path_len = strlen(str); //path 길이 저장
}

int search_n(struct Node* tree, char * outputRequestURL,char * outputType, char * outputPath, char * outputParameter, int * outputParameterIndex,char* inputPath, int len) {

	struct Node* temp = tree; //트라이 root부터 시작
	int x;
    
	for (x = 0; inputPath[x] != ' ' && inputPath[x] != '?'; x++) { // ' ' , '?'을 만나기 전까지가 path, '?'일시에는 parameter에대한 파싱 필요

		int index = inputPath[x]; //문자열안에있는 문자 한개씩 검사.
		temp = temp->childs[index]; 
		if (temp == NULL) { //해당 구간노드가 생성이 안되있으면 --> 트라이에 해당 노드 없는것 -> index.html로 변환해야함
			// printf("not found\n");
			break;
		}
	}
    // inputPath[x] = 0;
    // printf("%s\n",inputPath);
    if(inputPath[x] == '?') { //?를 만난상태로 끝났을때 --> parameter 파싱필요
        // inputPath[x] = ' ';
		x = x + 1; // '?' 뛰어넘음.
        char * parameterPointer = inputPath + x; //parameter부분부터 탐색 시작
        int y;
        for(y = 0; parameterPointer[y] != ' '; y++, x++);

        parameterPointer[y] = 0;
        strcpy(outputParameter, parameterPointer); //parameter문자열 저장
        *outputParameterIndex = strlen(parameterPointer); //parameter 문자열 길이 저장
        // printf("parameter : %s len : %d\n", outputParameter, *outputParameterIndex);
		parameterPointer[y] = ' ';
    }

	if (inputPath[x] == ' ') { //path 부분
        // printf("inputpath : %s\n", inputPath);
		if (temp->path_len != 0) { //해당 노드가 데이터 노드일때 
			// printf("%s %s\n", temp->path, temp->type);
			inputPath[x] = 0;
            strcpy(outputType, temp->type); //type과 path 저장
            strcpy(outputPath, inputPath);
            return 1; // 원하는 데이터 트라이에 존재 --> 1 return
		}
	}
	//요청한 자원이 존재하지 않을경우. --> index.html
	
	char dicts[MAX];
	check_indexFile(dicts, inputPath);
	getRequstURL(outputRequestURL, inputPath); //실제 requesturl 얻음
	sprintf(outputPath, "%s/index.html", dicts); //경로 앞쪽에 디렉토리들 있으면 디렉토리 있는 형태로 리턴
    strcpy(outputType, "text/html"); 
    // strcpy(outputPath, "/index.html");

	return 0; // 원하는 데이터 트라이에 존재 x --> 0 return
}

