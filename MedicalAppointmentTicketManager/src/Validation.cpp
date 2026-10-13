#include "Validation.h"
#include <iomanip>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cctype>



// Проверка валидности ФИО
bool Validation::isValidFio(const std::string& fio) const {

    const size_t MAX_LEN = 50;

    size_t length = 0; // длина в символах
    bool hasLetter = false; // хотя бы одна буква

    for (size_t i = 0; i < fio.size(); ) {
        unsigned char b = static_cast<unsigned char>(fio[i]); // байт
        unsigned int cp; // код символа в Юникоде

        if (b < 0x80) { // Значение байта меньше 128 = английский символ, цифра, пробел или точка
            cp = b;
            i += 1;
        }
        else if (b >= 0xC2 && b <= 0xDF) { // Два байта - кириллица 
            if (i + 1 >= fio.size()) return false;
            unsigned char b2 = static_cast<unsigned char>(fio[i + 1]);
            if ((b2 & 0xC0) != 0x80) return false; // нужна вторая часть вида 10xxxxxx, иначе строка — битый UTF-8

            cp = ((b & 0x1F) << 6) | (b2 & 0x3F); // декодирование UTF-8 в код символа Юникода
            i += 2;
        }
        else { // три и четыре байта = русских букв, пробела и точки среди них тоже нет
            return false;
        }

        bool isLetter = (cp >= 0x0410 && cp <= 0x044F) // А-Я, а-я
            || cp == 0x0401 // Ё
            || cp == 0x0451; // ё

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

    return hasLetter; // Пустая строка и строка из одних пробелов и точек не проходят
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

    // Все позиции, кроме '-', должны быть цифрами
    for (size_t i = 0; i < date.size(); ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) {
            return false;
        }
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
    return date >= todayStr();
}



// Проверка валидности времени
bool Validation::isValidTime(const std::string& time) const {
    // Проверка структуры строки
    if (time.length() != 5 || time[2] != ':') {
        return false;
    }

    // Все позиции, кроме ':', должны быть цифрами
    for (size_t i = 0; i < time.size(); ++i) {
        if (i == 2) continue;
        if (!std::isdigit(static_cast<unsigned char>(time[i]))) {
            return false;
        }
    }

    // Разрешены только минуты 00 или 30
    if (time.compare(3, 2, "00") != 0 &&
        time.compare(3, 2, "30") != 0) {
        return false;
    }

    // Проверка интервала [09:00, 18:00)
    return (time >= "09:00" && time < "18:00");
}



// Проверка валидности кабинета
bool Validation::isValidOffice(int office) const {
    // Целое положительное число
    return office > 0;
}



// Проверка, что дата и время уже в прошлом
bool Validation::isSlotInPast(const std::string& date, const std::string& time) const {
    std::string today = todayStr();

    // Разные дни - сравнение по строке yyyy-mm-dd
    if (date < today) return true;
    if (date > today) return false;

    // Один день — сравниваем время
    return time <= nowTimeStr();
}



// Текущая дата в формате yyyy-mm-dd (локальное время)
std::string Validation::todayStr() const {
    auto now = std::chrono::system_clock::now();
    auto local = std::chrono::current_zone()->to_local(now);
    std::chrono::year_month_day today{ std::chrono::floor<std::chrono::days>(local) };

    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << static_cast<int>(today.year()) << '-'
        << std::setw(2) << std::setfill('0') << static_cast<unsigned>(today.month()) << '-'
        << std::setw(2) << std::setfill('0') << static_cast<unsigned>(today.day());
    return oss.str();
}



// Текущее время в формате hh:mm (локальное время)
std::string Validation::nowTimeStr() const {
    auto now = std::chrono::system_clock::now();
    auto local = std::chrono::current_zone()->to_local(now);

    auto timeOfDay = std::chrono::hh_mm_ss{
        local - std::chrono::floor<std::chrono::days>(local)
    };

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << timeOfDay.hours().count() << ':'
        << std::setw(2) << std::setfill('0') << timeOfDay.minutes().count();
    return oss.str();
}