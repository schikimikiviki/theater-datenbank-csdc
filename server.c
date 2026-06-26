#define _GNU_SOURCE // damit kann ich intellisense sagen - verwende GNU / POSIX
                    // Erweiterungen (für flags)
#include <netdb.h>  // getnameinfo
#include <stdio.h>  // console input/output, perror
#include <stdlib.h> // exit
#include <string.h> // string manipulation

#include <netinet/in.h> // sockaddr_in
#include <sys/socket.h> // socket APIs
#include <unistd.h>     // open, close

// unsere files einbinden
#include "database.h"
#include "helpers.h"
#include "session.h"

#include <libpq-fe.h> // das ist für die postgres anbindung
#include <signal.h>   // signal handling
#include <time.h>     // time

#define SIZE 1024  // buffer size
#define PORT 8081  // port number
#define BACKLOG 10 // number of pending connections queue will hold

/**
 * @brief Generates file URL based on route
 * @param route requested route
 * @param fileurl generated url
 */
void getFileURL(char *route, char *fileURL);

/**
 * @brief Sets *MIME to the mime type of file
 * @param file file URL
 * @param mime mime type of file
 */
void getMimeType(char *file, char *mime);

/**
 * @brief Handles SIGINT signal
 */
void handleSignal(int signal);

void sendFileToClient(int clientSocket, const char *filepath);

void sendHTTPHeader(int clientSocket);

int serverSocket;
int clientSocket;

char *request;
PGconn *conn = NULL;

