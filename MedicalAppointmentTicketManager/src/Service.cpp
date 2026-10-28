#include "Service.h"
#include <sqlite3.h>
#include <chrono>
#include <ctime>
#include <tuple>
#include <algorithm>



// Разбор строки в int
std::optional<int> Service::parseInt(const std::string& s) const {
	try {
		size_t pos = 0;
		int val = std::stoi(s, &pos); // Преобразуем строку в int, pos — индекс первого символа после числа
		if (pos != s.size()) return std::nullopt;
		return val;
	}
	catch (...) {
		return std::nullopt;
	}
}



Service::Service(Database& db, Validation validation) : db(db), validation(validation) {}



// Добавление нового врача
AddDoctorResult Service::addDoctor(const std::string& fio, const std::string& speciality, const std::string& office) {
	// Проверка ФИО
	if (!validation.isValidFio(fio))
		return AddDoctorResult::InvalidFio;

	// Проверка специальности
	if (speciality.empty())
		return AddDoctorResult::EmptySpeciality;

	// Проверка кабинета
	auto officeOpt = parseInt(office);
	if (!officeOpt || !validation.isValidOffice(*officeOpt))
		return AddDoctorResult::InvalidOffice;

	try {
		Doctor d(fio, speciality, *officeOpt);
		db.addDoctor(d);
		return AddDoctorResult::Ok;
	}
	catch (const SQLite::Exception&) {
		return AddDoctorResult::DatabaseError;
	}
}



// Удаление врача
RemoveDoctorResult Service::removeDoctor(const std::string& id) {
	auto idOpt = parseInt(id);
	if (!idOpt)
		return RemoveDoctorResult::DoctorNotFound;

	try {
		// Проверка существования врача
		auto doc = db.findDoctorById(*idOpt);
		if (!doc)
			return RemoveDoctorResult::DoctorNotFound;

		// Проверка отсутствия талонов
		if (db.hasActiveTickets(*idOpt))
			return RemoveDoctorResult::HasTickets;

		db.removeDoctor(*idOpt);
		return RemoveDoctorResult::Ok;
	}
	catch (const SQLite::Exception&) {
		return RemoveDoctorResult::DatabaseError;
	}
}



// Получение списка врачей
std::vector<Doctor> Service::getDoctors() {
	return db.listDoctors();
}



// Регистрация нового пациента
RegisterPatientResult Service::registerPatient(const std::string& fio, const std::string& phone) {
	// Проверка ФИО
	if (!validation.isValidFio(fio))
		return RegisterPatientResult::InvalidFio;

	// Проверка телефона
	if (!validation.isValidPhone(phone))
		return RegisterPatientResult::InvalidPhone;

	try {
		// Проверка, что телефон еще не занят
		if (db.findPatientByPhone(phone))
			return RegisterPatientResult::PhoneTaken;

		Patient p(fio, phone);
		db.addPatient(p);
		return RegisterPatientResult::Ok;
	}
	catch (const SQLite::Exception&) {
		return RegisterPatientResult::DatabaseError;
	}
}



// Вход пациента по номеру телефона
LoginPatientResponse Service::loginPatient(const std::string& phone) const {
	LoginPatientResponse resp;

	// Проверка формата телефона
	if (!validation.isValidPhone(phone)) {
		resp.status = LoginPatientResult::InvalidPhone;
		return resp;
	}

	try {
		// Поиск пациента в БД
		auto p = db.findPatientByPhone(phone);
		if (!p) {
			resp.status = LoginPatientResult::NotFound;
			return resp;
		}

		resp.status = LoginPatientResult::Ok;
		resp.patient = *p;
	}
	catch (const SQLite::Exception&) {
		resp.status = LoginPatientResult::DatabaseError;
	}

	return resp;
}



// Получение списка пациентов
std::vector<Patient> Service::getPatients() {
	return db.listPatients();
}



// Создание нового талона
CreateTicketResult Service::createTicket(const std::string& doctor_id, const std::string& date, const std::string& time) {
	auto idOpt = parseInt(doctor_id);
	if (!idOpt)
		return CreateTicketResult::DoctorNotFound;

	// Сначала дешевые проверки формата — они дают более понятное сообщение
	if (!validation.isValidDate(date))
		return CreateTicketResult::InvalidDate;

	if (!validation.isValidTime(time))
		return CreateTicketResult::InvalidTime;

	// Слот не должен быть в прошлом
	if (validation.isSlotInPast(date, time))
		return CreateTicketResult::TicketInPast;

	// Только теперь обращаемся к БД
	try {
		if (!db.findDoctorById(*idOpt))
			return CreateTicketResult::DoctorNotFound;
	}
	catch (const SQLite::Exception&) {
		return CreateTicketResult::DatabaseError;
	}

	try {
		Ticket t(*idOpt, date, time);
		db.addTicket(t);
		return CreateTicketResult::Ok;
	}
	catch (const SQLite::Exception& e) {
		// Нарушение уникальности "врач + дата + время"
		if (e.getErrorCode() == SQLITE_CONSTRAINT) // Нарушение ограничения
			return CreateTicketResult::Duplicate;
		return CreateTicketResult::DatabaseError;
	}
}



