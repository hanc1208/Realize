#pragma once
#include "C:\xampp\mysql\include\mysql.h"
#pragma comment(lib, "C:\\xampp\\mysql\\lib\\libmysql.lib")

#include "Macro.h"
#include "user.h"
#include <vector>
#include <map>

NS_REALIZE_BEGIN


class MySQL
{
private:
	MYSQL* m_mysql_connection;
public:
	bool isError;

	MySQL();
	~MySQL();

	bool connect(char* host, char* user, char* password, char* db, int port);
	bool disconnect();

	int getErrno();
	const char* getError();

	list<user> sql_result(const char *);
	bool result(const char* query, vector<map<string, string>>& query_result);
	bool result(const char* query, map<string, string>& query_result);
	bool query(const char* query);
	bool update(const char* table, map<string, string>& map_value, const char* where_value);
	bool insert(const char* table, map<string, string>& map_value);
};

NS_REALIZE_END