#include "Validation.h"
#include <chrono>
#include <sstream>
#include <algorithm>



// Проверка валидности ФИО
bool Validation::isValidFio(const std::string& fio) const {

    const size_t MAX_LEN = 50;

    size_t length = 0;        // длина в символах
    bool hasLetter = false;   // хотя бы одна буква

    for (size_t i = 0; i < fio.size(); ) {
        unsigned char b = static_cast<unsigned char>(fio[i]);
        unsigned int cp;      // код символа в Юникоде

        if (b < 0x80) { 
            // Значение байта меньше 128 = английский символ, цифра, пробел или точка
            cp = b;
            i += 1;
        }
        else if (b >= 0xC2 && b <= 0xDF) {
            // Два байта: нужна вторая часть вида 10xxxxxx
            if (i + 1 >= fio.size()) return false;
            unsigned char b2 = static_cast<unsigned char>(fio[i + 1]);
            if ((b2 & 0xC0) != 0x80) return false;

            cp = ((b & 0x1F) << 6) | (b2 & 0x3F);
            i += 2;
        }
        else {
            // три и четыре байта = русских букв, пробела и точки среди них нет
            return false;
        }

        bool isLetter = (cp >= 0x0410 && cp <= 0x044F)   // А-Я, а-я
            || cp == 0x0401                      // Ё
            || cp == 0x0451;                     // ё

        if (isLetter) {
            hasLetter = true;
        }
        else if (cp != ' ' && cp != '.') {
            return false;
        }

        if (++length > MAX_LEN) {
            return false;
        }
    }

    return hasLetter;         // Пустая строка и строка из одних пробелов и точек не проходят
}



// Проверка валидности номера телефона
bool Validation::isValidPhone(const std::string& phone) const {
	// Проверка структуры строки
	if (phone.length() != 12 || !phone.starts_with("+7")) {
		return false;
	}

	// Проверка, что все оставшиеся символы с индекса 2 до конца цифры
	return std::all_of(phone.begin() + 2, phone.end(), [](unsigned char c) {return std::isdigit(c);});
}



// Проверка валидности даты
bool Validation::isValidDate(const std::string& date) const {
	// Проверка структуры строки
	if (date.length() != 10 || date[4] != '-' || date[7] != '-') {
		return false;
	}

	// Извлечение чисел
	int year, month, day;
	char dash1, dash2;
	std::stringstream ss(date);

	if (!(ss >> year >> dash1 >> month >> dash2 >> day)) {
		return false;
	}

	// Проверка существования даты в календаре включая високосные годы
	std::chrono::year_month_day parsed_date{ std::chrono::year{year} / month / day };
	if (!parsed_date.ok()) {
		return false;
	}

	// Проверка "не раньше сегодняшней" даты
	auto now = std::chrono::system_clock::now();
	std::chrono::year_month_day today{ std::chrono::floor<std::chrono::days>(now) };

	return parsed_date >= today;
}



// Проверка валидности времени
bool Validation::isValidTime(const std::string& time) const {
    // Проверка структуры строки
    if (time.length() != 5 || time[2] != ':') {
        return false;
    }

    // Извлечение чисел
    int hours, minutes;
    char colon;
    std::stringstream ss(time);

    if (!(ss >> hours >> colon >> minutes)) {
        return false;
    }

    // Проверка существования времени
    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
        return false;
    }

    // Проверка интервала [09:00, 18:00)
    return (time >= "09:00" && time < "18:00");
}



// Проверка валидности кабинета
bool Validation::isValidOffice(unsigned int office) const {
    // Целое положительное число
    return office > 0;
}
