#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h> // time

#include "database.h"
#include "session.h"

#define SIZE 1024 // buffer size
#define ROWS 8
#define COLS 12

/**
 * @brief Returns a string with the current time in HTTP response date format
 * @param buf buffer to store the time string
 * https://stackoverflow.com/questions/7548759/generate-a-date-string-in-http-response-date-format-in-c
 */
void getTimeString(char *buf);

void getFieldData(int clientSocket) {

  char buffer[1024]; // Zwischenspeicher den wir definieren

  snprintf(buffer, sizeof(buffer),
           "<form action=\"/login\" method=\"GET\">"
           "<label for=\"kundenID\">Kundennummer:</label><br>"
           "<input type=\"text\" id=\"kundenID\" name=\"kundenID\"><br><br>"
           "<input type=\"submit\" value=\"Anmelden\">"
           "</form>");

  send(clientSocket, buffer, strlen(buffer), 0);
}

// used to check if a string starts with a sequence
int startsWith(const char *a, const char *b) {
  if (strncmp(a, b, strlen(b)) == 0)
    return 1;
  return 0;
}

void sendCSSHeader(int clientSocket) {
  char timeBuf[100];
  getTimeString(timeBuf);
  char resHeader[SIZE];
  sprintf(resHeader,
          "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: text/css\r\n\n",
          timeBuf);
  send(clientSocket, resHeader, strlen(resHeader), 0);
}

void sendImageHeader(int clientSocket) {
  char timeBuf[100];
  getTimeString(timeBuf);
  char resHeader[SIZE];
  sprintf(resHeader,
          "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: image/jpeg\r\n\n",
          timeBuf);
  send(clientSocket, resHeader, strlen(resHeader), 0);
}

void sendFileToClient(int clientSocket, const char *filepath) {
  FILE *file = fopen(filepath, "r");
  if (!file)
    return;
  fseek(file, 0, SEEK_END);
  long fsize = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buf = malloc(fsize);
  fread(buf, fsize, 1, file);
  send(clientSocket, buf, fsize, 0);
  free(buf);
  fclose(file);
}

void sendHTTPHeader(int clientSocket, Session *session) {
  char timeBuf[100];
  getTimeString(timeBuf);
  char resHeader[SIZE];
  if (session) {
    // wenn wir eingelogged sind -> id der session mitschicken
    sprintf(resHeader,
            "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: "
            "text/html\r\nSet-Cookie: sessionId=%s\r\n\r\n",
            timeBuf, session->sessionId);
  } else {
    // wenn nicht -> normaler Header ohne ID
    sprintf(resHeader,
            "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: text/html\r\n\r\n",
            timeBuf);
  }
  send(clientSocket, resHeader, strlen(resHeader), 0);
}

void sendRedirect(int clientSocket, const char *location,
                  const char *sessionId) {
  char response[512];
  if (sessionId) {
    snprintf(response, sizeof(response),
             "HTTP/1.1 302 Found\r\n"
             "Location: %s\r\n"
             "Set-Cookie: sessionId=%s\r\n"
             "Content-Length: 0\r\n"
             "\r\n",
             location, sessionId);
  } else {
    snprintf(response, sizeof(response),
             "HTTP/1.1 302 Found\r\n"
             "Location: %s\r\n"
             "Content-Length: 0\r\n"
             "\r\n",
             location);
  }
  send(clientSocket, response, strlen(response), 0);
}

// diese Funktion nimmt einen HTTP Request entgegen
// zb sowas wie:
// GET /index.html HTTP/1.1
// Host : localhost Cookie : theme = dark;
// sessionId = ABC123XYZ;
// lang = de

// ===> wir möchten das so zamschneiden dass am ende nur die SessionID übrig
// bleibt

