#include <stdio.h>
#include <errno.h>
#include <stdlib.h> //exit
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>

volatile int cont1 = 0;
volatile int cont2 = 0;

void hler(int senial) {
  if (senial == SIGUSR1) {cont1++; printf("[PADRE] Petición tratada por hijo 1 (total: %i)\n", cont1);}
  else if (senial == SIGUSR2) {cont2++; printf("[PADRE] Petición tratada por hijo 2 (total: %i)\n", cont2);}
}


void main(int argc, char**argv){
  if (argc < 2) {
    printf("Introduce los parámetros necesarios");
    exit(1);
  }

  struct addrinfo hints, *res;
  struct sockaddr_storage cli;
  char buf[81], host[NI_MAXHOST], serv[NI_MAXSERV], send[256];
  int bytes = 0;
  int status;

  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = 0;
  getaddrinfo(argv[1],argv[2],&hints, &res);
  int sd = socket(res->ai_family, res->ai_socktype,0);
  bind(sd, (struct sockaddr *)res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);

  struct sigaction act;
  act.sa_handler = hler;
  act.sa_flags = SA_RESTART;
  sigaction(SIGUSR1, &act, NULL);
  sigaction(SIGUSR2, &act, NULL);

  int i = 0;
  for (i = 0; i < 2; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      while(1){
        socklen_t clen = sizeof(cli);
        int c = recvfrom(sd, buf, 80, 0,(struct sockaddr *) &cli, &clen);
        getnameinfo((struct sockaddr *) &cli, clen, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST);
        printf("[H:%i,%i] Host:%s\tPuerto:%s\n",getpid(), i+1, host,serv);

        if (i == 0) kill(getppid(), SIGUSR1);
        else kill(getppid(), SIGUSR2);

        if (buf[0] == 'a') {
          sendto(sd, host, strlen(host), 0, (struct sockaddr *) &cli, clen);
        } else if (buf[0] == 'p'){
          sendto(sd, serv, strlen(serv), 0, (struct sockaddr *) &cli, clen);
        } else if (buf[0] == 'q'){
          exit(0);
        } else {
          sendto(sd,"Comando Desconocido\n",strlen("Comando Desconocido\n"),0, (struct sockaddr *) &cli, clen);
        }

      }
    } else if (pid == -1) {
      printf("ERROR: FORK");
    }
  }

  while(wait(&status) > 0) {
    printf("[PADRE] Terminó hijo con código de salida: %i\n", status);
  }


}
