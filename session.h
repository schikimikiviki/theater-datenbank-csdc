#ifndef SESSION_H
#define SESSION_H

#define MAX_SESSIONS 64
#define SESSION_ID_LEN 64

typedef struct {
  char sessionId[SESSION_ID_LEN];
  int inUse;
  char auffuehrungName[100];
  int loggedIn;
  int kundenNummer;
  char sitzplatz[4];
  int reservierungsNummer;
  char datumAuffuehrung[11];
  char uhrzeitAuffuehrung[9];
} Session;

void initSessionStore(void);
Session* createSession(void);
Session* findSession(const char* sessionId);

#endif