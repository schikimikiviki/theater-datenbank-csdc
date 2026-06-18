#ifndef SESSION_H
#define SESSION_H

typedef struct {
  char sessionId[64];
  char auffuehrungName[100];
  int loggedIn;
  int kundenNummer;
  int sitzplatz;
  int reservierungsNummer;

} Session;

#endif