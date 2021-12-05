#include <stdio.h>
#include <iostream>
#include <string>
#include <regex>
#include <sqlite3.h>
#include <fstream>
#include <chrono>
#include <unistd.h>
// #include <experimental/filesystem>

using namespace std;
const char* db_name = "phone_list.db";

void helpMenu()
{
    cout << "How to use this program---------------------------" << endl;
    cout << "ADD \"<Person>\" \"<Telephone#>\" " << endl;
    cout << "DEL \"<Person>\"" << endl; 
    cout << "DEL \"<Telephone#>\"" << endl; 
    cout << "LIST" << endl; 
}
void writeLog(string s)
{ // Writes to a log file 
    ofstream log_file;
    log_file.open("log_file.txt", fstream::app);
    if(log_file.is_open())
    { // Checking if file successfully opened
        // Adding the timestamp
        const auto time = chrono::system_clock::now();
        time_t time_today = chrono::system_clock::to_time_t(time);
        log_file << ctime(&time_today);

        // Adding the real uid to log
        uid_t uid = getuid();
        log_file << "UID: " << uid << endl;

        // Adding action taken
        log_file << s << endl;



        log_file << endl; // Making space between the logs

        // FIXME:
        // Changing file permissions
        // C++11 does not have #include <filesystem>
        // which is needed to change file permissions 
        // fs::permissions((const char*)"log_file.txt", fs::perms::owner_all);
        
        log_file.close();

    }
}

static int callback(void* not_used, int argc, char** argv, char** column_names)
{ // Returns (calls back) the data to be displayed
    for(int i = 0; i < argc; i++)
    { // Iterates through the columns of the table
        cout << column_names[i] << ": " << argv[i] << endl;
    }

    cout << endl;
    return 0;
}

static int createDB(const char* s)
{ // Creating the database
    sqlite3* db;
    int exit = 0;
    exit = sqlite3_open(s, &db);
    sqlite3_close(db);
    
    return 0;
}

static int createTable(const char* db_name)
{ // Creating the table
    sqlite3* db;
    char* err_msg;

    string sql = "CREATE TABLE IF NOT EXISTS PHONE_LIST("
                "NAME VAR(15) NOT NULL, "
                "PHONE_NUMBER VAR(17) NOT NULL UNIQUE);";


    int exit = 0;

    // Opening a new database connection
    exit = sqlite3_open(db_name, &db);
    exit = sqlite3_exec(db, sql.c_str(), NULL, 0, &err_msg);

    if(exit != SQLITE_OK)
    { // Checking if connection established successfully
        cerr << "ERROR IN CREATE TABLE FUNCTION." << endl;
        sqlite3_free(err_msg);
    }
    sqlite3_close(db);

    return 0;
}

static int selectData(const char* db_name)
{
    sqlite3* db;
    char* err_msg;

    // Retrieve all rows from the table
    string sql = "SELECT * FROM PHONE_LIST;";

    int exit = sqlite3_open(db_name, &db);
    exit = sqlite3_exec(db, sql.c_str(), callback, NULL, &err_msg);

    if(exit != SQLITE_OK)
    {
        cerr << "ERROR: selectData FUNCTION FAILED." << endl;
        sqlite3_free(err_msg);
    }
    
    sqlite3_close(db);
    writeLog("LISTED");

    return 0;

}

static int insertData(const char* db_name, char** argv)
{
    sqlite3* db;
    char* err_msg;
    sqlite3_stmt *stmt;
    string name = argv[2];
    string log_insert = "ADDED: " + name;
    string sql = "INSERT INTO PHONE_LIST (NAME,PHONE_NUMBER) VALUES (?,?);";

    int exit = sqlite3_open(db_name, &db);
    if(exit != SQLITE_OK)
    {
        cerr << "ERROR: Could not insert entry." << endl;
        sqlite3_free(err_msg);
    }

    exit = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if(exit != SQLITE_OK)
    {
        cerr << "ERROR: Could not bind value on insert." << endl;
        sqlite3_free(err_msg);
        sqlite3_close(db);
    }

    sqlite3_bind_text(stmt, 1, argv[2], -1, NULL);
    sqlite3_bind_text(stmt, 2, argv[3], -1, NULL);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    writeLog(log_insert);

    return 0;

}