char *extractSessionId(const char *request) {
  // strstr sucht nach einem Teilstring
  const char *cookieHeader = strstr(request, "Cookie: ");
  if (!cookieHeader)
    return NULL;

  const char *sessionIdStr = strstr(cookieHeader, "sessionId=");
  if (!sessionIdStr)
    return NULL;

  sessionIdStr +=
      strlen("sessionId="); // damit wir genau hinter "sessionId=" rauskommen

  const char *end = sessionIdStr; // ende finden, wobei das Ende ";" ist
  while (*end && *end != ';' && *end != '\r' && *end != '\n') {
    end++;
  }

  size_t len = end - sessionIdStr;
  if (len == 0 || len >= 64)
    return NULL;

  char *value =
      malloc(len + 1); // den Speicher für genau diese Länge reservieren

  // memcpy kopiert Bytes von einer Stelle woanders hin -> value ist das Ziel,
  // Zeichen werden von sessionIDstr kopiert
  memcpy(value, sessionIdStr, len);
  value[len] = '\0'; // string beenden mit dem zero character
  return value;      // pointer retournieren
}

void getTimeString(char *buf) {
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
}

// das verwenden wir um aus "Romeo%20und%20Julia" den String
// "Romeo und Julia" zu machen
void urlDecode(const char *src, char *dest) {
  while (*src) {
    if (*src == '%' && isxdigit((unsigned char)src[1]) &&
        isxdigit((unsigned char)src[2])) {

      char hex[3] = {src[1], src[2], '\0'};
      *dest++ = (char)strtol(hex, NULL, 16);
      src += 3;
    } else if (*src == '+') {
      *dest++ = ' ';
      src++;
    } else {
      *dest++ = *src++;
    }
  }

  *dest = '\0';
}

void renderNameOfShow(char auffuehrungName[], int clientSocket) {

  char buffer[1024]; // Zwischenspeicher den wir definieren

  char decoded[100];

  urlDecode(auffuehrungName, decoded);

  snprintf(buffer, sizeof(buffer),
           "<p>Wählen Sie einen Sitzplatz für die Aufführung %s</p>", decoded);
  send(clientSocket, buffer, strlen(buffer), 0);
}

void createSeatNumbers(int clientSocket, PGconn *conn) {

  send(clientSocket,
       "<div class=\"seat-map-shell\">"
       "<div class=\"stage-indicator\">Buehne</div>"
       "<table class=\"seat-map\">\n",
       79, 0);

  for (int r = 0; r < ROWS; r++) {

    send(clientSocket, "<tr>", 4, 0);

    for (int c = 1; c <= COLS; c++) {

      char buffer[128];
      char seat[4];

      snprintf(seat, sizeof(seat), "%c%02d", 'A' + r, c);

      // wir müssen vor dem rendern prüfen ob der Sitzplatz reserviert wurde
      // oder nicht

      int isReserved = checkSeatAvailability(seat, conn);

      if (isReserved) {
        snprintf(buffer, sizeof(buffer),
                 "<td class=\"seat seat-reserved\">%s</td>", seat);
        // diese sitzplätze sollen nicht anklickbar sein!
      } else {
        snprintf(buffer, sizeof(buffer),
                 "<td class=\"seat seat-free\"><a "
                 "href=\"/reservierung.html?sitzplatz=%s\">%s</a></td>",
                 seat, seat);
        // diese schon => weiterleitung auf die reservierungsseite
      }
      send(clientSocket, buffer, strlen(buffer), 0);
    }

    send(clientSocket, "</tr>\n", 6, 0);
  }

  send(clientSocket, "</table></div>\n", 15, 0);
}