int main() {

  // datenbankverbindung aufbauen
  conn = PQconnectdb(
      "user=postgres password=postgres host=127.0.0.1 dbname=theater");

  if (PQstatus(conn) != CONNECTION_OK) {
    fprintf(stderr, "DB connection failed: %s\n", PQerrorMessage(conn));
    PQfinish(conn);
    return 1;
  }

  // Session aufbauen damit wir später Daten speichern können
  Session session; // TODO: session ID setzen

  // zufallsgenerator für IDs initialisieren => sonst immer dieselben
  srand((unsigned)time(NULL));

  // register signal handler
  signal(SIGINT, handleSignal);
  signal(SIGPIPE, SIG_IGN); //
  // Standardverhalten von SIGPIPE: Prozess sofort beenden – ohne Fehlermeldung
  // notwendig wenn wir viele Db abfragen hintereinander machen
  // sig_ign heißt "ignore"

  // server internet socket address
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET; // IPv4
  serverAddress.sin_port =
      htons(PORT); // port number in network byte order (host-to-network short)
  serverAddress.sin_addr.s_addr =
      htonl(INADDR_LOOPBACK); // localhost (host to network long)

  // socket of type IPv4 using TCP protocol
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  // reuse address and port
  setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

  // bind socket to address
  if (bind(serverSocket, (struct sockaddr *)&serverAddress,
           sizeof(serverAddress)) < 0) {
    printf("Error: The server is not bound to the address.\n");
    return 1;
  }

  // listen for connections
  if (listen(serverSocket, BACKLOG) < 0) {
    printf("Error: The server is not listening.\n");
    return 1;
  }

  // Hier wird Speicher reserviert:
  // NI_MAXHOST ≈ 1025
  // NI_MAXSERV ≈ 32
  char hostBuffer[NI_MAXHOST];
  char serviceBuffer[NI_MAXSERV];

  // getnameinfo ist eine Funktion aus netdb.h und wird vom OS bereitgestellt
  // die flag NI_NUMERICSERV steht für den numerischen Wert vom Port
  // setzt man das auf 0, hat man irgendeinen komischen alias namen in der URL
  // der auf den Port gemapped ist

  int error = getnameinfo((struct sockaddr *)&serverAddress,
                          sizeof(serverAddress), hostBuffer, sizeof(hostBuffer),
                          serviceBuffer, sizeof(serviceBuffer), NI_NUMERICSERV);

  if (error != 0) {
    printf("Error: %s\n", gai_strerror(error));
    return 1;
  }

  printf("\nServer is listening on http://%s:%s/\n\n", hostBuffer,
         serviceBuffer);

  while (1) {
    char method[10], route[100];

    // accept connection and read data
    clientSocket = accept(serverSocket, NULL, NULL);

    if (clientSocket < 0) {
      perror("accept");
      continue;
    }

    // request buffer
    request = (char *)malloc(SIZE * sizeof(char));
    int totalRead = read(clientSocket, request, SIZE - 1);
    request[totalRead] = '\0';

    // content-Length parsen
    char *contentLengthStr = strstr(request, "Content-Length: ");
    int contentLength = 0;
    if (contentLengthStr) {
      contentLengthStr += strlen("Content-Length: ");
      contentLength = atoi(contentLengthStr);
    }

    // Body nach \r\n\r\n suchen
    char *bodyStart = strstr(request, "\r\n\r\n");
    char *body = NULL;
    if (bodyStart) {
      bodyStart += 4; // hinter \r\n\r\n
      int bodyAlreadyRead = totalRead - (bodyStart - request);
      if (bodyAlreadyRead < contentLength) {
        // es fehlen noch Bytes – nachlesen
        int remaining = contentLength - bodyAlreadyRead;
        char *extraBuf = malloc(remaining + 1);
        int extraRead = read(clientSocket, extraBuf, remaining);
        extraBuf[extraRead] = '\0';

        body = malloc(contentLength + 1);
        memcpy(body, bodyStart, bodyAlreadyRead);
        memcpy(body + bodyAlreadyRead, extraBuf, extraRead);
        body[contentLength] = '\0';
        free(extraBuf);
      } else {
        // Body ist bereits komplett im Buffer
        body = malloc(bodyAlreadyRead + 1);
        memcpy(body, bodyStart, bodyAlreadyRead);
        body[bodyAlreadyRead] = '\0';
      }
    }

    // parse HTTP request
    sscanf(request, "%s %s", method, route);
    printf("%s %s", method, route);

    free(request);

    // hier zuerst Parameter rausballern, strchr ist wie split() und gibt
    // pointer zurück
    char *query = strchr(route, '?');

    if (query != NULL) {
      *query = '\0'; // route endet jetzt bei '?'
      query++;       // zeigt auf "name=Hamlet"
    }

    if (strcmp(route, "/static/styles.css") == 0) {
      sendCSSHeader(clientSocket);
      sendFileToClient(clientSocket, "htdocs/static/styles.css");
    } else if (strcmp(route, "/index") == 0 ||
               strcmp(route, "/index.html") == 0 || strcmp(route, "/") == 0) {

      sendHTTPHeader(clientSocket);

      sendFileToClient(clientSocket, "htdocs/index.html");
    } else if (strcmp(route, "/auffuehrungen") == 0 ||
               strcmp(route, "/auffuehrungen.html") == 0) {

      sendHTTPHeader(clientSocket);

      // Header-HTML
      sendFileToClient(clientSocket, "htdocs/auffuehrungen-header.html");

      // Dynamische Tabellen-Zeilen
      getAllShows(conn, clientSocket);

      // Footer-HTML
      sendFileToClient(clientSocket, "htdocs/auffuehrungen-footer.html");

    } else if (strcmp(route, "/login") == 0 ||
               strcmp(route, "/login.html") == 0) {

      // hier Parameter aus der URL parsen
      // Achtung:
      // es werden hier 2 Arten von Parametern übergeben:
      // erstens, wenn wir von der "aufführungsseite" auf das Login kommen,
      // dann haben wir "auffuehrung=Hamlet&time=...&date=..."
      // zweitens, wenn wir das Feld mit der Kundennummer ausfüllen
      // und dann haben wir "kundenID=123"

      char *token = strtok(query, "&");
      // wir müssen jetzt auf "&" splitten

      while (token != NULL) {

        char *value = strchr(token, '=');

        if (value != NULL) {
          *value = '\0';
          value++;

          if (strcmp(token, "name") == 0) {

            strcpy(session.auffuehrungName,
                   value); // wir kopieren den String in die Session

          } else if (strcmp(token, "datum") == 0) {
            strcpy(session.datumAuffuehrung, value);
          } else if (strcmp(token, "uhrzeit") == 0) {
            strcpy(session.uhrzeitAuffuehrung, value);
          } else if (strcmp(token, "kundenID") == 0) {

            int kundenNr = atoi(value);
            session.kundenNummer =
                kundenNr; // wir kopieren den int in die Session

            // hier müssen wir jetzt prüfen ob das eine valide Kundennummer
            // ist

            int isRegistered = getIsRegistered(kundenNr, conn);

            if (isRegistered) {

              // loggedIn auf true setzen in der session
              session.loggedIn = 1;

              // wir leiten auf die nächste Seite weiter
              char response[512];

              snprintf(response, sizeof(response),
                       "HTTP/1.1 302 Found\r\n"
                       "Location: /sitzplatz.html\r\n"
                       "Content-Length: 0\r\n"
                       "\r\n");

              send(clientSocket, response, strlen(response), 0);

            } else {

              // wenn falsche Kundennummer: man kann sich registrieren
              // ==> sign up page anzeigen

              // wir leiten auf die nächste Seite weiter
              char response[512];

              snprintf(response, sizeof(response),
                       "HTTP/1.1 302 Found\r\n"
                       "Location: /register.html\r\n"
                       "Content-Length: 0\r\n"
                       "\r\n");

              send(clientSocket, response, strlen(response), 0);
            }
          }
        }

        token = strtok(NULL, "&");
      }

      sendHTTPHeader(clientSocket);

      sendFileToClient(clientSocket, "htdocs/login-header.html");

      // formulareingaben holen
      getFieldData(clientSocket);

      sendFileToClient(clientSocket, "htdocs/login-footer.html");

    } else if (strcmp(route, "/sitzplatz") == 0 ||
               strcmp(route, "/sitzplatz.html") == 0) {

      sendHTTPHeader(clientSocket);

      sendFileToClient(clientSocket, "htdocs/sitzplatz-header.html");

      renderNameOfShow(session.auffuehrungName, clientSocket);
      createSeatNumbers(clientSocket, conn);

      sendFileToClient(clientSocket, "htdocs/sitzplatz-footer.html");

    } else if (strcmp(route, "/reservierung") == 0 ||
               strcmp(route, "/reservierung.html") == 0) {

      // parameter parsen
      char *param = strchr(query, '=');

      if (param != NULL) {
        *param = '\0'; // query endet jetzt bei '='
        param++;       // zeigt auf "A01"
      }

      strcpy(session.sitzplatz,
             param); // wir kopieren den String in die Session

      sendHTTPHeader(clientSocket);

      sendFileToClient(clientSocket, "htdocs/reservierung-header.html");

      renderReservation(clientSocket, session);

      sendFileToClient(clientSocket, "htdocs/reservierung-footer.html");

    } else if (strcmp(route, "/reservierung-machen") == 0 ||
               strcmp(route, "/reservierung-machen.html") == 0) {

      // zuerst machen wir den db request
      // je nachdem, ob das klappt, zeigen wir verschiedene inhalte an

      int reservationNum = makeReservation(session, conn);

      if (reservationNum != 0) {
        // Null steht hier für fehlgeschlagen
        // success seite anzeigen + reservierungsnummer

        printf("RESERVIERUNG ERFOLGREICH: %d", reservationNum);
        session.reservierungsNummer = reservationNum;
        char response[512];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 302 Found\r\n"
                 "Location: /reservierung-erfolgreich?nummer=%d\r\n"
                 "Content-Length: 0\r\n"
                 "\r\n",
                 reservationNum);
        send(clientSocket, response, strlen(response), 0);
      } else {
        // Fehlermeldung wenns nicht geklappt hat & auf startseite
        // zurückleiten
        sendHTTPHeader(clientSocket);

        sendFileToClient(clientSocket, "htdocs/error.html");
      }

    } else if (strcmp(route, "/reservierung-erfolgreich") == 0 ||
               strcmp(route, "/reservierung-erfolgreich.html") == 0) {

      sendHTTPHeader(clientSocket);
      sendFileToClient(clientSocket, "htdocs/success-header.html");
      renderSuccess(clientSocket, session);
      sendFileToClient(clientSocket, "htdocs/success-footer.html");
    } else if (startsWith(route, "/static/images/")) {
      char path[512];

      snprintf(path, sizeof(path), "htdocs%s", route);

      sendImageHeader(clientSocket);
      sendFileToClient(clientSocket, path);
    } else if (strcmp(route, "/register") == 0 ||
               strcmp(route, "/register.html") == 0) {

      // parameter einsammeln für den POST request
      char svnr[11] = "";
      char gebdt[11] = "";
      char vorname[50] = "";
      char nachname[50] = "";
      char strasse[100] = "";
      char hausnr[10] = "";
      char plz[10] = "";
      char ort[10] = "";
      int artistId = -1;

      char *token = strtok(body, "&");
      while (token != NULL) {
        char *value = strchr(token, '=');
        if (value != NULL) {
          *value = '\0';
          value++;
          // urlDecode auf value anwenden
          char decoded[256];
          urlDecode(value, decoded);

          if (strcmp(token, "svnr") == 0) {
            strcpy(svnr, decoded);
          } else if (strcmp(token, "gebdt") == 0) {
            strcpy(gebdt, decoded);
          } else if (strcmp(token, "vorname") == 0) {
            strcpy(vorname, decoded);
          } else if (strcmp(token, "nachname") == 0) {
            strcpy(nachname, decoded);
          } else if (strcmp(token, "strasse") == 0) {
            strcpy(strasse, decoded);
          } else if (strcmp(token, "hausnr") == 0) {
            strcpy(hausnr, decoded);
          } else if (strcmp(token, "plz") == 0) {
            strcpy(plz, decoded);
          } else if (strcmp(token, "artist") == 0) {
            artistId = atoi(decoded);
          } else if (strcmp(token, "ort") == 0) {
            strcpy(ort, decoded);
          }
        }
        token = strtok(NULL, "&");
      }

      if (strlen(svnr) > 0 && strlen(gebdt) > 0 && strlen(vorname) > 0 &&
          strlen(nachname) > 0 && strlen(strasse) > 0 && strlen(hausnr) > 0 &&
          strlen(plz) > 0 && strlen(ort) > 0 && artistId != -1) {

        // registierung machen
        printf("\nWir haben eine Registrierung erhalten... \n");
        printf("SVNR: %s Geburtsdatum: %s ArtistID: %d", svnr, gebdt, artistId);

        int kundenNummer =
            makeRegistration(conn, svnr, gebdt, artistId, vorname, nachname,
                             strasse, hausnr, plz, ort);

        if (kundenNummer != 0) {

          // POST Request war erfolgreich
          // ==> wir zeigen die KNR an und leiten weiter auf das Login

          char response[512];

          snprintf(response, sizeof(response),
                   "HTTP/1.1 302 Found\r\n"
                   "Location: /reservation-success.html?kundenNummer=%d\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n",
                   kundenNummer);

          send(clientSocket, response, strlen(response), 0);

        } else {
          // Fehlermeldung anzeigen
          sendHTTPHeader(clientSocket);

          sendFileToClient(clientSocket, "htdocs/error.html");
        }
      }

      free(body);

      // sonst einfach nur die registrierungsseite anzeigen
      sendHTTPHeader(clientSocket);

      sendFileToClient(clientSocket, "htdocs/register-header.html");

      // formulareingaben holen
      getRegistrationData(clientSocket, conn);

      sendFileToClient(clientSocket, "htdocs/register-footer.html");
    }

    else if (strcmp(route, "/kuenstler") == 0 ||
             strcmp(route, "/kuenstler.html") == 0) {

      sendHTTPHeader(clientSocket);

      sendFileToClient(clientSocket, "htdocs/kuenstler-header.html");

      // künstler listen
      getAllKuenstler(conn, clientSocket);

      sendFileToClient(clientSocket, "htdocs/kuenstler-footer.html");
    } else if (strcmp(route, "/reservation-success.html") == 0 ||
               strcmp(route, "/reservation-success") == 0) {

      // parameter parsen
      char *param = strchr(query, '=');

      if (param != NULL) {
        *param = '\0'; // query endet jetzt bei '='
        param++;       // zeigt auf "12334"
      }

      sendHTTPHeader(clientSocket);

      sendFileToClient(clientSocket, "htdocs/reservation-success-header.html");

      int kundenNr = atoi(param);
      displayKundenNr(clientSocket, kundenNr);

      sendFileToClient(clientSocket, "htdocs/reservation-success-footer.html");

    }

    else {

      // hier default seite anzeigen

      sendHTTPHeader(clientSocket);

      sendFileToClient(clientSocket, "htdocs/fallback.html");
    }

    close(clientSocket);
  }
}

