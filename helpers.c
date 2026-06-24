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

void sendHTTPHeader(int clientSocket) {
  // Funktion die HTTP-Header sendet
  char timeBuf[100];
  getTimeString(timeBuf);
  char resHeader[SIZE];
  sprintf(resHeader,
          "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: text/html\r\n\n",
          timeBuf);
  send(clientSocket, resHeader, strlen(resHeader), 0);
}

void getTimeString(char *buf) {
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
}

// das verwenden wir um aus "Romeo%20und%20Julia" den String
// "Romeo und Julia" zu machen
void urlDecode(char *src, char *dest) {
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

  send(clientSocket, "<table>\n", 8, 0);

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

  send(clientSocket, "</table>\n", 10, 0);
}

void renderReservation(int clientSocket, Session session) {
  char buffer[1024]; // Zwischenspeicher den wir definieren

  // in der Session haben wir jetzt alle Informationen, die wir brauchen
  // außer Reservierungsnummer => reservierung wurde noch nicht gemacht

  char decodedAuffuehrung[100];

  urlDecode(session.auffuehrungName, decodedAuffuehrung);

  snprintf(buffer, sizeof(buffer), "<p>Aufführung:  %s</p>\n",
           decodedAuffuehrung);
  send(clientSocket, buffer, strlen(buffer), 0);
  snprintf(buffer, sizeof(buffer), "<p>Kundennummer:  %d</p>\n",
           session.kundenNummer);
  send(clientSocket, buffer, strlen(buffer), 0);
  snprintf(buffer, sizeof(buffer), "<p>Sitzplatz:  %s</p>\n",
           session.sitzplatz);
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

void renderSuccess(int clientSocket, Session session) {
  char buffer[1024]; // Zwischenspeicher den wir definieren

  char decodedName[100];
  urlDecode(session.auffuehrungName, decodedName);

  snprintf(buffer, sizeof(buffer),
           "<p>Sie haben gebucht:<br>Aufführung: %s am %s, um %s<br>Sitzplatz: "
           "%s<br><b>Ihre Reservierungsnummer lautet: %d</b></p>",
           decodedName, session.datumAuffuehrung, session.uhrzeitAuffuehrung,
           session.sitzplatz, session.reservierungsNummer);
  send(clientSocket, buffer, strlen(buffer), 0);
}

void getRegistrationData(int clientSocket) {

  char buffer[1024]; // Zwischenspeicher den wir definieren

  snprintf(buffer, sizeof(buffer),
           "<form action=\"/login\" method=\"GET\">"

           // felder
           "<label for=\"svnr\">Sozialversicherungsnummer:</label><br>"
           "<input type=\"text\" id=\"svnr\" name=\"svnr\"><br><br>"

           "<label for=\"gebdt\">Geburtsdatum:</label><br>"
           "<input type=\"text\" id=\"gebdt\" name=\"gebdt\"><br><br>"

           // absenden
           "<input type=\"submit\" value=\"Anmelden\">"
           "</form>");

  send(clientSocket, buffer, strlen(buffer), 0);
}