CREATE TABLE ORT (
    plz INTEGER PRIMARY KEY,
    ort VARCHAR(255) NOT NULL
);

CREATE TABLE PERSON (
    svnr_zahl VARCHAR(10) NOT NULL UNIQUE,
    geburtsdatum DATE NOT NULL,
    vorname VARCHAR(255) NOT NULL,
    nachname VARCHAR(255) NOT NULL,
    strasse VARCHAR(255) NOT NULL,
    hausnummer INTEGER NOT NULL,
    plz INTEGER NOT NULL,
    PRIMARY KEY (svnr_zahl, geburtsdatum),
    FOREIGN KEY (plz) REFERENCES ORT(plz)
);

CREATE TABLE BANK (
    bankleitzahl VARCHAR(255) PRIMARY KEY,
    bankname VARCHAR(255) NOT NULL
);

CREATE TABLE GEHALTSKONTO (
    kontonummer INTEGER NOT NULL,
    bankleitzahl VARCHAR(255) NOT NULL,
    kontostand DECIMAL(12,2) NOT NULL,
    PRIMARY KEY (kontonummer, bankleitzahl),
    FOREIGN KEY (bankleitzahl) REFERENCES BANK(bankleitzahl)
);

CREATE TABLE THEATERSTUECK (
    name VARCHAR(255) PRIMARY KEY,
    notwendige_buehnenarbeiter INTEGER NOT NULL,
    probendauer_erfahrungswert INTEGER NOT NULL
);

CREATE TABLE SPRACHE (
    bezeichnung VARCHAR(255) PRIMARY KEY
);

CREATE TABLE AUFFUEHRUNG (
    datum DATE NOT NULL,
    uhrzeit TIME NOT NULL,
    regisseur VARCHAR(255) NOT NULL,
    budget DECIMAL(12,2) NOT NULL,
    theaterstueck_name VARCHAR(255) NOT NULL,
    PRIMARY KEY (datum, uhrzeit),
    FOREIGN KEY (theaterstueck_name) REFERENCES THEATERSTUECK(name)
);

CREATE TABLE ROLLENBUCHTYP (
    isbn VARCHAR(10) PRIMARY KEY,
    autor VARCHAR(255) NOT NULL,
    theaterstueck_name VARCHAR(255) NOT NULL UNIQUE,
    FOREIGN KEY (theaterstueck_name) REFERENCES THEATERSTUECK(name)
);

CREATE TABLE ROLLENBUCH (
    inventarnummer INTEGER PRIMARY KEY,
    isbn VARCHAR(10) NOT NULL,
    FOREIGN KEY (isbn) REFERENCES ROLLENBUCHTYP(isbn)
);

CREATE TABLE TELEFONNUMMER (
    telefonnummer VARCHAR(255),
    UNIQUE (telefonnummer,svnr_zahl),
    FOREIGN KEY (svnr_zahl) REFERENCES PERSON(svnr_zahl)	
);

CREATE TABLE ANGESTELLTER (
    angestelltennummer INTEGER PRIMARY KEY,
    svnr_zahl VARCHAR(10) NOT NULL,
    geburtsdatum DATE NOT NULL,
    kontonummer INTEGER NOT NULL,
    bankleitzahl VARCHAR(255) NOT NULL,
    UNIQUE (svnr_zahl, geburtsdatum),
    UNIQUE (kontonummer, bankleitzahl),
    FOREIGN KEY (svnr_zahl, geburtsdatum)
        REFERENCES PERSON(svnr_zahl, geburtsdatum),
    FOREIGN KEY (kontonummer, bankleitzahl)
        REFERENCES GEHALTSKONTO(kontonummer, bankleitzahl)
);

CREATE TABLE KUENSTLER (
    angestelltennummer INTEGER PRIMARY KEY,
    kuenstlername VARCHAR(255) NOT NULL UNIQUE,
    einstellungsdatum DATE NOT NULL,
    FOREIGN KEY (angestelltennummer)
        REFERENCES ANGESTELLTER(angestelltennummer)
);

CREATE TABLE BUEHNENARBEITER (
    angestelltennummer INTEGER PRIMARY KEY,
    FOREIGN KEY (angestelltennummer)
        REFERENCES ANGESTELLTER(angestelltennummer)
);

