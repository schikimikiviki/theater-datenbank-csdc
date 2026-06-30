#ifndef HELPERS_H
#define HELPERS_H

#include "session.h"

void getFieldData(int clientSocket);
int startsWith(const char *a, const char *b);
void sendCSSHeader(int clientSocket);
void sendFileToClient(int clientSocket, const char *filepath);
void sendHTTPHeader(int clientSocket, Session *session);
void sendRedirect(int clientSocket, const char *location, const char *sessionId);
void renderNameOfShow(char auffuehrungName[], int clientSocket);
void createSeatNumbers(int clientSocket, PGconn *conn);
void renderReservation(int clientSocket, const Session *session);
int generateRandomNumber();
void renderSuccess(int clientSocket, const Session *session);
void sendImageHeader(int clientSocket);
void getRegistrationData(int clientSocket, PGconn *conn);
void urlDecode(const char *src, char *dest);
void displayKundenNr(int clientSocket, int kundenNr);
char *extractSessionId(const char *request);

#endif