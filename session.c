#include "session.h"
#include <stdlib.h>
#include <string.h>

// In C ist die Bedeutung von "static" ein bisschen anders als in Java:
// Hier wird es verwendet, um eine globale Variable (außerhalb der Funktionen)
// zu definieren kann aber nicht von extern verwendet werden
// wir schreiben also "static", DAMIT sie von außen nicht sichtbar ist
// => wir brauchen sie nur in dieser Datei

static Session store[MAX_SESSIONS];

void initSessionStore(void) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    store[i].inUse = 0;
  }
}

static void generateSessionId(char *buf, size_t len) {
  const char hex[] = "0123456789abcdef";
  for (size_t i = 0; i < len - 1; i++) {
    buf[i] = hex[rand() % 16];
  }
  buf[len - 1] = '\0'; // das ist der Zero character der am Ende von jedem
                       // String stehen muss
}

Session *createSession(void) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (!store[i].inUse) {
      char id[SESSION_ID_LEN]; // => fixe Länger die im Headerfile definiert ist
      int unique;

      do {

        // hier übergeben wir die id und es passiert ein << array decay >>
        // In c wird das erste Element in einem Array in einen pointer
        // umgewandelt daher können wir einfach id übergeben und es ist dasselbe
        // wie wenn wir direkt den pointer übergeben: &id[0]
        generateSessionId(id, SESSION_ID_LEN);
        unique = 1;
        for (int j = 0; j < MAX_SESSIONS; j++) {
          if (store[j].inUse && strcmp(store[j].sessionId, id) == 0) {
            // wir generieren so lange eine neue session bis eine unique dabei
            // ist strcmp ist dabei eine Comparison zwischen der ID in "unserer
            // Session" also falls wir bereits eingeloggt sind und der neuen ID
            unique = 0;
            break;
          }
        }
      } while (!unique);

      // wir instanziieren die Session mit leeren Werten, nur die Session-ID
      // wird gesetzt
      strcpy(store[i].sessionId, id);
      store[i].inUse = 1;
      store[i].loggedIn = 0;
      store[i].kundenNummer = 0;
      store[i].reservierungsNummer = 0;
      store[i].sitzplatz[0] = '\0';
      store[i].auffuehrungName[0] = '\0';
      store[i].datumAuffuehrung[0] = '\0';
      store[i].uhrzeitAuffuehrung[0] = '\0';

      // wir retournieren den pointer zu unserer Session
      return &store[i];
    }
  }
  return NULL;
}

Session *findSession(const char *sessionId) {
  if (!sessionId)
    return NULL;
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (store[i].inUse && strcmp(store[i].sessionId, sessionId) == 0) {
      return &store[i];
    }
  }
  return NULL;
}
