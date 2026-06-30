#ifndef DATABASE_H
#define DATABASE_H

#include "session.h"
#include <libpq-fe.h>

void getAllShows(PGconn *conn, int clientSocket, Session *sessionPtr);
int getSvnrByKundennummer(int kundenNummer, PGconn *conn);
int getIsRegistered(int kundenNummer, PGconn *conn);
int checkSeatAvailability(char seatNum[], PGconn *conn);
int makeReservation(Session session, PGconn *conn);
void getAllKuenstler(PGconn *conn, int clientSocket);
void displayKuenstlerSelection(PGconn *conn, int clientSocket);
int makeRegistration(PGconn *conn, char svnr[], char gebdt[], int artistID,
                     char vorname[], char nachname[], char strasse[],
                     char hausnr[], char plz[], char ort[]);

#endif