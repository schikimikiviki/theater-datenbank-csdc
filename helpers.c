#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h> // time

#define SIZE 1024 // buffer size

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
