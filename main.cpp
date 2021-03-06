#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <mysql.h>
#include <sstream>
#include <string>
#include <ctime>
#include <algorithm>
#include <map>
#include <ctime>
using namespace std;

#define ip_address "192.168.0.102"
#define NO_ITEM "000"

MYSQL* connection;
MYSQL_ROW row;
MYSQL_RES* res;

SOCKET Connections[10];//�������� ���-��
int Counter = 0;

# define PRINTNUSERS if (Counter) printf("%d user online\n",Counter);else printf("No user online\n");



enum Request_Codes
{
    AUTHORIZATION = 11111,
    CHECK_BALANCE = 22222,
    ITEM_DETAILED_INFO = 33333,
    SELL_MENU = 44444,
    REGISTRATION = 55555,
    CHECK_ORDER_STATUS = 66666,
    USER_EXIT = 88888,
    ADD_ITEMS_TO_DB = 99999

};

enum User_Roles
{
    MANAGER = 1,
    CUSTOMER = 2,
    ADMIN = 3,
    NO_USER = 0
};


void connect_sql()
{
    connection=mysql_init(0);
    connection=mysql_real_connect(connection, ip_address, "s_vvardenfell", "s_vvardenfell",
                            "store_db", 0, NULL, 0);
    if(connection)
        cout<<"Connected to MySQL!"<<endl;
    else
        cout<<"Not connected;"<<" Error # "<<mysql_errno(connection)<<endl;
}

void error_message()
{
    cout<<"Error # "<<mysql_errno(connection);
    if(mysql_errno(connection)==1064)
            cout<<" (Syntax Error)"<<endl;
        else
    if(mysql_errno(connection)==1054)
            cout<<" (Insertion Fail (Unknown column in field list)"<<endl;
        else
    cout<<endl;

}

void show_item_detailed_info(int index)
{
    cout<<"Got show item detailed info"<<endl;

    //getting item code
    int msg_size;
    recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* item_code = new char[msg_size+1];
    item_code[msg_size] = '\0';
    recv(Connections[index], item_code, msg_size, NULL);

    cout<<"Got item code "<<item_code<<endl;

    stringstream ss;
    ss<<"SELECT name FROM goods WHERE id="<<item_code;

    string query=ss.str();
    ss.str(string(""));
    const char* q = query.c_str();

    if(connection)
    {
        int qstate = mysql_query(connection, q);

        if(!qstate)
        res = mysql_store_result(connection);

        int number_of_results = mysql_num_rows(res);
        cout<<"number_of_results: "<<number_of_results<<endl;

        if(number_of_results)
        {

            ss<<"SELECT orders_detailed.order_id, (SELECT name FROM goods WHERE id="<<item_code<<") AS 'name',"
            "(SELECT name FROM categories WHERE id =(SELECT category_id FROM goods WHERE goods.id="<<item_code<<")) AS 'category',"
            "(SELECT name FROM suppliers WHERE id =(SELECT supplier_id FROM goods WHERE goods.id="<<item_code<<")) AS 'supplier',"
            "amount AS 'amount_in_order', total_cost AS 'sum_on_warehouse', customer_name, customer_address, order_total_cost"
            " FROM orders_detailed INNER JOIN orders ON orders_detailed.order_id=orders.order_id WHERE orders_detailed.good_id="<<item_code<<";";

            query=ss.str();
            cout<<query<<endl;
            ss.str(string(""));
            q = query.c_str();

            qstate = mysql_query(connection, q);

            if(!qstate)
                res = mysql_store_result(connection);

            while(row = mysql_fetch_row(res))
            {
                cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<row[4]<<" "<<row[5]<<" "<<row[6]<<" "<<row[7]<<" "<<row[8]<<endl;
                ss<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<row[4]<<" "<<row[5]<<" "<<row[6]<<" "<<row[7]<<" "<<row[8]<<endl;
            }

            query=ss.str();
            msg_size=query.size();
            send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
            send(Connections[index], query.c_str(), msg_size, NULL);
        }
        else
        {
            query="no such item id";
            msg_size=query.size();
            send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
            send(Connections[index], query.c_str(), msg_size, NULL);
        }
    }

    delete[] item_code;
}

