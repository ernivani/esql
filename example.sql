CREATE DATABASE boutique;

USE boutique;

CREATE TABLE alchool (nom:varchar(50),pourcentage:int,couleur:varchar(20),prix:int);

INSERT INTO alchool VALUES ('vodka', 40, 'transparent', 20);
INSERT INTO alchool VALUES ('whisky', 45, 'amber', 25);
INSERT INTO alchool VALUES ('red wine', 12, 'red', 15);

SELECT * FROM alchool;

SELECT * FROM alchool WHERE pourcentage = 40;

SELECT * FROM alchool WHERE couleur = 'red' OR couleur = 'amber';

SELECT * FROM alchool WHERE prix <= 20 AND pourcentage > 10;

INSERT INTO alchool VALUES ('gin', 37, 'transparent', 22);
INSERT INTO alchool VALUES ('rum', 38, 'dark', 18);

SELECT * FROM alchool WHERE pourcentage >= 35 AND pourcentage <= 45;

SELECT * FROM alchool WHERE (couleur = 'dark' OR couleur = 'amber') AND prix < 25;

SELECT * FROM alchool WHERE nom = 'vodka' AND prix > 15;

SELECT * FROM alchool;
