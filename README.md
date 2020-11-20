# WebServer_C

# 임베디드 시스템 최종 프로젝트

정적 파일들을 서비스하는 Web Server 프로젝트. 

C의 Socket, Thread를 사용하여 다중 클라이언트 접속을 처리하고, 클라이언트가 요청한 페이지를 제공한다.

사용자의 Get 요청에 대한 응답 퍼포먼스를 끌어올리기 위해 요청 정보를 캐싱하는 기법을 사용하였고, Trie 자료구조를 사용해 캐싱 과정을 구현했다.

캐싱은 요청한 자원이 최초 요청이면 Trie에 저장하고, 반복된 요청이면 Trie를 사용하여 자원를 빠르게 탐색한다.

사용 언어 : C

실행 방법

![image](https://user-images.githubusercontent.com/38209962/99801526-878af480-2b79-11eb-8e78-154c2b665f93.png)

실행 결과

![image](https://user-images.githubusercontent.com/38209962/99802079-64147980-2b7a-11eb-9150-7befffd8d5c3.png)

클라이언트가 request한 페이지를 제공한다.

![image](https://user-images.githubusercontent.com/38209962/99801764-e8b2c800-2b79-11eb-91f1-74ef4b76d35f.png)

클라이언트가 request 할때마다 Log(client ip, requested resource, resource size)를 남긴다.

![image](https://user-images.githubusercontent.com/38209962/99801655-bdc87400-2b79-11eb-9a4e-300bb973bc11.png)

테스팅 결과