void check_order_status(int index)
{
        cout<<"Got check order status code"<<endl;

//                int order_code_from_client;
//                recv(Connections[index], reinterpret_cast<char*>(&order_code_from_client), sizeof(int), NULL);
        int msg_size;
        recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
        char* order_code_from_client = new char[msg_size+1]; ///���������-������� ��� SIZEOF
        order_code_from_client[msg_size] = '\0';
        recv(Connections[index], order_code_from_client, msg_size, NULL);

        cout<<"Got order number from client: "<<order_code_from_client<<endl;

        stringstream ss;
        ss<<"SELECT * FROM orders_detailed INNER JOIN orders ON orders_detailed.order_id=orders.order_id WHERE orders_detailed.order_id="<<order_code_from_client;

        string query=ss.str();
        const char* q = query.c_str();
        ss.str(string(""));

        if(connection)
        {
            int qstate = mysql_query(connection, q);

            if(!qstate)
                res = mysql_store_result(connection);

            int number_of_results = mysql_num_rows(res);
            cout<<"number_of_results: "<<number_of_results<<endl;

            if(number_of_results)
            {
                while(row = mysql_fetch_row(res))
                {
                    cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<row[4]<<" "<<row[5]<<" "<<row[6]<<" "<<row[7]/*<<" "<<row[8]<<" "<<row[9]<<" "<<row[10]*/<<endl;
                    ss<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<row[4]<<" "<<row[5]<<" "<<row[6]<<" "<<row[7]<<" "<<row[8]<<" "<<row[9]<<" "<<row[10]<<endl;
                }

                query = ss.str();
                msg_size = query.size();
                send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
                send(Connections[index], query.c_str(), msg_size, NULL);
            }
            else
            {
                query = "no such order";
                msg_size = query.size();
                send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
                send(Connections[index], query.c_str(), msg_size, NULL);
            }
        }
}

void check_balance(int index)
{
        cout<<"Got show table request"<<endl;
        int msg_size;
        stringstream ss;
        if(connection)
        {
            int qstate = mysql_query(connection, "SELECT id, name, amount, price FROM goods;");

            if(!qstate)
                res = mysql_store_result(connection);

            while(row = mysql_fetch_row(res))
            {
                cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<endl;
                ss<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<endl;
            }

            string query=ss.str();
            msg_size=query.size();
            cout<<query<<endl;
            send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
            send(Connections[index], query.c_str(), msg_size, NULL);
        }

}

void authorization(int index)
{
    //�������� � ������������ ����� � ������ �� �������
    int msg_size;
    recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* login_and_password_data = new char[msg_size+1];
    login_and_password_data[msg_size] = '\0';
    recv(Connections[index], login_and_password_data, msg_size, NULL);

    cout<<"Login and password raw data from client: "<<login_and_password_data<<endl;
    string login, password, login_and_password;
    login_and_password=login_and_password_data;

    login = login_and_password.substr(0, login_and_password.find('*'));
    login_and_password.erase(0, login_and_password.find('*')+1);
    password=login_and_password;
    cout<<"login: "<<login<<" password: "<<password<<endl;

    stringstream ss;
    ss<<"SELECT password, role_id FROM user_list WHERE name="<<login;
    string query = ss.str();
    int qstate = mysql_query(connection, query.c_str());

    if(!qstate)
        res = mysql_store_result(connection);

        //���� ������ � ����� ������� ����, ���������� ������ �� �� � �� �������
    if(row = mysql_fetch_row(res))
    {
        cout<<"User found, password and role: ";
        cout<<row[0]<<" "<<row[1]<<endl;
        if(password==row[0])//���� ������ ������, ����������� ���� ��� ��������� ����
        {
            msg_size=strlen(row[1]);
            send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
            send(Connections[index], row[1], msg_size, NULL);
        }
        else//���� ������ ��������, ���������� 0
        {
            cout<<"password doesn't match"<<endl;
            const char* login = "0";
            msg_size=strlen(login);///��������������, ��� ����� ���������� ������; ������� ��������� � �������
            send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
            send(Connections[index], login, msg_size, NULL);
        }

    }
    else//���� ������ � ����� ������� ���, ���������� '0'/NO_USER
    {
        ///OLD VERSION WORKS
        const char* role = "0"; //NO_USER
        msg_size=strlen(role);///��������������, ��� ����� ���������� ������; ������� ��������� � �������
        send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
        send(Connections[index], role, msg_size, NULL);
        cout<<"No such user"<<endl;

//                    ///NEW VERSION TESTING
//                    cout<<"qstate "<<qstate<<endl;
//                    int role = NO_USER;
//                    send(Connections[index], reinterpret_cast<char*>(&role), sizeof(int), NULL);
//                    cout<<"No such user"<<endl;

    }

    delete[] login_and_password_data;
}

