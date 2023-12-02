#include <iostream>
#include <Windows.h>
#include <pqxx/pqxx>
#include <string>
//#pragma execution_character_set("utf-8")

using namespace std;
using namespace pqxx;

enum fields { first_name, last_name, e_mail };

class Client_Manager
{
private:
	string sql;

public:
	connection* c;
	Client_Manager(string str)
	{
		this->sql = "";
		try
		{
			c = new connection(str);

			if (c->is_open()) {
				cout << "Opened database successfully: " << c->dbname() << endl;
			}
			else {
				cout << "Can't open database" << endl;
			}
		}
		catch (const exception& e)
		{
			cout << e.what() << endl;
		}
	}

	void SetConnection() {
		try
		{
			c = new connection(
				"host=localhost "
				"port=5432 "
				"dbname=client_manager "
				"user=user "
				"password=user");
			if (c->is_open()) {
				cout << "Opened database successfully: " << c->dbname() << endl;
			}
			else {
				cout << "Can't open database" << endl;
			}
		}
		catch (const exception& e)
		{
			cout << e.what() << endl;
		}
	}

	// Метод, создающий структуру БД(таблицы).
	void create_tables(void)
	{
		transaction tx{ *c };
		sql = "CREATE table if not EXISTS client (" \
			"id SERIAL  primary key, " \
			"firstname  VARCHAR(64) not null, " \
			"lastname   VARCHAR(64) not null, " \
			"email      VARCHAR(64) not null); ";
		tx.exec(sql);

		sql = "CREATE table if not EXISTS phone (" \
			"id SERIAL  primary key, " \
			"number     VARCHAR(16) not null); ";
		tx.exec(sql);

		sql = "CREATE table if not EXISTS clientphone (" \
			"client_id integer not null references client(id), " \
			"phone_id integer not null references Phone(id), " \
			"constraint client_phone primary key(client_id, phone_id)); ";
		tx.exec(sql);

		tx.commit();
	}

	// Метод, позволяющий добавить нового клиента.
	void new_client(string fname, string lname, string email)
	{
		try
		{
			int id = 0;
			transaction tx{ *c };

			auto collection = tx.query<int>("SELECT client.id FROM client WHERE firstname like '" + fname + "' and lastname like '" + lname + "' and email like '" + email + "'");
			for (auto& record : collection) id = get<0>(record);
			if (!id)
			{
				sql = "INSERT INTO public.client (id, firstname, lastname, email) VALUES(nextval('client_id_seq'::regclass), '" + fname + "', '" + lname + "', '" + email + "');";
				tx.exec(sql);
				tx.commit();
			}
		}
		catch (const exception& e)
		{
			cout << e.what() << endl;
		}
	}

	// Метод, позволяющий добавить телефон для существующего клиента.
	void phone_number(int client_id, string phone)
	{
		try
		{
			string str = "";
			int id = 0;
			transaction tx{ *c };
			auto collection = tx.query<string>("SELECT number FROM phone WHERE number = '" + phone + "'");
			for (auto& record : collection) str = get<0>(record);
			if (str != phone)
			{
				sql = "INSERT INTO public.phone (id, number) VALUES(nextval('phone_id_seq'::regclass), '" + phone + "') RETURNING id;";
				result r = tx.exec(sql);
				if (r.size() == 1)
				{
					id = r[0][0].as<int>();
					//std::cout << "Inserted ID: " << id << std::endl;
				}
				sql = "INSERT INTO public.clientphone VALUES('" + to_string(client_id) + "', '" + to_string(id) + "')";
				tx.exec(sql);
				tx.commit();
			}
		}
		catch (const exception& e)
		{
			cout << e.what() << endl;
		}
	}

	// Метод, позволяющий изменить данные о клиенте.
	void edit_client(int client_id, fields field, string data)  // first_name last_name e_mail
	{
		try
		{
			int id = 0;
			transaction tx{ *c };
			auto collection = tx.query<int>("SELECT client.id FROM client WHERE id = '" + to_string(client_id) + "'");
			for (auto& record : collection) id = get<0>(record);
			if (id)
			{
				sql = "UPDATE public.client SET ";
				switch (field)
				{
				case first_name:
					sql += "firstname='";
					break;
				case last_name:
					sql += "lastname='";
					break;
				case e_mail:
					sql += "email='";
					break;
				}
				sql += data + "' WHERE id='" + to_string(id) + "'";
				tx.exec(sql);
				tx.commit();
			}
		}
		catch (const exception& e)
		{
			cout << e.what() << endl;
		}
	}

