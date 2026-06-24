#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // socket APIs
#include <time.h>
#include <unistd.h>

#include "helpers.h"
#include "session.h"

static void terminate(int code, PGresult *res, PGconn *conn) {
  if (code != 0)
    fprintf(stderr, "%s\n", PQerrorMessage(conn));

  if (res != NULL)
    PQclear(res);

  if (conn != NULL)
    PQfinish(conn);

  exit(code);
}

void getAllShows(PGconn *conn, int clientSocket) {

  PGresult *res = NULL;

  res = PQexec(conn, "SELECT * from auffuehrung;");

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    terminate(1, res, conn);
  }

  //   int cols = PQnfields(res);
  int rows = PQntuples(res);
  char buffer[1024]; // Zwischenspeicher den wir definieren

  for (int i = 0; i < rows; i++) {
    char *datum = PQgetvalue(res, i, 0);
    char *uhrzeit = PQgetvalue(res, i, 1);
    char *regisseur = PQgetvalue(res, i, 2);
    char *budget = PQgetvalue(res, i, 3);
    char *name = PQgetvalue(res, i, 4);

    // wir schneiden 3 letzte Chars ab -> sonst Uhrzeit im Sekundenformat
    char uhrzeitCopy[9];
    strcpy(uhrzeitCopy, uhrzeit);
    uhrzeitCopy[strlen(uhrzeitCopy) - 3] = 0;

    // Datum formatieren: Wir möchten statt "2025-07-05" sowas "05.07.2025"
    char datumCopy[11];
    strcpy(datumCopy,
           datum); // zuerst string kopieren um das original nicht zu ändern

    char *token = strtok(datumCopy, "-");
    char *parts[3]; // wir speichern 3 pointers weil es 3 strings sein werden
    int count = 0;

    while (token != NULL) {
      parts[count] = token;
      token = strtok(NULL, " - "); // immer weiter cutten
      count++;
    }

    // neuen string zambauen
    char datumNeu[11] = "";
    strcat(datumNeu, parts[2]);
    strcat(datumNeu, ".");
    strcat(datumNeu, parts[1]);
    strcat(datumNeu, ".");
    strcat(datumNeu, parts[0]);

    // ausgeben
    snprintf(buffer, sizeof(buffer),
             "<tr>"
             "<td><a href=\"/login.html?"
             "name=%s&datum=%s&uhrzeit=%s\">%s</a></td>"
             "<td>%s</td>"
             "<td>%s</td>"
             "<td>%s</td>"
             "<td>%s €</td>"
             "</tr>",
             name, datum, uhrzeit, name, datumNeu, uhrzeitCopy, regisseur,
             budget);

    send(clientSocket, buffer, strlen(buffer), 0);
  }

  PQclear(res);
}

int getIsRegistered(int kundenNummer, PGconn *conn) {
  PGresult *res = NULL;

  if (PQstatus(conn) != CONNECTION_OK) {
    terminate(1, res, conn);
  }

  const char *query = "SELECT * from besucher WHERE Kundennummer = $1";

  char snum[32]; // char array für die kundennr
  snprintf(snum, sizeof(snum), "%d", kundenNummer); // umwandlung in string

  const char *params[1] = {snum}; // hier müssen immer strings übergeben werden

  res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    terminate(1, res, conn);
  }

  int rows = PQntuples(res);
  PQclear(res);

  if (rows == 1) {
    // diese Abfrage macht nur Sinn wenn 1 Zeile zurückkommt
    // ===> er existiert in der Datenbank
    return 1;

  } else {

    return 0;
  }
}

int checkSeatAvailability(char seatNum[], PGconn *conn) {
  PGresult *res = NULL;

  if (PQstatus(conn) != CONNECTION_OK) {
    terminate(1, res, conn);
  }

  const char *query = "SELECT sitzplatz from reservierung WHERE sitzplatz = $1";

  const char *params[1] = {seatNum};
  res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    terminate(1, res, conn);
  }

  int rows = PQntuples(res);
  PQclear(res);

  if (rows == 1) {
    // diese Abfrage macht nur Sinn wenn 1 Zeile zurückkommt
    // ===> er existiert in der Datenbank = sitz ist belegt
    return 1;

  } else {

    return 0;
  }
}