static int deleteData(const char* db_name, char* argv[])
{
    sqlite3* db;
    sqlite3_stmt* stmt;
    char* err_msg;
    string sql;
    string log_data;
    string data_to_delete = argv[2];
    
    if(string::npos != data_to_delete.find_first_of("0123456789")) // Check if the input contains any numbers 
    {
        sql = "DELETE FROM PHONE_LIST WHERE NAME = ?;";
        log_data = "DELETED: " + data_to_delete;

    }
    else // Runs if no number found
    {
        sql = "DELETE FROM PHONE_LIST WHERE PHONE_NUMBER = ?;";
    }

    int exit = sqlite3_open(db_name, &db);
    if(exit != SQLITE_OK)
    {
        cerr << "ERROR: Could not delete data." << endl;
        sqlite3_free(err_msg);
        return 1;
    }

    exit = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if(exit != SQLITE_OK)
    {
        cerr << "ERROR: Could not bind value on DELETE." << endl;
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_text(stmt, 1, data_to_delete.c_str(), -1, NULL);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if(sqlite3_changes(db) == 0)
    { // If no changes were made to the database after error checking, input was not there
        return 1;
    }
    else
    {
        writeLog(data_to_delete);
    }

    sqlite3_close(db);
    return 0;
}

int main(int argc, char** argv)
{
    const char* db_name = "phone_list_DB.db";
    createDB(db_name);
    createTable(db_name);

    string name_regex_string("[A-Z][a-z]+\\s[A-Z][a-z]+|[A-Z][a-z]+,\\s[A-Z][a-z]+|[A-Z][a-z]+,\\s[A-Z][a-z]+\\s[A-Z][a-z]+|[A-Z]'[A-Z][a-z]+,\\s[A-Z][a-z]+\\s[A-Z]\\.|[A-Z][a-z]+\\s[A-Z]'[A-Z][a-z]+-[A-Z][a-z]+|[A-Z][a-z]+|[A-Z][a-z]+\\s[A-Z][a-z]+\\s[A-Z][a-z]+|[A-Z][a-z]+,\\s[A-Z][a-z]+\\s[A-Z]\\.");
    string phone_regex_string("[\\d]{5}|(\\+?[\\d])?\\(?[\\d]{1,3}\\)?[\\d]{1,3}[-\\s\\.][\\d]{4}|(\\+[\\d]{2}\\s)\\(?[\\d]{2}\\)?\\s[\\d]{3}[-\\.\\s][\\d]{4}|(\\+?[\\d]{3})\\s[\\d]{3}[\\s\\.-][\\d]{3}[\\s\\.-][\\d]{4}|[\\d]{5}[\\.\\s-][\\d]{5}|(\\+?[\\d]{3})\\s[\\d]\\s[\\d]{3}[\\s\\.-]?[\\d]{3}[\\s\\.-][\\d]{4}|(\\+?[\\d]{3})\\s[\\d]\\s\\([\\d]{3}\\)[\\s\\.-]?[\\d]{3}[\\s\\.-][\\d]{4}|(\\+?[\\d]\\s)?[\\d]{3}[\\.\\s-][\\d]{3}[\\.\\s-][\\d]{4}|(\\+?[\\d]\\s)?\\(?[\\d]{3}\\)?[\\s\\.-]?[\\d]{3}[\\s\\.-][\\d]{4}");
    regex name_regex(name_regex_string);
    regex phone_regex(phone_regex_string);

    if(argc == 4)
    { // If 4 arguments are entered: ADD Person Telephone #
        string action = argv[1]; // cli action parameter input into string
        string person_name = argv[2]; // cli 
        string phone_num = argv[3];
        if(action == "ADD" && regex_match(phone_num, phone_regex))
        { // Adding Person and Telephone # if regex is valid
            insertData(db_name, argv);
            return 0;
        }
        helpMenu();
        cerr << "ERROR 1: Improper input.\n";
        return 1;
    }
    else if(argc == 3)
    { // If 3 arguments are entered: Either DEL Person or DEL Telephone #
        string action = argv[1]; // cli action parameter input into string
        string input_to_delete = argv[2]; // Could be phone number or name
        if(action == "DEL" && (regex_match(input_to_delete, name_regex) || regex_match(input_to_delete, phone_regex)))
        { // if either name or number is valid and DEL was the action
            deleteData(db_name, argv);
            return 0;
        }
        helpMenu();
        cerr << "ERROR 2: Improper input.\n";
        return 1;
    }
    else if(argc == 2)
    { // If 2 arguments are entered: LIST
        string action = argv[1]; // cli action parameter input into string
        if(action == "LIST")
        { // Check parameter to verify LIST is the action
            selectData(db_name);
            return 0;
        }
        helpMenu();
        cerr << "ERROR 3: Improper input.\n";
        return 1;
    
    }
    
    cerr << "ERROR 4: Improper input.\n";
    return 1;
}
