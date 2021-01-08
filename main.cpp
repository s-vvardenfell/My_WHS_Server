#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <mysql.h>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
#include <map>
#include <ctime>
using namespace std;

#define ip_address "192.168.0.102"

MYSQL* connection;
MYSQL_ROW row;
MYSQL_RES* res;

SOCKET Connections[5];//изменить кол-во
int Counter = 0;

enum Request_Codes
{
    AUTHORIZATION = 11111,
    CHECK_BALANCE = 22222,
    ITEM_DETAILED_INFO = 33333,
    SELL_MENU = 44444,
    REGISTRATION = 55555
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

void show_table_goods(string& msg)
{
    stringstream ss,ss1;
    ss<<"SELECT * FROM goods;";
    string temp=ss.str();
    const char* q = temp.c_str();

    if(connection)
    {
        int qstate = mysql_query(connection, q);

        if(!qstate)
        {
            res = mysql_store_result(connection);

            while(row = mysql_fetch_row(res))
            {
                cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<row[4]<<endl;
                ss1<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<row[4]<<endl;
                msg=ss1.str();
            }
        }
        else
            error_message();
    }
}

void ClientHandler(int index)//ф-я, принимающ-я индекс соед-я в сокет-массиве
{
    int msg_size, request_code;
    //ДОБАВИТЬ УСЛОВИЕ - КЛИЕНТ ОТСОЕДИНИЛСЯ = ВЫХОД ИЗ ЦИКЛА
    while (true)//бесконечн цикл, в кот будут приним и отпр сообщ-я клиентов
    {

    recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
    char* msg = new char[msg_size+1];
    msg[msg_size] = '\0';
    recv(Connections[index], msg, msg_size, NULL);
    request_code=atoi(msg);

    cout<<"Request code from client: "<<request_code<<endl;

        if(connection)
        {
            if(request_code==AUTHORIZATION)
            {
                //получаем и обрабатываем логин и пароль от клиента
                recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
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
                string str = ss.str();
                int qstate = mysql_query(connection, str.c_str());

                if(!qstate)
                    res = mysql_store_result(connection);

                //если запись с таким логином есть, сравниваем пароль из бд и от клиента
                if(row = mysql_fetch_row(res))
                {
                    cout<<"User found, password and role: ";
                    cout<<row[0]<<" "<<row[1]<<endl;
                    if(password==row[0])//если пароль верный, отпаравляем роль для активации меню
                    {
                        msg_size=sizeof(row[0]);
                        send(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                        send(Connections[index], row[1], msg_size, NULL);
                    }
                    else//если пароль неверный, отправляем 0
                    {
                        cout<<"password doesn't match"<<endl;
                        const char* login = "0";
                        msg_size=sizeof(login);
                        send(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                        send(Connections[index], login, msg_size, NULL);
                    }

                }
                else//если записи с таким логином нет, отправляем 0
                {
                    cout<<"qstate "<<qstate<<endl;
                    const char* role = "0";
                    msg_size=sizeof(role);
                    send(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                    send(Connections[index], role, msg_size, NULL);
                    cout<<"No such user"<<endl;

                }

                delete[] login_and_password_data;
            }
            else if(request_code==CHECK_BALANCE)
            {
                cout<<"Got show table request"<<endl;
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

                    string str=ss.str();
                    msg_size=str.size();
                    cout<<"msg_size=sizeof(str) "<<msg_size<<endl;
                    cout<<str<<endl;
                    send(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                    send(Connections[index], str.c_str(), msg_size, NULL);
                }

            }
            else if(request_code==ITEM_DETAILED_INFO)
            {
                cout<<"Got show item detailed info"<<endl;

                recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                char* item_code = new char[msg_size+1];
                item_code[msg_size] = '\0';
                recv(Connections[index], item_code, msg_size, NULL);

                cout<<"Got item code "<<item_code<<endl;

                stringstream ss,ss1,ss2;
                ss<<"SELECT * FROM goods WHERE id="<<item_code;
                string temp_str=ss.str();
                const char* q = temp_str.c_str();

                if(connection)
                {
                    int qstate = mysql_query(connection, q);

                    if(!qstate)
                        res = mysql_store_result(connection);

                    while(row = mysql_fetch_row(res))
                    {
                        cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<row[4]<<" "<<row[5]<<endl;
                        ss1<<"SELECT id, name, (SELECT name FROM categories WHERE id="<<row[2]<<"), (SELECT name FROM suppliers WHERE id="<<row[3]<<"), amount, price FROM goods WHERE id="<<item_code<<endl;
                    }

                    string temp_str=ss1.str();
                    const char* q = temp_str.c_str();

                    qstate = mysql_query(connection, q);

                    if(!qstate)
                        res = mysql_store_result(connection);

                    while(row = mysql_fetch_row(res))
                    {
                        cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<row[4]<<" "<<row[5]<<endl;
                        ss2<<row[0]<<" "<<row[1]<<" "<<row[2]<<" "<<row[3]<<" "<<row[4]<<" "<<row[5]<<endl;
                    }

                    temp_str=ss2.str();
                    msg_size=temp_str.size();
                    send(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                    send(Connections[index], temp_str.c_str(), msg_size, NULL);
                }

                delete[] item_code;

            }
            else if(request_code==REGISTRATION)
            {
                cout<<"Registration menu"<<endl;

                recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                char* login_and_password_raw_data = new char[msg_size+1];
                login_and_password_raw_data[msg_size] = '\0';
                recv(Connections[index], login_and_password_raw_data, msg_size, NULL);

                cout<<login_and_password_raw_data<<endl;

                string login, password,
                        login_and_password = login_and_password_raw_data;

                login = login_and_password.substr(0, login_and_password.find('*'));
                login_and_password.erase(0, login_and_password.find('*')+1);
                password=login_and_password;

                cout<<login<<" "<<password<<endl;

                stringstream ss;
                ss<<"INSERT INTO user_list (name, password, role_id) VALUES (\'";
                ss<<login<<"\',\'"<<password<<"\',"<<"2);"; //добавить сюда подзапрос из таблицы ролей
                string query = ss.str();
                cout<<query<<endl;
                const char* q = query.c_str();

                int qstate = mysql_query(connection,q);

                if(!qstate)
                {
                    cout<<"New user created"<<endl;
                    cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                }
                else
                    error_message();

            }
            else if(request_code==SELL_MENU)
            {
                cout<<"Sell menu"<<endl;
                int order_is_done=0;

                while(1)
                {
                    recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                    char* item_code = new char[msg_size+1];
                    item_code[msg_size] = '\0';
                    recv(Connections[index], item_code, msg_size, NULL);

                    cout<<"Got item code "<<item_code<<endl;

                    stringstream ss,ss1,ss2;
                    ss<<"SELECT name, amount, price FROM goods WHERE id="<<item_code;
                    string temp_str=ss.str();
                    const char* q = temp_str.c_str();

                    delete[] item_code;

                    if(connection)//else отработать
                    {
                        //отправляю данные о товаре по артикулу клиенту
                        int qstate = mysql_query(connection, q);

                        if(!qstate)
                            res = mysql_store_result(connection);

                        while(row = mysql_fetch_row(res))
                        {
                            cout<<row[0]<<" "<<row[1]<<" "<<row[2]<<endl;
                            ss1<<row[0]<<"*"<<row[1]<<"*"<<row[2]<<endl;
                        }

                        temp_str=ss1.str();
                        msg_size=temp_str.size();
                        send(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                        send(Connections[index], temp_str.c_str(), msg_size, NULL);

                        recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
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
                    //получаем сформированный заказ в виде строки
                    recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                    char* complete_order = new char[msg_size+1];
                    complete_order[msg_size] = '\0';
                    recv(Connections[index], complete_order, msg_size, NULL);

                    cout<<"complete_order: "<<complete_order<<endl;

                    string temp_str = complete_order;

                    delete[] complete_order;

                    unsigned int cnt = (count(temp_str.begin(),temp_str.end(), '*'));
                    cout<<"Count of elements (*): "<<cnt<<endl;

                    //сохраняем данные из строки в multimap: first=код/id, second=количество
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
                    it=order_elements.begin();//иначе занемем данные из недоступной области памяти;
                                            //удалить если удалю вывод содержимого multimap выше;

                    string temp_order;

                    if(connection)
                    {

                        string id;//для работы с заказом по id

                        //создаю запись "temp_order date time" в поле name таблицы orders для работы с текущим
                        //заказом, избегая подсчета строк в таблице для создания нового заказа
                        stringstream ss, ss2;
                        ss<<"INSERT INTO orders (customer_name) VALUES (\'";

                        time_t now = time(0);
                        char* dt = ctime(&now);

                        temp_order="temp_order ";
                        temp_order+=dt; //прибавляем дату-время для уникальности
                        ss<<temp_order<<"\');";

                        string query = ss.str();

                        cout<<query<<endl;

                        const char* q = query.c_str();

                        int qstate = mysql_query(connection,q);

                            if(!qstate)
                            {
                                cout<<"Record inserted"<<endl;
                                cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                            }
                            else
                                error_message();

                        //в цикле проходим по multimap и заносим в таблицу значения для данного временного id
                        for(it=order_elements.begin(); it!=order_elements.end();++it)
                        {

                            stringstream ss3;
                            ss3<<"INSERT INTO orders_detailed (order_id, good_id, amount, total_cost) "
                            "VALUES (";
                            ss3<<"(SELECT order_id FROM orders WHERE customer_name=\'"<<temp_order<<"\'), ";
                            ss3<<((*it).first)<<", "<<((*it).second)<<", (";
                            ss3<<"(SELECT price FROM goods WHERE id="<<((*it).first)<<")*"<<((*it).second)<<")";
                            ss3<<");";

                            query = ss3.str();

                            const char* q2 = query.c_str();

                            qstate = mysql_query(connection,q2);

                                if(!qstate)
                                {
                                    cout<<"Record inserted"<<endl;
                                    cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                                }
                                else
                                    error_message();
                        }

                            //считаем стоимость заказа по orders_detailed и заносим в orders
                            stringstream ss4;

                            ss4<<"SELECT SUM(total_cost) FROM orders_detailed GROUP BY order_id HAVING order_id=";
                            ss4<<"(SELECT order_id FROM orders WHERE customer_name=\'"<<temp_order<<"\')";

                            query = ss4.str();

                            const char* q2 = query.c_str();

                            qstate = mysql_query(connection,q2);

                            string total_cost_str;//строка содержащая значение - сумму по заказу
                            cout<<"Got total cost in orders_detailed : ";
                            if(!qstate)
                                res = mysql_store_result(connection);

                                while(row = mysql_fetch_row(res))
                                {
                                    total_cost_str=row[0];
                                    cout<<row[0]<<endl;
                                }

                            //вносим полную стоимость заказа в столбец total_cost в orders
                            stringstream ss5;
                            ss5<<"UPDATE orders SET order_total_cost=";
                            ss5<<total_cost_str;
                            ss5<<" WHERE customer_name=\'"<<temp_order<<"\';";
                            query = ss5.str();

                            const char* q3 = query.c_str();

                            qstate = mysql_query(connection,q3);

                                if(!qstate)
                                {
                                    cout<<"Records inserted"<<endl;
                                    cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                                }
                                else
                                    error_message();

                            //отправляем сумму заказа, у клиента спросим подтверждение
                            //и ввод личн данных
                            msg_size=total_cost_str.size();
                            send(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                            send(Connections[index], total_cost_str.c_str(), msg_size, NULL);

                            //если пришел ответ от клиента что отменить заказ - удаляем temp из таблицы
                            //если оформляем заказ - принимаем имя и адрес
                            //получаем имя*адрес или @denied@ для удаления временной записи

                            recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                            char* confirm_or_denied = new char[msg_size + 1];
                            confirm_or_denied[msg_size] = '\0';
                            recv(Connections[index], confirm_or_denied, msg_size, NULL);
                            cout<<"Got name*address or @denied@ from client: "<<confirm_or_denied<<endl;

                            if(string(confirm_or_denied)=="@denied@")//удаляю записи из order и order_detailed для данного temp_order
                            {
                                stringstream ss6;
                                ss6<<"DELETE FROM orders WHERE customer_name=\'"<<temp_order<<"\';";
                                query = ss6.str();
                                cout<<query<<endl;
                                const char* q4 = query.c_str();

                                qstate = mysql_query(connection,q4);

                                if(!qstate)
                                {
                                    cout<<"Records with temp_order "<<temp_order<< "removed"<<endl;
                                    cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                                }
                                else
                                    error_message();

                                return;//в случае отмены завершаем работу здесь
                            }
                            else //заношу имя и адрес в стобцы в orders, делаю селект и отправляю клиенту для формирования чека
                            {
                                //сохраняю id заказа для последующего использования, когда по name будет не найти
                                stringstream ss6;
                                ss6<<"SELECT order_id FROM orders WHERE customer_name=\'"<<temp_order<<"\'";

                                query = ss6.str();
                                q = query.c_str();
                                qstate = mysql_query(connection,q);

                                if(!qstate)
                                res = mysql_store_result(connection);

                                while(row = mysql_fetch_row(res))
                                {
                                    id=row[0];
                                    cout<<"id текущего заказа: "<<row[0]<<endl;
                                }

                                //меняю temp_name и пустое поле адрес на данные клиента
                                string name_and_address = confirm_or_denied;//сохраняю имя*адрес в string для обработки
                                delete[] confirm_or_denied;
                                stringstream ss7;
                                ss7<<"UPDATE orders SET customer_name=\'"<<(name_and_address.substr(0, name_and_address.find('*')))<<"\'";
                                name_and_address.erase(0, name_and_address.find('*')+1);
                                ss7<<", customer_address=\'"<<name_and_address<<"\'";
                                ss7<<" WHERE order_id=(SELECT order_id FROM orders WHERE customer_name=\'"<<temp_order<<"\');";
                                query = ss7.str();
                                cout<<query<<endl;
                                q = query.c_str();//вот тут можно использовать один и тот же q с самого начала как и query

                                qstate = mysql_query(connection,q);

                                    if(!qstate)
                                    {
                                        cout<<"Records inserted"<<endl;
                                        cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                                    }
                                    else
                                        error_message();
                            }

                            //отправляю строкой данные о сформированном заказе клиенту для печати
                            cout<<"Sending full order data from sever to client...."<<endl;

                            //формирую информацию для отправки

                            //из таблицы orders

                            stringstream ss7;
                            ss7<<"SELECT * FROM orders WHERE order_id=\'"<<id<<"\';";

                            string order_full_data_to_client;
                            query = ss7.str();
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

                            //из таблицы orders_detailed

                            order_full_data_to_client+='#';//добавляю разделитель для последующей обработки
                            stringstream ss8;
                            ss8<<"SELECT name, orders_detailed.amount, total_cost FROM orders_detailed INNER JOIN goods ON orders_detailed.good_id=goods.id WHERE order_id=\'"<<id<<"\';";

                            query = ss8.str();
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
                            send(Connections[index], (char*)&msg_size, sizeof(int), NULL);
                            send(Connections[index], order_full_data_to_client.c_str(), msg_size, NULL);

                            //списываю товар с остатков, т.к. заказ сформирован и отправлен
                            stringstream ss9;

                            ss9<<"UPDATE goods SET amount = (goods.amount-(SELECT orders_detailed.amount FROM orders_detailed WHERE order_id="<<id;
                            ss9<<") ) WHERE id IN (SELECT orders_detailed.good_id FROM orders_detailed WHERE order_id ="<<id;
                            ss9<<");";
                            query = ss9.str();
                            cout<<query<<endl;

                            q = query.c_str();//вот тут можно использовать один и тот же q с самого начала как и query

                            qstate = mysql_query(connection,q);

                            if(!qstate)
                            {
                                cout<<"goods.amount had been corrected"<<endl;
                                cout<<"Affected rows: "<<mysql_affected_rows(connection)<<endl;
                            }
                            else
                            {
                                cout<<"Shoot"<<endl;
                                error_message();

                            }

                    }
            }
            else
                break;
        }

        delete[] msg;
    }
}


int main(int argc, char* argv[])
{
    connect_sql();

    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2,1);

    if (WSAStartup(DLLVersion, &wsaData) !=0)
    {
        cout << "Error" << endl;
        exit(1);
    }

    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);

    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    SOCKET newConnection;

    for (int i = 0; i < 5; ++i)//изменить кол-во подключений
    {
        newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);

        if (newConnection == 0)
        {
            cout << "Error #2" << endl;
        }
        else
        {
            cout << "Client connected!" << endl;

            Connections[i] = newConnection;
            ++Counter;

            //создаю потоки используя std::thread

            //временно; нет защиты разделяемых данных;
            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(i), NULL, NULL);
        }
    }

    system("pause");

    mysql_free_result(res);
    mysql_close(connection);

    return 0;
}
