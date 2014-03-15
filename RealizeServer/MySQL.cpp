#include "stdafx.h"
#include "MySQL.h"

NS_REALIZE_BEGIN

MySQL::MySQL()
{
}

MySQL::~MySQL()
{
}

bool MySQL::connect(char* host, char* user, char* password, char* db, int port)
{
	m_mysql_connection = mysql_init(NULL);

	if(mysql_real_connect(m_mysql_connection, host, user, password, db, port, NULL, NULL) != m_mysql_connection) {
		return false;
	}
	else {
		return true;
	}
}
bool MySQL::disconnect()
{
	mysql_close(m_mysql_connection);
	return true;
}

int MySQL::getErrno()
{
	return mysql_errno(m_mysql_connection);
}

const char* MySQL::getError()
{
	return mysql_error(m_mysql_connection);
}

bool MySQL::result(const char* query, vector<map<string, string>>& query_result)
{
	query_result.clear();

	if(mysql_query(m_mysql_connection, query)) {
		return false;
	}
	else {
		MYSQL_RES* mysql_res = mysql_store_result(m_mysql_connection);

		if(mysql_res == NULL) {
			return false;
		}

		MYSQL_ROW mysql_row;
		int mysql_num_field = mysql_num_fields(mysql_res);

		while((mysql_row = mysql_fetch_row(mysql_res)) != NULL) {
			map<string, string> mysql_row_result;

			for(int i=0; i<mysql_num_field; i++)
				//mysql_row_result[mysql_fetch_field_direct(res, i)] = mysql_row[i];

			query_result.push_back(mysql_row_result);
		}

		mysql_free_result(mysql_res);
	}

	return true;
}
bool MySQL::result(const char* query, map<string, string>& query_result)
{
	query_result.clear();

	if(mysql_query(m_mysql_connection, query)) {
		return false;
	}
	else {
		MYSQL_RES* mysql_res = mysql_store_result(m_mysql_connection);

		if(mysql_res == NULL) {
			return false;
		}

		MYSQL_ROW mysql_row;
		int mysql_num_field = mysql_num_fields(mysql_res);
		MYSQL_FIELD* mysql_field = mysql_fetch_fields(mysql_res);

		for(int i=0; i<mysql_num_field; i++) {
			OutputDebugString(mysql_field[i].name);
			OutputDebugString("\n");
		}

		if((mysql_row = mysql_fetch_row(mysql_res)) != NULL) {
			for(int i=0; i<mysql_num_field; i++)
				query_result[mysql_field[i].name] = mysql_row[i];
		}

		mysql_free_result(mysql_res);
	}

	return true;
}
bool MySQL::query(const char* query)
{
	if(mysql_query(m_mysql_connection, query)) {
		return false;
	}
	else {
		return true;
	}
}

list<user> MySQL::sql_result(const char * query)
{
	list<user> users;
	// Select 쿼리문
	bool stat=mysql_query(m_mysql_connection,query);
	if (stat != 0)
	{
		fprintf(stderr, "Mysql query error : %s", getError());
	}

	else{

		MYSQL_RES *sql_result;
		MYSQL_ROW sql_row;
		// 결과출력
		sql_result=mysql_store_result(m_mysql_connection);
		while((sql_row=mysql_fetch_row(sql_result))!=NULL)
		{
			user User = {sql_row[0], sql_row[1], sql_row[2], atoi(sql_row[3]), atoi(sql_row[4]), atoi(sql_row[5]), atoi(sql_row[6]) };
			users.push_back(User);
		}
		mysql_free_result(sql_result);
		return users;
	}
}

bool MySQL::update(const char* table, map<string, string>& map_value, const char* where_value)
{
	string query = "UPDATE `";
	query += table;
	query += "` SET ";

	for(auto it = map_value.begin(); it != map_value.end(); it++) {
		if(it != map_value.begin()) {
			query += ", ";
		}
		query += "`";
		query += it->first;
		query += "` = '";
		query += it->second;
		query += "'";
	}

	query += " WHERE ";
	query += where_value;

	if(mysql_query(m_mysql_connection, query.c_str())) {
		return false;
	}
	else {
		return true;
	}
}
bool MySQL::insert(const char* table, map<string, string>& map_value)
{
	string query = "INSERT into `";
	query += table;
	query += "` (";

	for(auto it = map_value.begin(); it != map_value.end(); it++) {
		if(it != map_value.begin()) {
			query += ", ";
		}
		query += "`";
		query += it->first;
		query += "`";
	}

	query += ") VALUES (";

	for(auto it = map_value.begin(); it != map_value.end(); it++) {
		if(it != map_value.begin()) {
			query += ", ";
		}
		query += "'";
		query += it->second;
		query += "'";
	}

	query += ")";

	if(mysql_query(m_mysql_connection, query.c_str())) {
		return false;
	}
	else {
		return true;
	}
}

NS_REALIZE_END