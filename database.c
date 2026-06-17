#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // socket APIs
#include <unistd.h>

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

  printf("DEBUG: entered getAllShows\n");

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
    // char *regisseur = PQgetvalue(res, i, 2);
    // char *budget = PQgetvalue(res, i, 3);
    // char *name = PQgetvalue(res, i, 4);

    snprintf(buffer, sizeof(buffer), "<tr><td>%s</td><td>%s</td></tr>", datum,
             uhrzeit);

    send(clientSocket, buffer, strlen(buffer), 0);
  }

  PQclear(res);
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

// sonst:  if (PQresultStatus(res) != PGRES_COMMAND_OK) wenn POST, UPDATE