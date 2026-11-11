#include "Service.h"
#include <sqlite3.h>
#include <chrono>
#include <ctime>
#include <tuple>
#include <algorithm>
#include <cctype>


namespace {

	// Приводит латинские и кириллические буквы к нижнему регистру для поиска
	std::string toLowerSearchText(const std::string& value) {
		std::string result;
		result.reserve(value.size());

		for (size_t i = 0; i < value.size();) {
			unsigned char first = static_cast<unsigned char>(value[i]);

			if (i + 1 < value.size() && first == 0xD0) {
				unsigned char second = static_cast<unsigned char>(value[i + 1]);
				if (second >= 0x90 && second <= 0x9F) {
					result.push_back(static_cast<char>(0xD0));
					result.push_back(static_cast<char>(second + 0x20));
				}
				else if (second >= 0xA0 && second <= 0xAF) {
					result.push_back(static_cast<char>(0xD1));
					result.push_back(static_cast<char>(second - 0x20));
				}
				else if (second == 0x81) { // Ё -> ё
					result.push_back(static_cast<char>(0xD1));
					result.push_back(static_cast<char>(0x91));
				}
				else {
					result.push_back(static_cast<char>(first));
					result.push_back(static_cast<char>(second));
				}
				i += 2;
			}
			else if (i + 1 < value.size() && first == 0xD1) {
				result.push_back(static_cast<char>(first));
				result.push_back(value[i + 1]);
				i += 2;
			}
			else {
				result.push_back(static_cast<char>(std::tolower(first)));
				++i;
			}
		}

		return result;
	}



	// Разбивает ФИО на слова, не учитывая точки после инициалов
	std::vector<std::string> splitFioWords(const std::string& fio) {
		std::vector<std::string> words;
		std::string current;
		const std::string lowerFio = toLowerSearchText(fio);

		for (char c : lowerFio) {
			if (c == '.' || std::isspace(static_cast<unsigned char>(c))) {
				if (!current.empty()) {
					words.push_back(current);
					current.clear();
				}
			}
			else {
				current += c;
			}
		}
		if (!current.empty()) words.push_back(current);
		return words;
	}



	// ФИО можно искать по фамилии, полному имени или инициалам без учета регистра
	bool matchesDoctorFio(const std::string& doctorFio, const std::string& searchFio) {
		const auto doctorWords = splitFioWords(doctorFio);
		const auto searchWords = splitFioWords(searchFio);
		if (searchWords.empty()) return true;

		size_t doctorIndex = 0;
		for (const auto& searchWord : searchWords) {
			bool found = false;
			while (doctorIndex < doctorWords.size()) {
				const auto& doctorWord = doctorWords[doctorIndex++];
				if (doctorWord.starts_with(searchWord)) {
					found = true;
					break;
				}
			}
			if (!found) return false;
		}

		return true;
	}

}



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
			// Фильтр по специальности: регистр букв не важен
			if (!speciality.empty() && toLowerSearchText(doc.getSpeciality()) != toLowerSearchText(speciality))
				continue;
			// ФИО можно указать полностью, инициалами или только фамилию
			if (!doctor_fio.empty() && !matchesDoctorFio(doc.getFio(), doctor_fio))
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


// Поиск свободных талонов конкретного врача на дату или на все будущие даты
FreeSlotsResponse Service::getFreeSlotsByDoctor(const std::string& doctor_id, const std::string& date) const {
	FreeSlotsResponse resp;

	auto idOpt = parseInt(doctor_id);
	if (!idOpt) {
		resp.status = FreeSlotsResult::DoctorNotFound;
		return resp;
	}

	// Дата необязательна: пустое значение означает поиск по всем будущим датам
	if (!date.empty() && !validation.isValidDate(date)) {
		resp.status = FreeSlotsResult::InvalidDate;
		return resp;
	}

	try {
		// Проверка существования врача
		if (!db.findDoctorById(*idOpt)) {
			resp.status = FreeSlotsResult::DoctorNotFound;
			return resp;
		}

		// Если дата не задана, показываем все будущие свободные талоны врача
		auto slots = date.empty() ? db.findFreeSlotsByDoctor(*idOpt) : db.findFreeSlots(*idOpt, date);
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