CREATE TABLE BESUCHER (
    kundennummer INTEGER PRIMARY KEY,
    svnr_zahl VARCHAR(10) NOT NULL,
    geburtsdatum DATE NOT NULL,
    lieblingskuenstler_nr INTEGER NOT NULL,
    UNIQUE (svnr_zahl, geburtsdatum),
    FOREIGN KEY (svnr_zahl, geburtsdatum)
        REFERENCES PERSON(svnr_zahl, geburtsdatum),
    FOREIGN KEY (lieblingskuenstler_nr)
        REFERENCES KUENSTLER(angestelltennummer)
);

CREATE TABLE AVERSION (
    kuenstler1_nr INTEGER NOT NULL,
    kuenstler2_nr INTEGER NOT NULL,
    PRIMARY KEY (kuenstler1_nr, kuenstler2_nr),
    FOREIGN KEY (kuenstler1_nr)
        REFERENCES KUENSTLER(angestelltennummer),
    FOREIGN KEY (kuenstler2_nr)
        REFERENCES KUENSTLER(angestelltennummer)
);

CREATE TABLE KANN_SPIELEN (
    kuenstler_nr INTEGER NOT NULL,
    sprache VARCHAR(255) NOT NULL,
    stueck VARCHAR(255) NOT NULL,
    PRIMARY KEY (kuenstler_nr, sprache, stueck),
    FOREIGN KEY (kuenstler_nr)
        REFERENCES KUENSTLER(angestelltennummer),
    FOREIGN KEY (sprache)
        REFERENCES SPRACHE(bezeichnung),
    FOREIGN KEY (stueck)
        REFERENCES THEATERSTUECK(name)
);

CREATE TABLE SPIELT_IN (
    kuenstler_nr INTEGER NOT NULL,
    auffuehrung_datum DATE NOT NULL,
    auffuehrung_uhrzeit TIME NOT NULL,
    PRIMARY KEY (kuenstler_nr, auffuehrung_datum, auffuehrung_uhrzeit),
    FOREIGN KEY (kuenstler_nr)
        REFERENCES KUENSTLER(angestelltennummer),
    FOREIGN KEY (auffuehrung_datum, auffuehrung_uhrzeit)
        REFERENCES AUFFUEHRUNG(datum, uhrzeit)
);

CREATE TABLE RESERVIERUNG (
    reservierungsnummer INTEGER PRIMARY KEY,
    sitzplatz VARCHAR(255) NOT NULL,
    besucher_id INTEGER NOT NULL,
    datum DATE NOT NULL,
    uhrzeit TIME NOT NULL,
    FOREIGN KEY (besucher_id)
        REFERENCES BESUCHER(kundennummer),
    FOREIGN KEY (datum, uhrzeit)
        REFERENCES AUFFUEHRUNG(datum, uhrzeit)
);

CREATE TABLE ENTLEIHT (
    inventarnummer INTEGER PRIMARY KEY,
    angestelltennummer INTEGER NOT NULL,
    FOREIGN KEY (inventarnummer)
        REFERENCES ROLLENBUCH(inventarnummer),
    FOREIGN KEY (angestelltennummer)
        REFERENCES ANGESTELLTER(angestelltennummer)
);


CREATE FUNCTION ENTLEIHT()
RETURNS TRIGGER AS
$$
BEGIN
    -- Keine Prüfung nötig, wenn nicht ausgeliehen
    IF NEW.ENTLEIHT IS NULL THEN
        RETURN NEW;
    END IF;

    IF NOT EXISTS (
        SELECT 1
        FROM Kuenstler
        WHERE angestelltennummer = NEW.ENTLEIHT
    )
    AND NOT EXISTS (
        SELECT 1
        FROM Buehnenarbeiter
        WHERE angestelltennummer = NEW.ENTLEIHT
    ) THEN
        RAISE EXCEPTION
        'Nur Künstler oder Bühnenarbeiter dürfen Rollenbücher entleihen.';
    END IF;

    RETURN NEW;
END;
$$
LANGUAGE plpgsql;


CREATE trigger TRG_ENTLEIHT
BEFORE INSERT OR UPDATE
ON Entleiht
FOR EACH ROW
EXECUTE FUNCTION ENTLEIHT();