// wir übergeben hier pointer zu den Strings statt ganze Strings
// => das ist effizienter, weil man nur die Addresse des ersten Zeichens des
// Strings übergeben muss!

void getFileURL(char *route, char *fileURL) {
  // if route has parameters, remove them
  char *question = strrchr(route, '?');
  if (question)
    *question = '\0';

  // if route is empty, set it to index.html
  if (route[strlen(route) - 1] == '/') {
    strcat(route, "index.html");
  }

  // get filename from route
  strcpy(fileURL, "htdocs");
  strcat(fileURL, route);

  // if filename does not have an extension, set it to .html
  const char *dot = strrchr(fileURL, '.');
  if (!dot || dot == fileURL) {
    strcat(fileURL, ".html");
  }
}

void getMimeType(char *file, char *mime) {
  // position in string with period character
  const char *dot = strrchr(file, '.');

  // if period not found, set mime type to text/html
  if (dot == NULL)
    strcpy(mime, "text/html");

  else if (strcmp(dot, ".html") == 0)
    strcpy(mime, "text/html");

  else if (strcmp(dot, ".css") == 0)
    strcpy(mime, "text/css");

  else if (strcmp(dot, ".js") == 0)
    strcpy(mime, "application/js");

  else if (strcmp(dot, ".jpg") == 0)
    strcpy(mime, "image/jpeg");

  else if (strcmp(dot, ".png") == 0)
    strcpy(mime, "image/png");

  else if (strcmp(dot, ".gif") == 0)
    strcpy(mime, "image/gif");

  else
    strcpy(mime, "text/html");
}

void handleSignal(int signal) {
  if (signal == SIGINT) {
    printf("\nShutting down server...\n");

    close(clientSocket);
    close(serverSocket);

    PQfinish(conn);

    if (request != NULL)
      free(request);

    exit(0);
  }
}
