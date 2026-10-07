BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "doctors" (
	"id"	INTEGER,
	"fio"	TEXT NOT NULL,
	"speciality"	TEXT NOT NULL,
	"office"	INTEGER NOT NULL,
	PRIMARY KEY("id" AUTOINCREMENT),
	CHECK("office" > 0)
);
CREATE TABLE IF NOT EXISTS "history" (
	"id"	INTEGER,
	"ticket_id"	INTEGER NOT NULL,
	"patient_id"	INTEGER NOT NULL,
	"action_time"	TEXT NOT NULL,
	"action_type"	TEXT NOT NULL CHECK("action_type" IN ('запись', 'отмена')),
	PRIMARY KEY("id" AUTOINCREMENT),
	FOREIGN KEY("patient_id") REFERENCES "patients"("id"),
	FOREIGN KEY("ticket_id") REFERENCES "tickets"("id")
);
CREATE TABLE IF NOT EXISTS "patients" (
	"id"	INTEGER,
	"fio"	TEXT NOT NULL,
	"phone"	TEXT NOT NULL UNIQUE,
	PRIMARY KEY("id" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "tickets" (
	"id"	INTEGER,
	"doctor_id"	INTEGER NOT NULL,
	"date"	TEXT NOT NULL,
	"time"	TEXT NOT NULL,
	"patient_id"	INTEGER,
	PRIMARY KEY("id" AUTOINCREMENT),
	FOREIGN KEY("doctor_id") REFERENCES "doctors"("id"),
	FOREIGN KEY("patient_id") REFERENCES "patients"("id")
);
COMMIT;