int getSvnrByKundennummer(int kundenNummer, PGconn *conn) {

  PGresult *res = NULL;

  int svnr = -1; // Default: nicht gefunden

  if (PQstatus(conn) != CONNECTION_OK) {
    terminate(1, res, conn);
  }

  const char *query = "SELECT svnr_zahl from besucher WHERE Kundennummer = $1";

  char snum[32];                                    // char array für die svnr
  snprintf(snum, sizeof(snum), "%d", kundenNummer); // umwandlung in string

  const char *params[1] = {snum}; // hier müssen immer strings übergeben werden

  res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    terminate(1, res, conn);
  }

  int rows = PQntuples(res);

  if (rows == 1) {
    // diese Abfrage macht nur Sinn wenn 1 Zeile zurückkommt
    char *value = PQgetvalue(res, 0, 0);
    svnr = atoi(value);
  }

  PQclear(res);

  return svnr;
}

int makeReservation(Session session, PGconn *conn) {
  PGresult *res = NULL;

  if (PQstatus(conn) != CONNECTION_OK) {
    terminate(1, res, conn);
  }

  // random reservierungsnummer generieren
  int reservierungsNum = generateRandomNumber();

  const char *query =
      "INSERT INTO reservierung (reservierungsnummer, "
      "sitzplatz, besucher_id, datum, uhrzeit) VALUES($1, $2, $3, $4, $5)";

  char snum[32]; // char array für die svnr
  snprintf(snum, sizeof(snum), "%d", reservierungsNum); // umwandlung in string

  // time_t now = time(NULL);
  // struct tm *t = localtime(&now);

  // char datum[11];
  // char uhrzeit[9];

  // strftime(datum, sizeof(datum), "%Y-%m-%d", t);
  // strftime(uhrzeit, sizeof(uhrzeit), "%H:%M:%S", t);

  char kundenNummer[32];
  snprintf(kundenNummer, sizeof(kundenNummer), "%d", session.kundenNummer);

  const char *params[5] = {
      snum, session.sitzplatz, kundenNummer, session.datumAuffuehrung,
      session.uhrzeitAuffuehrung}; // hier müssen immer strings übergeben werden

  res = PQexecParams(conn, query, 5, NULL, params, NULL, NULL, 0);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    terminate(1, res, conn);
    return 0;

  } else {
    PQclear(res);
    return reservierungsNum;
  }
}

void getAllKuenstler(PGconn *conn, int clientSocket) {

  PGresult *res = NULL;

  res = PQexec(conn, "SELECT * from kuenstler;");

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    terminate(1, res, conn);
  }

  //   int cols = PQnfields(res);
  int rows = PQntuples(res);
  char buffer[1024]; // Zwischenspeicher den wir definieren

  for (int i = 0; i < rows; i++) {
    char *angestelltenNr = PQgetvalue(res, i, 0);
    char *kuenstlerName = PQgetvalue(res, i, 1);
    char *einstellungsDatum = PQgetvalue(res, i, 2);

    // ausgeben
    snprintf(buffer, sizeof(buffer),
             "<tr>"
             "<td>%s</td>"
             "<td>%s</td>"
             "<td>%s</td>"
             "</tr>",
             angestelltenNr, kuenstlerName, einstellungsDatum);

    send(clientSocket, buffer, strlen(buffer), 0);
  }

  PQclear(res);
}

void displayKuenstlerSelection(PGconn *conn, int clientSocket) {

  PGresult *res = NULL;

  res = PQexec(conn, "SELECT * from kuenstler;");

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    terminate(1, res, conn);
  }

  //   int cols = PQnfields(res);
  int rows = PQntuples(res);
  char buffer[1024]; // Zwischenspeicher den wir definieren

  for (int i = 0; i < rows; i++) {
    char *angestelltenNr = PQgetvalue(res, i, 0);
    char *kuenstlerName = PQgetvalue(res, i, 1);

    int angestellenNummer = atoi(angestelltenNr);

    // ausgeben
    snprintf(buffer, sizeof(buffer), "<option value = \"%d\"> %s</ option>",
             angestellenNummer, kuenstlerName);

    send(clientSocket, buffer, strlen(buffer), 0);
  }

  PQclear(res);
}