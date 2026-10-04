#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <Windows.h>

int main() {
	SetConsoleOutputCP(65001);
	try {
		SQLite::Database db("data/tickets.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
		std::cout << "БД успешно открыта/создана: " << db.getFilename() << std::endl;
	}
	catch (const SQLite::Exception& e)
	{
		std::cerr << "Ошибка SQLite: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}