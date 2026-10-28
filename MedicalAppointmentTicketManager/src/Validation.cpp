#include "Validation.h"
#include <iomanip>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cctype>



// Проверка валидности ФИО: "Фамилия Имя [Отчество]" с полными словами (>=2 букв), либо "Фамилия И. [О.]" с инициалами для имени/отчества
bool Validation::isValidFio(const std::string& fio) const {
    const size_t MAX_LEN = 50;

    std::vector<std::string> words; // уже разобранные слова
    std::string word; // текущее слово
    size_t length = 0;

    auto flushWord = [&]() {
        if (!word.empty()) words.push_back(word);
        word.clear();
        };

    for (size_t i = 0; i < fio.size(); ) {
        unsigned char b = static_cast<unsigned char>(fio[i]);
        size_t charBytes;

        if (b == ' ') { flushWord(); i += 1; ++length; continue; }

        if (b < 0x80) { // по ASCII: допустима только точка
            if (b != '.') return false;
            word += fio[i];
            charBytes = 1;
        }
        else if (b >= 0xC2 && b <= 0xDF) { // кириллица, 2 байта
            if (i + 1 >= fio.size() || (static_cast<unsigned char>(fio[i + 1]) & 0xC0) != 0x80)
                return false;
            word += fio.substr(i, 2);
            charBytes = 2;
        }
        else {
            return false;
        }

        if (++length > MAX_LEN) return false;
        i += charBytes;
    }
    flushWord();

    if (words.size() != 2 && words.size() != 3) return false;

    // Число кириллических символов в слове (без учета точки)
    auto letterCount = [](const std::string& w) {
        size_t n = 0;
        for (size_t i = 0; i < w.size(); ) {
            unsigned char b = static_cast<unsigned char>(w[i]);
            i += (b < 0x80) ? 1 : 2;
            if (b != '.') ++n;
        }
        return n;
        };

    // Фамилия: только полное слово (без точки), минимум 2 буквы
    if (words[0].find('.') != std::string::npos || letterCount(words[0]) < 2) return false;

    // Имя/отчество: либо полное слово (>=2 буквы, без точки), либо инициал (1 буква + точка в конце)
    for (size_t i = 1; i < words.size(); ++i) {
        const std::string& w = words[i];
        bool isInitial = w.size() >= 3 && w.back() == '.' && letterCount(w) == 1 &&
            w.find('.') == w.size() - 1;
        bool isFull = w.find('.') == std::string::npos && letterCount(w) >= 2;
        if (!isInitial && !isFull) return false;
    }

    return true;
}



// Приводит провалидированное ФИО к виду "Фамилия И." или "Фамилия И. О."
std::string Validation::formatFio(const std::string& fio) const {
    std::vector<std::string> words;
    std::string word;
    for (char c : fio) {
        if (c == ' ') { if (!word.empty()) { words.push_back(word); word.clear(); } }
        else word += c;
    }
    if (!word.empty()) words.push_back(word);

    std::string result = words[0]; // Фамилия без изменений
    for (size_t i = 1; i < words.size(); ++i) {
        // Первые 1-2 байта слова = первая буква
        unsigned char b = static_cast<unsigned char>(words[i][0]);
        std::string firstLetter = words[i].substr(0, (b < 0x80) ? 1 : 2);

        // Приведение к верхнему регистру кириллицы (а-я -> А-Я, ё -> Ё)
        if (firstLetter.size() == 2) {
            unsigned int cp = ((static_cast<unsigned char>(firstLetter[0]) & 0x1F) << 6)
                | (static_cast<unsigned char>(firstLetter[1]) & 0x3F);
            if (cp >= 0x0430 && cp <= 0x044F) cp -= 0x20;
            else if (cp == 0x0451) cp = 0x0401;
            firstLetter.clear();
            firstLetter += static_cast<char>(0xC0 | (cp >> 6));
            firstLetter += static_cast<char>(0x80 | (cp & 0x3F));
        }

        result += ' ';
        result += firstLetter;
        result += '.';
    }
    return result;
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