	// Метод, позволяющий удалить телефон у существующего клиента.
	void phone_delete(string phone)
	{
		try
		{
			int id = 0;
			transaction tx{ *c };
			auto collection = tx.query<int>("SELECT phone.id FROM public.phone WHERE number = '" + phone + "'");
			for (auto& record : collection) id = get<0>(record);
			sql = "DELETE FROM public.clientphone  WHERE phone_id='" + to_string(id) + "'";
			tx.exec(sql);
			sql = "DELETE FROM public.phone WHERE id='" + to_string(id) + "'";
			tx.exec(sql);
			tx.commit();
		}
		catch (const exception& e)
		{
			cout << e.what() << endl;
		}
	}

	// Метод, позволяющий удалить существующего клиента.
	void delete_client(int client_id)
	{
		try
		{
			int id = 0;
			transaction tx{ *c };
			auto collection = tx.query<int>("SELECT clientphone.phone_id FROM public.clientphone WHERE client_id= '" + to_string(client_id) + "'");
			for (auto& record : collection) id = get<0>(record);
			if (id)
			{
				sql = "DELETE FROM public.clientphone  WHERE client_id='" + to_string(client_id) + "'";
				tx.exec(sql);
				sql = "DELETE FROM public.phone  WHERE id='" + to_string(id) + "'";
				tx.exec(sql);
			}
			sql = "DELETE FROM public.client  WHERE id='" + to_string(client_id) + "'";
			tx.exec(sql);
			tx.commit();
		}
		catch (const exception& e)
		{
			cout << e.what() << endl;
		}
	}

	// Метод, позволяющий найти клиента по его данным — имени, фамилии, email или телефону.
	int search_client(string fname, string lname, string email, string phone)
	{
		int id = -1;
		try
		{
			transaction tx{ *c };
			auto collection = tx.query<int>("SELECT client.id FROM public.client WHERE firstname like '" + fname + "' and lastname like '" + lname + "' and email like '" + email + "'");
			for (auto& record : collection) id = get<0>(record);
			if (id < 0)
			{
				auto collection = tx.query<int>("SELECT phone.id FROM public.phone WHERE number like '" + phone + "'");
				for (auto& record : collection) id = get<0>(record);
				if (id > 0)
				{
					auto collection = tx.query<int>("SELECT clientphone.client_id FROM public.clientphone WHERE phone_id= '" + to_string(id) + "'");
					for (auto& record : collection) id = get<0>(record);
				}
			}
		}
		catch (const exception& e)
		{
			cout << e.what() << endl;
		}
		return id;
	}
};

int main()
{
	//setlocale(LC_ALL, "Russian");
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setvbuf(stdout, nullptr, _IOFBF, 1000);

	//Client_Manager clm;
	//clm.SetConnection();

	Client_Manager clm(
		"host=localhost "
		"port=5432 "
		"dbname=client_manager "
		"user=user "
		"password=user");

	clm.create_tables();
	clm.new_client("Пётр", "Иванов", "p_ivanov@mail.ru");
	clm.phone_number(1, "+7(901)234-56-78");
	clm.phone_number(1, "+7(987)654-32-10");
	clm.new_client("Иоан", "Петросян", "i_petrosyan@gmail.com");
	clm.phone_number(2, "+7(910)010-10-10");
	clm.edit_client(2, first_name, "Иван");
	clm.edit_client(2, last_name, "Петров");
	int client_id_search = clm.search_client("Иоан", "Петров", "i_petrov@gmail.com", "+7(910)010-10-10");
	clm.edit_client(2, e_mail, "i_petrov@gmail.com");
	clm.phone_delete("+7(987)654-32-10");
	clm.delete_client(1);

	system("pause");
}

/*
Создайте программу для управления клиентами на C++.

Нужно хранить персональную информацию о клиентах:

имя,
фамилия,
email,
телефон.
Сложность в том, что телефон у клиента может быть не один, а два, три и даже больше. А может и не быть — например, если он не захотел его оставлять.

Вам нужно разработать структуру БД для хранения информации и написать класс на С++ для управления данными со следующими методами:

Метод, создающий структуру БД (таблицы).
Метод, позволяющий добавить нового клиента.
Метод, позволяющий добавить телефон для существующего клиента.
Метод, позволяющий изменить данные о клиенте.
Метод, позволяющий удалить телефон у существующего клиента.
Метод, позволяющий удалить существующего клиента.
Метод, позволяющий найти клиента по его данным — имени, фамилии, email или телефону.
Эти методы обязательны, но это не значит, что должны быть только они. Можно создавать дополнительные методы и классы.
*/