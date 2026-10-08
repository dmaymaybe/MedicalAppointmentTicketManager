#pragma once
#include "Database.h"
#include "Validation.h"
#include <string>
#include <vector>
#include <optional>



// Добавление врача
enum class AddDoctorResult
{
	Ok,                // Успех
	InvalidFio,        // Неверное ФИО
	EmptySpeciality,   // Пустая специальность
	InvalidOffice,     // Неверный номер кабинета
	DatabaseError      // Ошибка БД
};

// Удаление врача
enum class RemoveDoctorResult
{
	Ok,                // Успех
	DoctorNotFound,    // Врач не найден
	HasTickets,        // У врача есть талоны
	DatabaseError      // Ошибка БД
};

// Создание талона
enum class CreateTicketResult
{
	Ok,                // Успех
	DoctorNotFound,    // Врач не найден
	InvalidDate,       // Неверная дата
	InvalidTime,       // Неверное время
	TicketInPast,      // Дата/время уже прошли
	Duplicate,         // Такой талон уже есть
	DatabaseError      // Ошибка БД
};

// Регистрация пациента
enum class RegisterPatientResult
{
	Ok,                // Успех
	InvalidFio,        // Неверное ФИО
	InvalidPhone,      // Неверный телефон
	PhoneTaken,        // Телефон уже занят
	DatabaseError      // Ошибка БД
};

// Вход пациента по телефону
enum class LoginPatientResult
{
	Ok,                // Успех
	InvalidPhone,      // Неверный формат телефона
	NotFound,          // Пациент не найден
	DatabaseError      // Ошибка БД
};

// Поиск свободных талонов
enum class FreeSlotsResult
{
	Ok,                // Успех
	DoctorNotFound,    // Врач не найден
	InvalidDate,       // Неверная дата
	DatabaseError      // Ошибка БД
};

// Запись пациента на талон
enum class BookResult
{
	Ok,                              // Успех
	TicketNotFound,                  // Талон не найден
	TicketAlreadyBooked,             // Талон уже занят
	PatientNotFound,                 // Пациент не найден
	AlreadyBookedWithDoctorOnDate,   // Уже записан к этому врачу в этот день
	TicketInPast,                    // Талон уже в прошлом
	DatabaseError                    // Ошибка БД
};

// Отмена записи
enum class CancelResult
{
	Ok,                 // Успех
	TicketNotFound,     // Талон не найден
	TicketAlreadyFree,  // Талон уже свободен
	NotPatientTicket,   // Талон принадлежит другому пациенту
	DatabaseError       // Ошибка БД
};



// Ответ на вход пациента
struct LoginPatientResponse
{
	LoginPatientResult status = LoginPatientResult::DatabaseError;
	std::optional<Patient> patient;    // Найденный пациент
};

// Ответ на поиск свободных талонов
struct FreeSlotsResponse
{
	FreeSlotsResult status = FreeSlotsResult::DatabaseError;
	std::vector<Ticket> tickets;       // Найденные талоны
};



class Service
{
public:
	Service(Database& db, Validation validation = Validation());

	AddDoctorResult addDoctor(const std::string& fio, const std::string& speciality, const std::string& office);
	RemoveDoctorResult removeDoctor(const std::string& id);
	std::vector<Doctor> getDoctors();

	RegisterPatientResult registerPatient(const std::string& fio, const std::string& phone);
	LoginPatientResponse loginPatient(const std::string& phone) const;
	std::vector<Patient> getPatients();

	CreateTicketResult createTicket(const std::string& doctor_id, const std::string& date, const std::string& time);
	FreeSlotsResponse getFreeSlots(const std::string& speciality, const std::string& doctor_fio, const std::string& date) const;
	FreeSlotsResponse getFreeSlotsByDoctor(const std::string& doctor_id, const std::string& date) const;
	std::vector<Ticket> getAllTickets();
	std::vector<Ticket> getPatientTickets(int patient_id);

	BookResult book(const std::string& ticket_id, int patient_id);
	CancelResult cancel(const std::string& ticket_id, int patient_id);

	std::vector<HistoryRecord> getHistory();
private:
	Database& db;
	Validation validation;

	std::optional<int> parseInt(const std::string& s) const;
};

