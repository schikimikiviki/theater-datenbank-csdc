#ifndef DATABASE_H
#define DATABASE_H

#include <libpq-fe.h>

void getAllShows(PGconn *conn, int clientSocket);
int getSvnrByKundennummer(int kundenNummer, PGconn *conn);

#endif