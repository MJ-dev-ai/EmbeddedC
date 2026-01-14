#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <mysql/mysql.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define ARR_CNT 5

void* send_msg(void* arg);
void* recv_msg(void* arg);
void error_handling(char* msg);
int calc_fee(char* in_date, char* in_time);
char name[NAME_SIZE] = "[Default]";
char msg[BUF_SIZE];

int main(int argc, char* argv[]) {
  int sock;
  struct sockaddr_in serv_addr;
  pthread_t snd_thread, rcv_thread;
  void* thread_return;
  if (argc != 4) {
    printf("Usage : %s <IP> <port> <name>\n", argv[0]);
    exit(1);
  }
  
  sprintf(name, "%s", argv[3]);
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    error_handling("socket() error");
  
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));
  
  if (connect(sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1)
    error_handling("connect() error");
  
  sprintf(msg, "[%s:PASSWD]", name);
  write(sock, msg, strlen(msg));
  
  pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
  pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
  pthread_join(snd_thread, &thread_return);
  pthread_join(rcv_thread, &thread_return);
  
  if(sock != -1)
    close(sock);
  
  return 0;
}

void* send_msg(void* arg) {
  int* sock = (int*)arg;
  int str_len;
  int ret;
  fd_set initset, newset;
  struct timeval tv;
  char name_msg[NAME_SIZE + BUF_SIZE + 2];
  
  FD_ZERO(&initset);
  FD_SET(STDIN_FILENO, &initset);
  
  fputs("Input a message! [ID]msg (Default ID:ALLMSG)\n", stdout);
  
  while (1) {
    memset(msg, 0, sizeof(msg));
    name_msg[0] = '\0';
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    newset = initset;
    ret = select(STDIN_FILENO + 1, &newset, NULL, NULL, &tv);
    
    if (FD_ISSET(STDIN_FILENO, &newset)) {
      fgets(msg, BUF_SIZE, stdin);
      if (!strncmp(msg, "quit\n", 5)) {
        *sock = -1; return NULL;
      }
      else if (msg[0] != '[') {
        strcat(name_msg, "[ALLMSG]");
        strcat(name_msg, msg);
      }
      else
        strcpy(name_msg, msg);
      if (write(*sock, name_msg, strlen(name_msg)) <= 0) {
        *sock = -1;
        return NULL;
      }
    }
    if (ret == 0) {
      if (*sock == -1)
        return NULL;
    }
  }
}