void registration(int index)
{
    cout<<"Registration menu"<<endl;

    //getting login and password from client
    int msg_size;
    recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* login_and_password_raw_data = new char[msg_size+1];
    login_and_password_raw_data[msg_size] = '\0';
    recv(Connections[index], login_and_password_raw_data, msg_size, NULL);

    cout<<"login_and_password_raw_data: "<<login_and_password_raw_data<<endl;

    string login, password,
            login_and_password = login_and_password_raw_data;

    delete[] login_and_password_raw_data;

    login = login_and_password.substr(0, login_and_password.find('*'));
    login_and_password.erase(0, login_and_password.find('*')+1);
    password=login_and_password;

    cout<<"login: "<<login<<" & password: "<<password<<endl;

    stringstream ss;

    ss<<"SELECT id FROM user_list WHERE name=\'"<<login<<"\';";
    string query=ss.str();
    const char* q = query.c_str();
    ss.str(string(""));

    if(connection)
    {
        int qstate = mysql_query(connection, q);
        if(!qstate)
            res = mysql_store_result(connection);
            int number_of_results = mysql_num_rows(res);

        if(!number_of_results)
        {
            cout<<"id's with this name(login) from user_list: "<<number_of_results<<" user doesn't exists!"<<endl;

            ss<<"INSERT INTO user_list (name, password, role_id) VALUES (\'";
            ss<<login<<"\',\'"<<password<<"\',"<<"2);"; //�������� ���� ��������� �� ������� �����
            query = ss.str();
            cout<<query<<endl;
            q = query.c_str();

            qstate = mysql_query(connection,q);

            if(!qstate)
            {
                cout<<"New user created"<<endl;
                cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;

                string registration_confirm = to_string(CUSTOMER); //set 2, can't create admin or manager with client application
                msg_size=registration_confirm.size();
                send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
                send(Connections[index], registration_confirm.c_str(), msg_size, NULL);
            }
            else
            {
                error_message();
            }

        }
        else
        {
            cout<<"id's with this name(login) from user_list: "<<number_of_results<<" user already exists!"<<endl;

            string registration_confirm = to_string(NO_USER); //set 2, can't create admin or manager with client application
            msg_size=registration_confirm.size();
            send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
            send(Connections[index], registration_confirm.c_str(), msg_size, NULL);
            error_message();
        }

    }
}

