#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ifaddrs.h>
#include <signal.h>
#include <wait.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>

#define MAX 2048

static char buf[MAX];
static char buffer[MAX];

// Format for each record
struct record
{
  int acctnum;
  char name[20];
  float value;
  char phone[16];
  int age;
} accinfo;

void signal_catcher(int the_sig)
{
  wait(0);
}

void query_database(int acctnum, char *buffer)
{
  int fd = open("db24", O_RDWR);
  if (fd == -1)
  {
    perror("Error! Cannot open file");
    exit(1);
  }

  // Searching for any account number
  while (read(fd, &accinfo, sizeof(struct record)) > 0)
  {
    if (accinfo.acctnum == acctnum)
    {
      // if the record with the desired account number is found, then construct a message to send back to the client
      sprintf(buffer, "%s %d %f\n\n", accinfo.name, accinfo.acctnum, accinfo.value);
      close(fd); // Close the file
      return;    // Exit after finding the record
    }
  }

  // If the account number is not found, error
  sprintf(buffer, "Account Number: %d not found", accinfo.acctnum);

  close(fd); // Close the file
}

void update_database(int acctnum, double value)
{
  int fd = open("db24", O_RDWR);
  if (fd == -1)
  {
    perror("Error! Cannot open file");
    exit(1);
  }

  // Search for the account number and mutex lock if the record is found
  while (read(fd, &accinfo, sizeof(struct record)) > 0)
  {
    if (accinfo.acctnum == acctnum)
    {
      // Found the record with the desired account number
      if (lockf(fd, F_LOCK, sizeof(struct record)) != 0)
      {
        perror("Mutex Lock failed");
        close(fd);
        exit(1);
      }

      accinfo.value += value; // Update the value

      // Write the updated record back to the file
      lseek(fd, -sizeof(struct record), SEEK_CUR); // Move back to update the record
      if (write(fd, &accinfo, sizeof(struct record)) == -1)
      {
        perror("Error! Cannot write to file");
        close(fd);
        exit(1);
      }

      if (lockf(fd, F_ULOCK, sizeof(struct record)) != 0)
      { // Unlock the record
        perror("Mutex Unlocking failed");
        close(fd);
        exit(1);
      }

      break;
    }
  }

  close(fd); // Close the file
}

void get_first_non_loopback_ip(char *buffer, size_t buflen)
{
  struct ifaddrs *ifaddr, *ifa;
  int family, s;

  if (getifaddrs(&ifaddr) == -1)
  {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  /* Walk through linked list, maintaining head pointer so we
    can free list later */
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
  {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;
    /* For an AF_INET* interface address, display the address */
    if (family == AF_INET && !(ifa->ifa_flags & IFF_LOOPBACK))
    {
      s = getnameinfo(ifa->ifa_addr,
                      (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                      buffer, buflen,
                      NULL, 0, NI_NUMERICHOST);
      if (s != 0)
      {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
      }
      break; // If we get here, we've got an IP, so break.
    }
  }

  freeifaddrs(ifaddr);
}

int main()
{
  int orig_sock, new_sock, port, len, i; // len = sizeof(serv_adr)
  socklen_t clnt_len, adr_size;
  struct sockaddr_in clnt_adr, serv_adr;
  char host[256];
  // struct hostent *host_entry;
  // int hostname;
  // struct in_addr **addr_list;

  if (signal(SIGCHLD, signal_catcher) == SIG_ERR)
  {
    perror("Error! SIGCHLD");
    return 1;
  }

  // Socket creation
  if ((orig_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Generate error");
    return 2;
  }

  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = INADDR_ANY;
  serv_adr.sin_port = 0; // To assign the port dynamically

  if (bind(orig_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0)
  {
    close(orig_sock);
    perror("Binding error");
    return 3;
  }

  // Getting IP
  // hostname = gethostname(hostbuffer, sizeof(hostbuffer));
  // host_entry = gethostbyname(hostbuffer);

  get_first_non_loopback_ip(host, 256);

  // if (host_entry == NULL){
  // perror("gethostbyname error");
  // exit(1);
  //}
  adr_size = sizeof(serv_adr);
  if (getsockname(orig_sock, (struct sockaddr *)&serv_adr, &adr_size) == -1)
  {
    perror("No Port Number assigned!");
    exit(1);
  }
  else
  {
    port = ntohs(serv_adr.sin_port);
  }
  // addr_list = (struct in_addr **)host_entry->h_addr_list;
  // char *ip = inet_ntoa(*addr_list[0]);

  // Message that needs to be broadcasted
  char msg[100];
  sprintf(msg, "PUT BANK620 %d", port);

  // UDP broadcast
  struct sockaddr_in local, remote;
  int sk, rlen = sizeof(remote), clen = sizeof(local);

  sk = socket(AF_INET, SOCK_DGRAM, 0);

  local.sin_family = AF_INET;
  local.sin_addr.s_addr = inet_addr("137.148.205.255");
  local.sin_port = ntohs(14585);

  setsockopt(sk, SOL_SOCKET, SO_BROADCAST, (struct sockaddr *)&local, clen);
  sendto(sk, msg, strlen(msg) + 1, 0, (struct sockaddr *)&local, clen);
  recvfrom(sk, buf, 3, 0, (struct sockaddr *)&remote, &rlen);
  printf("Registration %s from %s\n", buf, inet_ntoa(remote.sin_addr));
  close(sk);

  // TCP listen
  if (listen(orig_sock, 5) < 0)
  {
    close(orig_sock);
    perror("Listening error");
    return 4;
  }

  do
  {
    clnt_len = sizeof(clnt_adr);
    if ((new_sock = accept(orig_sock, (struct sockaddr *)&clnt_adr, &clnt_len)) < 0)
    {
      close(orig_sock);
      perror("Accepting error");
      return 5;
    }

    if (fork() == 0)
    {
      while ((len = recv(new_sock, buffer, MAX, 0) > 0))
      {

        // buffer[len] = '\0'; // Null terminate the received message
        printf("Service Requested from: %s \n", inet_ntoa(clnt_adr.sin_addr));
        // strcat(buffer, "\n\n");
        // Parse the received message to extract account number and value
        int acctnum;
        double value;
        // && sscanf(buffer + 4, "%d %lf", &acctnum, &value) == 2     && sscanf(buffer + 4, "%d", &acctnum) == 1
        if (strncmp(buffer, "upd", 3) == 0 && sscanf(buffer + 4, "%d %lf", &acctnum, &value) == 2)
        {
          // Update the database with the received account number and value
          update_database(acctnum, value);
          query_database(acctnum, buffer);
          // Construct the message to send back to the client
        }

        else if (strncmp(buffer, "qry", 3) == 0 && sscanf(buffer + 4, "%d", &acctnum) == 1)
        {
          // Retrieve information from the database based on the account number
          query_database(acctnum, buffer);
        }

        else if (strncmp(buffer, "quit", 4) == 0)
        { // If the client wants to quit
          break;
          exit(0); // Exit the child process
        }

        else
        {
          printf("Invalid command format");
        }

        // Convert the name to uppercase
        for (int i = 0; accinfo.name[i]; i++)
        {
          accinfo.name[i] = toupper(accinfo.name[i]);
        }

        if (send(new_sock, buffer, strlen(buffer), 0) == -1)
        {
          perror("Error! Cannot send message back to client");
          close(new_sock);
          exit(1);
        }
      }
      close(new_sock); // Close: socket and exit: child process
      exit(0);
    }

  } while (1);

  return 0;
}
