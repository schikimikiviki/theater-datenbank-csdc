#ifndef HELPERS_H
#define HELPERS_H

void getFieldData(int clientSocket);
int startsWith(const char *a, const char *b);
void sendCSSHeader(int clientSocket);
void sendFileToClient(int clientSocket, const char *filepath);
void sendHTTPHeader(int clientSocket);
void renderNameOfShow(char auffuehrungName[], int clientSocket);
void createSeatNumbers(int clientSocket, PGconn *conn);

#endif