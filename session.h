#ifndef SESSION_H
#define SESSION_H

typedef struct {
  char sessionId[64];
  char auffuehrungName[100];
  int loggedIn;
  int kundenNummer;
  char sitzplatz[4];
  int reservierungsNummer;
  char datumAuffuehrung[11];
  char uhrzeitAuffuehrung[9];

} Session;

#endif