void sell_menu(int index)
{
    cout<<"Sell menu"<<endl;
    int order_is_done=0;
    int msg_size;

    while(1)
    {
        recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
        char* item_code = new char[msg_size+1];
        item_code[msg_size] = '\0';
        recv(Connections[index], item_code, msg_size, NULL);

        cout<<"Got item code "<<item_code<<endl;

        stringstream ss;
        ss<<"SELECT name, amount, price FROM goods WHERE id="<<item_code;
        string query=ss.str();
        ss.str(string(""));
        const char* q = query.c_str();

        delete[] item_code;

        if(connection)
        {
            //��������� ������ � ������ �� �������� �������
            int qstate = mysql_query(connection, q);

            if(!qstate)
                res = mysql_store_result(connection);
                int number_of_results = mysql_num_rows(res);
                cout<<"number_of_results: "<<number_of_results<<endl;

            if(!number_of_results)//if there is no such item id - break/continue
            {
                query = NO_ITEM;
                msg_size=query.size();
                cout<<"!number_of_results: "<<number_of_results<<endl;
                send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
                send(Connections[index], query.c_str(), msg_size, NULL);

                continue;
            }


            while(row = mysql_fetch_row(res))
            {
                cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<endl;
                ss<<row[0]<<"*"<<row[1]<<"*"<<row[2]<<endl;
            }

                string str_query="";
                str_query=ss.str();
                cout<<"message to client: "<<str_query<<endl;
                ss.str(string(""));
                msg_size=str_query.size();
                send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
                send(Connections[index], str_query.c_str(), msg_size, NULL);

                recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
                char* order_continue = new char[msg_size+1];
                order_continue[msg_size] = '\0';
                recv(Connections[index], order_continue, msg_size, NULL);

                cout<<"Got continue code: "<<order_continue<<endl;
                order_is_done=atoi(order_continue);

                delete[] order_continue;
        }

        if(order_is_done==0)
        {
            cout<<"Escape input sequence on server"<<endl;
            break;
        }

    }

        //�������� �������������� ����� � ���� ������
        recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
        char* complete_order = new char[msg_size+1];
        complete_order[msg_size] = '\0';
        recv(Connections[index], complete_order, msg_size, NULL);

        cout<<"complete_order: "<<complete_order<<endl;

        string temp_str = complete_order;

        delete[] complete_order;

        unsigned int cnt = (count(temp_str.begin(),temp_str.end(), '*'));
        cout<<"Count of elements (*) in order: "<<cnt<<endl;

        //��������� ������ �� ������ � multimap: first=���/id, second=����������
        multimap<int, double> order_elements;

        for(unsigned int i=0; i<cnt; ++i)
        {
            order_elements.emplace(
            atoi(((temp_str.substr(0, temp_str.find('_'))).substr(0,temp_str.find('*'))).c_str()),
            atof(((temp_str.substr(0, temp_str.find('_'))).substr(temp_str.find('*')+1,temp_str.find('_'))).c_str())
                                                );
            temp_str.erase(0, temp_str.find('_')+1);
        }

        multimap<int, double>::iterator it;

        for(it=order_elements.begin(); it!=order_elements.end();++it)
        {
            cout<<(*it).first<<" and "<<(*it).second<<endl;
        }
        it=order_elements.begin();//����� ������� ������ �� ����������� ������� ������;
                                            //������� ���� ����� ����� ����������� multimap ����;
        string temp_order;

        if(connection)
        {
            string id;//��� ������ � ������� �� id

            //������ ������ "temp_order date time" � ���� name ������� orders ��� ������ � �������
            //�������, ������� �������� ����� � ������� ��� �������� ������ ������
            stringstream ss;
            ss<<"INSERT INTO orders (customer_name) VALUES (\'";

            time_t now = time(0);
            char* dt = ctime(&now);

            temp_order="data_temp_ ";
            temp_order+=dt; //���������� ����-����� ��� ������������
            ss<<temp_order<<"\');";

            string query = ss.str();
            ss.str(string(""));

            cout<<query<<endl;

            const char* q = query.c_str();

            int qstate = mysql_query(connection,q);

            if(!qstate)
            {
                cout<<"Record inserted"<<endl;
                cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
            }
            else
            {
                error_message();
            }

            //� ����� �������� �� multimap � ������� � ������� �������� ��� ������� ���������� id
            for(it=order_elements.begin(); it!=order_elements.end();++it)
            {
                ss<<"INSERT INTO orders_detailed (order_id, good_id, amount, total_cost) "
                "VALUES (";
                ss<<"(SELECT order_id FROM orders WHERE customer_name=\'"<<temp_order<<"\'), ";
                ss<<((*it).first)<<", "<<((*it).second)<<", (";
                ss<<"(SELECT price FROM goods WHERE id="<<((*it).first)<<")*"<<((*it).second)<<")";
                ss<<");";

                query = ss.str();
                ss.str(string(""));
                cout<<query<<endl;

                q = query.c_str();

                qstate = mysql_query(connection,q);

                if(!qstate)
                {
                    cout<<"Record inserted"<<endl;
                    cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                }
                else
                {
                    error_message();
                }

            }
                            //������� ��������� ������ �� orders_detailed � ������� � orders

            ss<<"SELECT SUM(total_cost) FROM orders_detailed GROUP BY order_id HAVING order_id=";
            ss<<"(SELECT order_id FROM orders WHERE customer_name=\'"<<temp_order<<"\')";

            query = ss.str();
            ss.str(string(""));
            q = query.c_str();

            qstate = mysql_query(connection,q);

            string total_cost_str;//������ ���������� �������� - ����� �� ������
            cout<<"Got total cost in orders_detailed : ";

            if(!qstate)
                res = mysql_store_result(connection);

            while(row = mysql_fetch_row(res))
            {
                total_cost_str=row[0];
                cout<<row[0]<<endl;
            }

                //������ ������ ��������� ������ � ������� total_cost � orders

            ss<<"UPDATE orders SET order_total_cost=";
            ss<<total_cost_str;
            ss<<" WHERE customer_name=\'"<<temp_order<<"\';";
            query = ss.str();
            ss.str(string(""));
            q = query.c_str();

            qstate = mysql_query(connection,q);

            if(!qstate)
            {
                cout<<"Records updated"<<endl;
                cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
            }
            else
            {
                error_message();
            }

            //���������� ����� ������, � ������� ������� �������������
            //� ���� ���� ������
            msg_size=total_cost_str.size();
            send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
            send(Connections[index], total_cost_str.c_str(), msg_size, NULL);

            //���� ������ ����� �� ������� ��� �������� ����� - ������� temp �� �������
            //���� ��������� ����� - ��������� ��� � �����
            //�������� ���*����� ��� @denied@ ��� �������� ��������� ������

            recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
            char* confirm_or_denied = new char[msg_size + 1];
            confirm_or_denied[msg_size] = '\0';
            recv(Connections[index], confirm_or_denied, msg_size, NULL);
            cout<<"Got name*address or @denied@ from client: "<<confirm_or_denied<<endl;

            if(string(confirm_or_denied)=="@denied@")//������ ������ �� order � order_detailed ��� ������� temp_order
            {
                ss<<"DELETE FROM orders WHERE customer_name=\'"<<temp_order<<"\';";
                query = ss.str();
                ss.str(string(""));
                cout<<query<<endl;
                q = query.c_str();
                qstate = mysql_query(connection,q);

                if(!qstate)
                {
                    cout<<"Records with temp_order "<<temp_order<< "removed"<<endl;
                    cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                }
                else
                    error_message();

                return;//� ������ ������ ��������� ������ �����
            }
            else //������ ��� � ����� � ������ � orders, ����� ������ � ��������� ������� ��� ������������ ����
            {
                //�������� id ������ ��� ������������ �������������, ����� �� name ����� �� �����

                ss<<"SELECT order_id FROM orders WHERE customer_name=\'"<<temp_order<<"\'";
                query = ss.str();
                ss.str(string(""));
                q = query.c_str();
                qstate = mysql_query(connection,q);

                if(!qstate)
                    res = mysql_store_result(connection);

                while(row = mysql_fetch_row(res))
                {
                    id=row[0];
                    cout<<"id �������� ������: "<<row[0]<<endl;
                }

                //����� temp_name � ������ ���� ����� �� ������ �������
                string name_and_address = confirm_or_denied;//�������� ���*����� � string ��� ���������
                delete[] confirm_or_denied;

                ss<<"UPDATE orders SET customer_name=\'"<<(name_and_address.substr(0, name_and_address.find('*')))<<"\'";
                name_and_address.erase(0, name_and_address.find('*')+1);
                ss<<", customer_address=\'"<<name_and_address<<"\'";
                ss<<" WHERE order_id=(SELECT order_id FROM orders WHERE customer_name=\'"<<temp_order<<"\');";
                query = ss.str();
                ss.str(string(""));
                cout<<query<<endl;
                q = query.c_str();//��� ��� ����� ������������ ���� � ��� �� q � ������ ������ ��� � query

                qstate = mysql_query(connection,q);

                if(!qstate)
                {
                    cout<<"Records updated"<<endl;
                    cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                }
                else
                    error_message();

            }

            //��������� ������� ������ � �������������� ������ ������� ��� ������
            cout<<"Sending full order data from sever to client...."<<endl;

            //�������� ���������� ��� ��������

            //�� ������� orders

            ss<<"SELECT * FROM orders WHERE order_id=\'"<<id<<"\';";

            string order_full_data_to_client;
            query = ss.str();
            ss.str(string(""));
            q = query.c_str();

            qstate = mysql_query(connection,q);

            cout<<"Customer order data (orders): ";
            if(!qstate)
                res = mysql_store_result(connection);

            while(row = mysql_fetch_row(res))
            {
                order_full_data_to_client.append(row[0]).append("_").append(row[1]).append("_").append(row[2]).append("_").append(row[3]).append("_");
                cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<endl;
            }

            //�� ������� orders_detailed

            order_full_data_to_client+='#';//�������� ����������� ��� ����������� ���������

            ss<<"SELECT name, orders_detailed.amount, total_cost FROM orders_detailed INNER JOIN goods ON orders_detailed.good_id=goods.id WHERE order_id=\'"<<id<<"\';";

            query = ss.str();
            ss.str(string(""));
            q = query.c_str();

            qstate = mysql_query(connection,q);

            cout<<"Customer full order (orders_detailed): ";
            if(!qstate)
                res = mysql_store_result(connection);

            while(row = mysql_fetch_row(res))
            {
                cout<<row[0]<<"*"<<row[1]<<"="<<row[2]<<endl;
                order_full_data_to_client.append(row[0]).append("*").append(row[1]).append("=").append(row[2]).append(",");

            }

            cout<<order_full_data_to_client<<endl;

            msg_size=order_full_data_to_client.size();
            send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
            send(Connections[index], order_full_data_to_client.c_str(), msg_size, NULL);

            //�������� ����� � ��������, �.�. ����� ����������� � ���������

            ss<<"SELECT good_id FROM orders_detailed WHERE order_id="<<id<<";";
            query = ss.str();
            ss.str(string(""));
            q = query.c_str();

            int counter=0;

            qstate = mysql_query(connection,q);

            if(!qstate)
                res = mysql_store_result(connection);

            while(row = mysql_fetch_row(res))
            {
                cout<<row[0]<<endl;

                ss<<"UPDATE goods SET amount = (goods.amount-(SELECT orders_detailed.amount FROM orders_detailed WHERE order_id = "<<id<<" AND good_id = "<<row[0];
                ss<<")) WHERE goods.id = "<<row[0]<<";";
                query = ss.str();
                ss.str(string(""));
                cout<<query<<endl;

                q = query.c_str();
                qstate = mysql_query(connection,q);

                if(!qstate)
                {
                    cout<<"goods.amount had been changed"<<endl;
                    cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                }
                else
                {
                    cout<<"goods.amount had NOT been changed"<<endl;
                    error_message();
                }


            }

        }
}