// Поиск свободных талонов по фильтрам (специальность, ФИО врача, дата)
FreeSlotsResponse Service::getFreeSlots(const std::string& speciality, const std::string& doctor_fio, const std::string& date) const {
	FreeSlotsResponse resp;

	// Если дата задана, проверяем ее формат
	if (!date.empty() && !validation.isValidDate(date)) {
		resp.status = FreeSlotsResult::InvalidDate;
		return resp;
	}

	try {
		auto doctors = db.listDoctors();

		for (const auto& doc : doctors) {
			// Сравнение точное, с учетом регистра: "терапевт" не найдет "Терапевт"
			
			// Фильтр по специальности
			if (!speciality.empty() && doc.getSpeciality() != speciality)
				continue;
			// Фильтр по ФИО врача
			if (!doctor_fio.empty() && doc.getFio() != doctor_fio)
				continue;

			// Свободные талоны этого врача
			auto slots = db.findFreeSlotsByDoctor(doc.getId());
			for (const auto& t : slots) {
				// Фильтр по дате
				if (!date.empty() && t.date != date)
					continue;
				// Слот уже прошел — не показываем
				if (validation.isSlotInPast(t.date, t.time))
					continue;
				resp.tickets.push_back(t);
			}
		}

		// После объединения талонов сортируем общий список по дате и времени
		std::sort(resp.tickets.begin(), resp.tickets.end(), [](const TicketView& a, const TicketView& b) {
			return std::tie(a.date, a.time) < std::tie(b.date, b.time);
		});

		resp.status = FreeSlotsResult::Ok;
	}
	catch (const SQLite::Exception&) {
		resp.status = FreeSlotsResult::DatabaseError;
	}

	return resp;
}


// Поиск свободных талонов конкретного врача на дату
FreeSlotsResponse Service::getFreeSlotsByDoctor(const std::string& doctor_id, const std::string& date) const {
	FreeSlotsResponse resp;

	auto idOpt = parseInt(doctor_id);
	if (!idOpt) {
		resp.status = FreeSlotsResult::DoctorNotFound;
		return resp;
	}

	// Проверка даты
	if (!validation.isValidDate(date)) {
		resp.status = FreeSlotsResult::InvalidDate;
		return resp;
	}

	try {
		// Проверка существования врача
		if (!db.findDoctorById(*idOpt)) {
			resp.status = FreeSlotsResult::DoctorNotFound;
			return resp;
		}

		// Читаем слоты и отсекаем прошедшие
		auto slots = db.findFreeSlots(*idOpt, date);
		for (const auto& t : slots) {
			if (validation.isSlotInPast(t.date, t.time))
				continue;
			resp.tickets.push_back(t);
		}

		resp.status = FreeSlotsResult::Ok;
	}
	catch (const SQLite::Exception&) {
		resp.status = FreeSlotsResult::DatabaseError;
	}

	return resp;
}



// Получение списка всех талонов
std::vector<TicketView> Service::getAllTickets() {
	return db.listAllTickets();
}



// Получение талонов пациента
std::vector<TicketView> Service::getPatientTickets(int patient_id) {
	return db.listPatientTickets(patient_id);
}



// Запись пациента на талон
BookResult Service::book(const std::string& ticket_id, int patient_id) {
	auto idOpt = parseInt(ticket_id);
	if (!idOpt)
		return BookResult::TicketNotFound;

	try {
		// Проверка существования талона
		auto ticket = db.findTicketById(*idOpt);
		if (!ticket)
			return BookResult::TicketNotFound;

		// Проверка, что талон свободен
		if (!ticket->isFree())
			return BookResult::TicketAlreadyBooked;

		// Нельзя записаться на прошедший талон
		if (validation.isSlotInPast(ticket->getDate(), ticket->getTime()))
			return BookResult::TicketInPast;

		// Проверка существования пациента
		if (!db.findPatientById(patient_id))
			return BookResult::PatientNotFound;

		// Проверка, что пациент еще не записан к этому врачу в этот день
		if (db.hasTicketWithDoctorOnDate(patient_id, ticket->getDoctorId(), ticket->getDate()))
			return BookResult::AlreadyBookedWithDoctorOnDate;

		// Талон заняли между проверкой и записью
		if (!db.bookSlot(*idOpt, patient_id))
			return BookResult::TicketAlreadyBooked;

		return BookResult::Ok;
	}
	catch (const SQLite::Exception&) {
		return BookResult::DatabaseError;
	}
}



// Отмена записи пациента
CancelResult Service::cancel(const std::string& ticket_id, int patient_id) {
	auto idOpt = parseInt(ticket_id);
	if (!idOpt)
		return CancelResult::TicketNotFound;

	try {
		// Проверка существования талона
		auto ticket = db.findTicketById(*idOpt);
		if (!ticket)
			return CancelResult::TicketNotFound;

		// Проверка, что талон занят
		if (ticket->isFree())
			return CancelResult::TicketAlreadyFree;

		// Проверка, что талон принадлежит именно этому пациенту
		if (ticket->getPatientId() != patient_id)
			return CancelResult::NotPatientTicket;

		if (!db.cancelBooking(*idOpt, patient_id))
			return CancelResult::DatabaseError;

		return CancelResult::Ok;
	}
	catch (const SQLite::Exception&) {
		return CancelResult::DatabaseError;
	}
}



// Получение истории обращений
std::vector<HistoryRecord> Service::getHistory() {
	return db.listHistory();
}