void* recv_msg(void* arg) {
  MYSQL* conn;
  MYSQL_ROW sqlrow;
  int res;
  char sql_cmd[200] = { 0 };
  char* host = "localhost";
  char* user = "iot";
  char* pass = "pwiot";
  char* dbname = "iotdb";
  int* sock = (int*)arg;
  int i;
  char* pToken;
  char* pArray[ARR_CNT] = { 0 };
  char name_msg[NAME_SIZE + BUF_SIZE + 2];
  int str_len;
  int state;
  char time[20];
  char date[20];
  
  conn = mysql_init(NULL);
  puts("MYSQL startup");
  if (!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0))) {
    fprintf(stderr, "ERROR : %s[%d]\n", mysql_error(conn), mysql_errno(conn));
    exit(1);
  }
  else
    printf("Connection Successful!\n\n");
  
  while (1) {
    memset(name_msg, 0x0, sizeof(name_msg));
    str_len = read(*sock, name_msg, NAME_SIZE + BUF_SIZE);
    if (str_len <= 0) {
      *sock = -1;
      return NULL;
    }
    fputs(name_msg, stdout);
    // name_msg[str_len-1] = 0; //'\n' 제거
    name_msg[strcspn(name_msg,"\n")] = '\0';
    
    pToken = strtok(name_msg, "[@]"); i = 0;
    while (pToken != NULL) {
      pArray[i] = pToken;
      if ( ++i >= ARR_CNT)
        break;
      pToken = strtok(NULL, "[@]");
    }
    
    if(!strcmp(pArray[1],"GETDB")) {
      memset(sql_cmd,0,sizeof(sql_cmd));
  
      if(i == 3) {
          // 특정 차량 ID 요청
          char* car_id = pArray[2];
          sprintf(sql_cmd, "SELECT state FROM car WHERE id='%s'", car_id);
          if(mysql_query(conn, sql_cmd)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            continue;
          }
          MYSQL_RES *result = mysql_store_result(conn);
          MYSQL_ROW row = mysql_fetch_row(result);
          int state = row ? atoi(row[0]) : 0;  // 없으면 0
          mysql_free_result(result);
  
          memset(name_msg,0,sizeof(name_msg));
          sprintf(name_msg,"[%s]%s@%s@%d\n", pArray[0], pArray[1], car_id, state);
          write(*sock, name_msg, strlen(name_msg));
      }
      else {
          // 모든 차량 상태 반환
          sprintf(sql_cmd, "SELECT state FROM car");
          if(mysql_query(conn, sql_cmd)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            continue;
          }
          MYSQL_RES *result = mysql_store_result(conn);
          MYSQL_ROW row;
          
          memset(sql_cmd,0,sizeof(sql_cmd));
          
          int status[3];
          int idx = 0;
          while((row = mysql_fetch_row(result))) {
              status[idx++] = atoi(row[0]);
          }
          mysql_free_result(result);
          sprintf(sql_cmd,"[%s]%s@%d@%d@%d\n", pArray[0], pArray[1], status[0], status[1], status[2]);
          write(*sock, sql_cmd, strlen(sql_cmd));
      }
    }
    if (!strcmp(pArray[1], "IN")) {
      sprintf(sql_cmd, "select state from parking where id='%s'",pArray[2]);
      if (mysql_query(conn, sql_cmd)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        break;
      }
      MYSQL_RES *result = mysql_store_result(conn);
      if (result == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        break;
      }
      
      sqlrow = mysql_fetch_row(result);
      if (atoi(sqlrow[0]) == 0) {
        sprintf(sql_cmd, "update parking set state=1, date=now(), time=now() where id='%s'", pArray[2]);
        res = mysql_query(conn, sql_cmd);
        if (res)
          fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
        
        memset(sql_cmd, 0x0, sizeof(sql_cmd));
        sprintf(sql_cmd,"insert into log(id, date, time, state) values('%s', now(), now(), 'IN')", pArray[2]);
        res = mysql_query(conn, sql_cmd);
        
        if (!res)
          printf("inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
        else
          fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
      }
    }
    else if (!strcmp(pArray[1], "OUT")) {
      sprintf(sql_cmd, "select state, time from parking where id='%s'",pArray[2]);
      if (mysql_query(conn, sql_cmd)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        break;
      }
      MYSQL_RES *result = mysql_store_result(conn);
      if (result == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        break;
      }
      
      sqlrow = mysql_fetch_row(result);
      if (atoi(sqlrow[0]) == 1) {
        sprintf(sql_cmd, "update parking set state=0, date=now(), time=now() where id='%s'", pArray[2]);
        res = mysql_query(conn, sql_cmd);
        if (res) {
          fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
          break;
        }
        
        printf("updated %lu rows\n", (unsigned long)mysql_affected_rows(conn));
        memset(sql_cmd, 0x0, sizeof(sql_cmd));
        sprintf(sql_cmd,"insert into log(id, date, time, state) values('%s', now(), now(), 'OUT')", pArray[2]);
        
        res = mysql_query(conn, sql_cmd);
        if (!res)
          printf("inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
        else
          fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
      }
    }
    if (!strcmp(pArray[1], "ENTER")) {
      sprintf(sql_cmd, "select state from car where id='%s'",pArray[2]);
      if (mysql_query(conn, sql_cmd)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        break;
      }
      MYSQL_RES *result = mysql_store_result(conn);
      if (result == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        break;
      }
      
      sqlrow = mysql_fetch_row(result);
      if (atoi(sqlrow[0]) == 0) {
        sprintf(sql_cmd, "update car set state=1, enter_time=now() where id='%s'", pArray[2]);
        res = mysql_query(conn, sql_cmd);
        if (res)
          fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
        
        memset(sql_cmd, 0x0, sizeof(sql_cmd));
        sprintf(sql_cmd,"insert into car_log(id, date, time, state) values('%s', now(), now(), 'ENTER')", pArray[2]);
        
        res = mysql_query(conn, sql_cmd);
        if (!res)
          printf("inserted %lu rows\n", (unsigned long)mysql_affected_rows(conn));
        else
          fprintf(stderr, "ERROR: %s[%d]\n", mysql_error(conn), mysql_errno(conn));
      }
    }
    else if (!strcmp(pArray[1], "LEAVE")) {
      // car 테이블에서 state 확인
      sprintf(sql_cmd, "SELECT state FROM car WHERE id='%s'", pArray[2]);
      if (mysql_query(conn, sql_cmd)) { fprintf(stderr, "%s\n", mysql_error(conn)); break; }
      MYSQL_RES *result = mysql_store_result(conn);
      sqlrow = mysql_fetch_row(result);
      int state = atoi(sqlrow[0]);
      
      if (state == 1) {
          // car_log에서 마지막 ENTER 기록 가져오기
          sprintf(sql_cmd, "SELECT date, time FROM car_log WHERE id='%s' AND state='ENTER' ORDER BY date DESC, time DESC LIMIT 1", pArray[2]);
          if (mysql_query(conn, sql_cmd)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
            break;
          }
          MYSQL_RES *enter_result = mysql_store_result(conn);
          MYSQL_ROW enter_row = mysql_fetch_row(enter_result);
          if (!enter_row) {
            fprintf(stderr, "No ENTER record found for %s\n", pArray[2]);
            continue;
          }
          char* enter_date = enter_row[0];
          char* enter_time = enter_row[1];
          mysql_free_result(enter_result);
  
          int fee = calc_fee(enter_date, enter_time);
  
          // car 상태 업데이트
          sprintf(sql_cmd, "UPDATE car SET state=0, leave_time=NOW() WHERE id='%s'", pArray[2]);
          mysql_query(conn, sql_cmd);
  
          // car_log에 LEAVE 기록
          sprintf(sql_cmd, "INSERT INTO car_log(id, date, time, state) VALUES('%s', NOW(), NOW(), 'LEAVE')", pArray[2]);
          mysql_query(conn, sql_cmd);
  
          // 클라이언트로 요금 전달
          memset(name_msg,0,sizeof(name_msg));
          sprintf(name_msg,"[BT]EXIT@%s@%d\n", pArray[2], fee);
          write(*sock, name_msg, strlen(name_msg));
      }
    }
  }
  mysql_close(conn);
}

int calc_fee(char* in_date, char* in_time) {
    struct tm tm_in = {0};
    time_t t_in, t_now;
    double diff_min;

    // 입력된 날짜와 시간 문자열을 tm 구조체로 변환
    // in_date: "YYYY-MM-DD", in_time: "HH:MM:SS"
    sscanf(in_date, "%4d-%2d-%2d", &tm_in.tm_year, &tm_in.tm_mon, &tm_in.tm_mday);
    sscanf(in_time, "%2d:%2d:%2d", &tm_in.tm_hour, &tm_in.tm_min, &tm_in.tm_sec);
    tm_in.tm_year -= 1900; // tm_year는 1900부터 시작
    tm_in.tm_mon -= 1;     // tm_mon는 0~11
    tm_in.tm_isdst = -1;

    t_in = mktime(&tm_in); // 입차 시간
    time(&t_now);          // 현재 서버 시간

    diff_min = difftime(t_now, t_in) / 60.0; // 총 분

    if (diff_min <= 0) return 0;
    int fee = (int)(diff_min) * 1000;
    if (fee > 40000) fee = 40000;
    return fee;
}

void error_handling(char* msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}