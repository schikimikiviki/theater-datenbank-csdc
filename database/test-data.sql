INSERT INTO ORT (plz, ort) VALUES
(1010, 'Wien'),
(4020, 'Linz'),
(5020, 'Salzburg'),
(6020, 'Innsbruck'),
(8010, 'Graz');

INSERT INTO PERSON
(svnr_zahl, geburtsdatum, vorname, nachname, strasse, hausnummer, plz)
VALUES
(100001, '1980-01-15', 'Max', 'Mustermann', 'Hauptstrasse', 1, 1010),
(100002, '1985-03-20', 'Anna', 'Mayer', 'Bahnhofstrasse', 12, 4020),
(100003, '1990-07-10', 'Peter', 'Huber', 'Ringstrasse', 8, 5020),
(100004, '1992-11-25', 'Julia', 'Wagner', 'Parkweg', 5, 6020),
(100005, '1988-04-17', 'Thomas', 'Gruber', 'Mozartgasse', 11, 8010),
(100006, '1995-08-30', 'Lisa', 'Schmidt', 'Kirchengasse', 3, 1010),
(100007, '1978-09-09', 'Michael', 'Bauer', 'Marktplatz', 22, 4020),
(100008, '1999-02-14', 'Sarah', 'Hofer', 'Bergweg', 7, 5020),
(100009, '1983-12-01', 'David', 'Leitner', 'Seeweg', 9, 6020),
(100010, '1991-06-05', 'Eva', 'Fuchs', 'Schulgasse', 15, 8010);

INSERT INTO BANK (bankleitzahl, bankname) VALUES
('12000', 'Erste Bank'),
('14000', 'Raiffeisen Bank'),
('15000', 'BAWAG');

INSERT INTO GEHALTSKONTO
(kontonummer, bankleitzahl, kontostand)
VALUES
(1001, '12000', 3500.00),
(1002, '12000', 4200.00),
(1003, '14000', 3900.00),
(1004, '14000', 5100.00),
(1005, '15000', 2800.00);

INSERT INTO ANGESTELLTER
(angestelltennummer, svnr_zahl, geburtsdatum, kontonummer, bankleitzahl)
VALUES
(1, 100001, '1980-01-15', 1001, '12000'),
(2, 100002, '1985-03-20', 1002, '12000'),
(3, 100003, '1990-07-10', 1003, '14000'),
(4, 100004, '1992-11-25', 1004, '14000'),
(5, 100005, '1988-04-17', 1005, '15000');

INSERT INTO KUENSTLER
(angestelltennummer, kuenstlername, einstellungsdatum)
VALUES
(1, 'Max Star', '2015-01-01'),
(2, 'Anna Bühne', '2018-05-01'),
(3, 'Peter Drama', '2020-09-01');

INSERT INTO BUEHNENARBEITER
(angestelltennummer)
VALUES
(4),
(5);

INSERT INTO SPRACHE (bezeichnung) VALUES
('Deutsch'),
('Englisch'),
('Franzoesisch');

INSERT INTO THEATERSTUECK
(name, notwendige_buehnenarbeiter, probendauer_erfahrungswert)
VALUES
('Hamlet', 2, 40),
('Faust', 3, 50),
('Romeo und Julia', 2, 35);

INSERT INTO AUFFUEHRUNG
(datum, uhrzeit, regisseur, budget, theaterstueck_name)
VALUES
('2025-07-01', '19:00', 'Regisseur A', 10000.00, 'Hamlet'),
('2025-07-05', '20:00', 'Regisseur B', 12000.00, 'Faust'),
('2025-07-10', '18:30', 'Regisseur C', 9000.00, 'Romeo und Julia'),
('2025-07-15', '19:30', 'Regisseur A', 11000.00, 'Hamlet');

INSERT INTO ROLLENBUCHTYP
(isbn, autor, theaterstueck_name)
VALUES
('1111111111', 'William Shakespeare', 'Hamlet'),
('2222222222', 'Johann Wolfgang Goethe', 'Faust'),
('3333333333', 'William Shakespeare', 'Romeo und Julia');

INSERT INTO ROLLENBUCH
(inventarnummer, isbn)
VALUES
(1, '1111111111'),
(2, '1111111111'),
(3, '2222222222'),
(4, '3333333333');

INSERT INTO TELEFONNUMMER
(telefonnummer)
VALUES
('06641234567'),
('06641234568'),
('06641234569'),
('06641234570'),
('06641234571');

INSERT INTO BESUCHER
(kundennummer, svnr_zahl, geburtsdatum, lieblingskuenstler_nr)
VALUES
(100, 100006, '1995-08-30', 1),
(101, 100007, '1978-09-09', 2),
(102, 100008, '1999-02-14', 3),
(103, 100009, '1983-12-01', 1);

INSERT INTO AVERSION
(kuenstler1_nr, kuenstler2_nr)
VALUES
(1, 2),
(2, 3);

INSERT INTO KANN_SPIELEN
(kuenstler_nr, sprache, stueck)
VALUES
(1, 'Deutsch', 'Hamlet'),
(1, 'Englisch', 'Hamlet'),
(2, 'Deutsch', 'Faust'),
(3, 'Deutsch', 'Romeo und Julia'),
(3, 'Franzoesisch', 'Romeo und Julia');

INSERT INTO SPIELT_IN
(kuenstler_nr, auffuehrung_datum, auffuehrung_uhrzeit)
VALUES
(1, '2025-07-01', '19:00'),
(1, '2025-07-15', '19:30'),
(2, '2025-07-05', '20:00'),
(3, '2025-07-10', '18:30');

INSERT INTO RESERVIERUNG
(reservierungsnummer, sitzplatz, besucher_id, datum, uhrzeit)
VALUES
(1, 'A01', 100, '2025-07-01', '19:00'),
(2, 'A02', 101, '2025-07-01', '19:00'),
(3, 'B01', 102, '2025-07-05', '20:00'),
(4, 'C03', 103, '2025-07-10', '18:30');

INSERT INTO ENTLEIHT
(inventarnummer, angestelltennummer)
VALUES
(1, 1),
(3, 2);
