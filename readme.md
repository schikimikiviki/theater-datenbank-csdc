# Theater Datenbank

Wir haben eine kleine postgres Datenbank, die mit Testdaten befüllt ist. Wir möchten über eine Weboberfläche mind. 4 Interaktionen / Zustände abbilden. Die folgenden Seiten werden abgebildet:

- Aufführungen: Hier wählt man die Aufführung, für die man eine Reservierung machen möchte

- Login: entweder man ist bereits Kund *in, oder man muss sich als solche *r anmelden

- Sitzplatz auswählen: Man wählt einen freien Sitzplatz

- Reservierung machen: Es wird eine Reservierung in der Datenbank gespeichert

## Technologien

- Webserver mit C (see: https://github.com/nipunchamikara/c-web-server/tree/main/htdocs)
- postgres Datenbank und libpq für die C Anbindung
- GUI mit HTML, CSS, JS

## Usage

Mit diesem Command können wir kompilieren:

```
make build
```

Mit diesem können wir den Server auch sofort starten:

```
make run
```

## Falls Änderungen nicht angezeigt werden

```
rm server
```
```
make run
```

## Datenbank ausetzen

```
sudo su postgres
```
psql 
```
```
CREATE database theater; 
```
```
exit
```
```
psql --username=postgres theater < ./database/import.sql 

```