void renderReservation(int clientSocket, const Session *session) {
  char buffer[1024]; // Zwischenspeicher den wir definieren

  // in der Session haben wir jetzt alle Informationen, die wir brauchen
  // außer Reservierungsnummer => reservierung wurde noch nicht gemacht

  char decodedAuffuehrung[100];

  urlDecode(session->auffuehrungName, decodedAuffuehrung);

  snprintf(buffer, sizeof(buffer), "<p>Aufführung:  %s</p>\n",
           decodedAuffuehrung);
  send(clientSocket, buffer, strlen(buffer), 0);
  snprintf(buffer, sizeof(buffer), "<p>Kundennummer:  %d</p>\n",
           session->kundenNummer);
  send(clientSocket, buffer, strlen(buffer), 0);
  snprintf(buffer, sizeof(buffer), "<p>Sitzplatz:  %s</p>\n",
           session->sitzplatz);
  send(clientSocket, buffer, strlen(buffer), 0);

  snprintf(buffer, sizeof(buffer),
           "<form action=\"/reservierung-machen\" method=\"GET\">");
  send(clientSocket, buffer, strlen(buffer), 0);

  snprintf(buffer, sizeof(buffer),
           "<input type=\"submit\" value=\"Reservieren\">");
  send(clientSocket, buffer, strlen(buffer), 0);

  snprintf(buffer, sizeof(buffer), "</form>");
  send(clientSocket, buffer, strlen(buffer), 0);

  // wenn das abgeschickt wird, bekommen wir /reservieren
}

int generateRandomNumber() { return rand() % 1000000 + 1; }

void renderSuccess(int clientSocket, const Session *session) {
  char buffer[1024]; // Zwischenspeicher den wir definieren

  char decodedName[100];
  urlDecode(session->auffuehrungName, decodedName);

  snprintf(
      buffer, sizeof(buffer),
      "<p>Sie haben gebucht:<hr><br>Aufführung: %s am %s, um %s<br>Sitzplatz: "
      "%s<br><br/><b>Ihre Reservierungsnummer lautet: %d</b></p>",
      decodedName, session->datumAuffuehrung, session->uhrzeitAuffuehrung,
      session->sitzplatz, session->reservierungsNummer);
  send(clientSocket, buffer, strlen(buffer), 0);
}

void getRegistrationData(int clientSocket, PGconn *conn) {

  char buffer[1024];

  // Formular ausgeben
  snprintf(buffer, sizeof(buffer),
           "<form action=\"/register\" method=\"POST\">"

           "<label for=\"svnr\">Sozialversicherungsnummer:</label><br>"
           "<input type=\"text\" id=\"svnr\" name=\"svnr\"><br><br>"

           "<label for=\"gebdt\">Geburtsdatum:</label><br>"
           "<input type=\"date\" id=\"gebdt\" name=\"gebdt\"><br><br>"

           "<label for=\"vorname\">Vorname:</label><br>"
           "<input type=\"text\" id=\"vorname\" name=\"vorname\"><br><br>"

           "<label for=\"nachname\">Nachname:</label><br>"
           "<input type=\"text\" id=\"nachname\" name=\"nachname\"><br><br>"

           "<label for=\"strasse\">Straße:</label><br>"
           "<input type=\"text\" id=\"strasse\" name=\"strasse\"><br><br>"

           "<label for=\"hausnr\">Hausnummer:</label><br>"
           "<input type=\"text\" id=\"hausnr\" name=\"hausnr\"><br><br>"

           "<label for=\"plz\">PLZ:</label><br>"
           "<input type=\"text\" id=\"plz\" name=\"plz\"><br><br>"

           "<label for=\"ort\">Ort:</label><br>"
           "<input type=\"text\" id=\"ort\" name=\"ort\"><br><br>"

           "<label for=\"artist\">Wählen Sie:</label><br>"
           "<select name=\"artist\" id=\"artist\">");

  send(clientSocket, buffer, strlen(buffer), 0);

  // Optionen rendern
  displayKuenstlerSelection(conn, clientSocket);

  snprintf(buffer, sizeof(buffer),
           "</select><br><br>"
           "<input type=\"submit\" value=\"Anmelden\">"
           "</form>");

  send(clientSocket, buffer, strlen(buffer), 0);
}

void displayKundenNr(int clientSocket, int kundenNr) {

  char buffer[1024]; // Zwischenspeicher den wir definieren

  snprintf(buffer, sizeof(buffer), "<div>Ihre Kundennummer lautet: %d</div>",
           kundenNr);

  send(clientSocket, buffer, strlen(buffer), 0);
}