void add_items_to_db_from_file(int index)
{
    cout<<"Adding items from txt/xml menu"<<endl;

    //getting items for recording
    int msg_size;
    recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* items_to_db = new char[msg_size+1];
    items_to_db[msg_size] = '\0';
    recv(Connections[index], items_to_db, msg_size, NULL);

    cout<<"Got message "<<items_to_db<<endl;

    stringstream ss;
    ss<<items_to_db;
    string buffer;

    vector<string> items_from_client;


    while(getline(ss, buffer))
    {
        items_from_client.push_back(buffer);
//        cout<<"buffer: "<<buffer<<endl;
    }

    for(auto &a : items_from_client)
        cout<<"items_from_client"<<a<<endl;

    ss.str(string(""));

    stringstream ss2;

    ss2<<"INSERT INTO goods (name, category_id, supplier_id, amount, price) VALUES ";

    //WORK CORRECT BUT LOOKS STUPID
    for(unsigned int i=0; i<items_from_client.size(); ++i)
    {
        ss2<<"(";

        ss2<<"\'"<<items_from_client[i].substr(0,items_from_client[i].find(","))<<"\'"<<",";
        items_from_client[i].erase(0, items_from_client[i].find(",")+1);

        ss2<<"\'"<<items_from_client[i].substr(0,items_from_client[i].find(","))<<"\'"<<",";
        items_from_client[i].erase(0, items_from_client[i].find(",")+1);

        ss2<<"\'"<<items_from_client[i].substr(0,items_from_client[i].find(","))<<"\'"<<",";
        items_from_client[i].erase(0, items_from_client[i].find(",")+1);

        ss2<<"\'"<<items_from_client[i].substr(0,items_from_client[i].find(","))<<"\'"<<",";
        items_from_client[i].erase(0, items_from_client[i].find(",")+1);

        ss2<<"\'"<<items_from_client[i].substr(0,items_from_client[i].find(","))<<"\'";
        items_from_client[i].erase(0, items_from_client[i].find(",")+1);

        ss2<<(i<items_from_client.size()-1?"),":")");
    }

//NOT WORK CORRECT
//    for(unsigned int i=0; i<items_from_client.size(); ++i)
//    {
//        ss2<<"(";
//
//        while(items_from_client[i].size()>6)
//        {
//
//            ss2<<"\'"<<items_from_client[i].substr(0,items_from_client[i].find(","))<<"\'"<<",";
//                items_from_client[i].erase(0, items_from_client[i].find(",")+1);
//
//
//            ss2<<"\'"<<items_from_client[i].substr(0,items_from_client[i].find(","))<<"\'"<<",";
//            if(items_from_client[i].find(","))
//                items_from_client[i].erase(0, items_from_client[i].find(",")+1);
//            else items_from_client[i].erase(0, items_from_client[i].size());
//
//        }
//        ss2<<(i<items_from_client.size()-7?"),":")");
//    }

    ss2<<";";

    string query=ss2.str();
    cout<<query<<endl;

    const char* q = query.c_str();

    int qstate = mysql_query(connection,q);
    int affected_rows;

    string number_of_new_records;

    if(!qstate)
    {

        affected_rows=mysql_affected_rows(connection);
        cout<<"Record inserted"<<endl;
        cout<<"Affected rows: "<<affected_rows<<endl;

        number_of_new_records = to_string(affected_rows);
        msg_size = number_of_new_records.size();
        send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
        send(Connections[index], number_of_new_records.c_str(), msg_size, NULL);
    }
    else
    {
        error_message();
        number_of_new_records = "0";
        msg_size=number_of_new_records.size();
        send(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
        send(Connections[index], number_of_new_records.c_str(), msg_size, NULL);
    }
}

void ClientHandler(int index)//�-�, ���������-� ������ ����-� � �����-�������
{
    int msg_size, request_code;

    while (true)//��������� ����, � ��� ������ � ���� �����-� ��������
    {

    recv(Connections[index], reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* msg = new char[msg_size+1];
    msg[msg_size] = '\0';
    recv(Connections[index], msg, msg_size, NULL);
    request_code=atoi(msg);

    delete[] msg;

    cout<<"Request code from client: "<<request_code<<endl;

        if(connection)
        {
            if(request_code==AUTHORIZATION)
            {
                authorization(index);
            }
            else if(request_code==CHECK_BALANCE)
            {
                check_balance(index);
            }
            else if(request_code==CHECK_ORDER_STATUS)
            {
                check_order_status(index);
            }
            else if(request_code==ITEM_DETAILED_INFO)
            {
                show_item_detailed_info(index);
            }
            else if(request_code==REGISTRATION)
            {
                registration(index);
            }
            else if(request_code==SELL_MENU)
            {
                sell_menu(index);
            }
            else if(request_code==ADD_ITEMS_TO_DB)
            {
                add_items_to_db_from_file(index);
            }
            else if(request_code==USER_EXIT)
            {
                cout<<"=====got user exit code====="<<endl;

                break;
            }
            else
            {
                break;
            }

        }

    }

    --Counter;
    printf("-disconnect\n"); PRINTNUSERS
    closesocket(Connections[Counter+1]);


    return;

}


int main(int argc, char* argv[])
{
    setlocale (LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    connect_sql();

    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2,1);

    if (WSAStartup(DLLVersion, &wsaData) !=0)
    {
        cout << "Error" << endl;
        exit(1);
    }

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);

    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1111);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(bind(sListen, (SOCKADDR*)&addr, sizeof(addr)))
    {
        cout<<"Error bind, "<<WSAGetLastError()<<endl;
        closesocket(sListen);
        WSACleanup();
        return -1;
    }


    if(listen(sListen, SOMAXCONN))
    {
        cout<<"Error listen, "<<WSAGetLastError()<<endl;
        closesocket(sListen);
        WSACleanup();
        return -1;
    }

    SOCKET newConnection;
    sockaddr_in client_addr;
    int client_addr_size = sizeof(client_addr);

    while(newConnection = accept(sListen, (SOCKADDR*)&client_addr, &client_addr_size))
    {
        ++Counter;
        Connections[Counter] = newConnection;

        HOSTENT *hst;
        hst = gethostbyaddr((char*)&client_addr.sin_addr.s_addr,4,AF_INET);

        printf("+%s new connection!\n", (hst)?hst->h_name:"",inet_ntoa(client_addr.sin_addr));
        PRINTNUSERS

        DWORD thID;
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(Counter), NULL, &thID);
    }


//������ ������
//    for (int i = 0; i < 5; ++i)//�������� ���-�� �����������
//    {
//        newConnection = accept(sListen, (SOCKADDR*)&client_addr, &client_addr_size);
//
//        if (newConnection == 0)
//        {
//            cout << "Error #2" << endl;
//        }
//        else
//        {
//            cout << "Client connected!" << endl;
//
//            Connections[i] = newConnection;
//            ++Counter;
//
//            //������ ������ ��������� std::thread
//
//            //��������; ��� ������ ����������� ������ ???;
//            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(i), NULL, NULL);
//        }
//    }

    system("pause");

    mysql_free_result(res);
    mysql_close(connection);

    //closesocket(Connections);
//    WSACleanup();

    return 0;
}
