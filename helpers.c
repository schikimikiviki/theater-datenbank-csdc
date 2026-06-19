#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

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
