#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_THREAD_

#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>

#include <set>

#include <websocketpp/common/thread.hpp>
#include <windows.h>
#include <stdio.h>

#include <iostream>
#include <sim_server.h>
#pragma comment(lib, "user32.lib")

#include "SimConnect.h"


#define BUF_SIZE 256




char STR_ENVOI_RPOS[100];
char STR_ENVOI_RREF[100];

std::mutex m_values_lock;
HANDLE hSimConnect = NULL;
int quit=0;
RPOS_resp RPOS_VAL;
RREF_resp RREF_VAL;

Simthrottle tc;

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

/* on_open insert connection_hdl into channel
 * on_close remove connection_hdl from channel
 * on_message queue send to all channels
 */



websocketpp::frame::opcode::value opcode_client = websocketpp::frame::opcode::value::TEXT;
bool check_co = false,check_empty = false;

enum action_type {
    SUBSCRIBE,
    UNSUBSCRIBE,
    MESSAGE
};

struct action {
    action(action_type t, connection_hdl h) : type(t), hdl(h) {}
    action(action_type t, connection_hdl h, server::message_ptr m)
      : type(t), hdl(h), msg(m) {}

    action_type type;
    websocketpp::connection_hdl hdl;
    server::message_ptr msg;
};

void send_a_message(server* s, websocketpp::connection_hdl hdl, std::string payload);
void float2Bytes(char bytes_temp[5],float float_variable);

class broadcast_server {
    public:
    broadcast_server() {
        // Initialize Asio Transport
        m_server.init_asio();

        m_server.set_access_channels(websocketpp::log::alevel::none);
        m_server.clear_access_channels(websocketpp::log::alevel::all);
        // Register handler callbacks
        m_server.set_open_handler(bind(&broadcast_server::on_open,this,::_1));
        m_server.set_close_handler(bind(&broadcast_server::on_close,this,::_1));
        m_server.set_message_handler(bind(&broadcast_server::on_message,this,::_1,::_2));
    }

    void run(uint16_t port) {
        // listen on specified port
        m_server.listen(port);

        // Start the server accept loop
        m_server.start_accept();

        // Start the ASIO io_service run loop
        try {
            m_server.run();
        } catch (const std::exception & e) {
            std::cout << e.what() << std::endl;
        }
    }

    void on_open(connection_hdl hdl) {
        {
            lock_guard<mutex> guard(m_action_lock);
           // std::cout << "on_open" << std::endl;
            m_actions.push(action(SUBSCRIBE,hdl));
        }
        m_action_cond.notify_one();
    }

    void on_close(connection_hdl hdl) {
        {
            lock_guard<mutex> guard(m_action_lock);
            //std::cout << "on_close" << std::endl;
            m_actions.push(action(UNSUBSCRIBE,hdl));
        }
        m_action_cond.notify_one();
    }

    void on_message(connection_hdl hdl, server::message_ptr msg) {
        // queue message up for sending by processing thread
        {
            lock_guard<mutex> guard(m_action_lock);
            //std::cout << "on_message" << std::end l;
            m_actions.push(action(MESSAGE,hdl,msg));
        }
        m_action_cond.notify_one();
    }


    void process_messages() {
        bool check=true;
        while(check) {
            unique_lock<mutex> lock(m_action_lock);

            while(m_actions.empty()) {
                m_action_cond.wait(lock);
            }

            action a = m_actions.front();
            m_actions.pop();

            lock.unlock();

            if (a.type == SUBSCRIBE) {
                std::cout <<"\nconnexion a un client\n";
                lock_guard<mutex> guard(m_connection_lock);
                m_connections.insert(a.hdl);
                guard.~lock_guard();
                check_co = true;
            } else if (a.type == UNSUBSCRIBE) {
                std::cout << "\ndeconnexion d'un client\n";
                lock_guard<mutex> guard(m_connection_lock);
                m_connections.erase(a.hdl);
                guard.~lock_guard();
            } else if (a.type == MESSAGE) {
                std::cout << "\n" << a.msg->get_payload() <<std::endl; // affichage du message
                parser(a.msg->get_payload());
            } else {
                // undefined.
            }
        }
        return;
    }

    void send_messages() {
        while(!check_co) {
            Sleep(10);
        }
        while (check_co) {

            lock_guard<mutex> guard(m_connection_lock);

            con_list::iterator it;
            if (m_connections.empty()) {
                if (!check_empty){
                    std::cout << "list of clients empty\n";
                    check_empty = true;
                }
            }
            else {
                check_empty = false;
                for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                    send_a_message(&m_server,*it,"50");
                }
            }
            guard.~lock_guard();
            Sleep(500);
        }
        return;
    }

    void process_simconnect() {
        if (initSimEvents()) {
            while (quit == 0) {
                    // Continuously call SimConnect_CallDispatch until quit - MyDispatchProc1 will handle simulation events
                    SimConnect_CallDispatch(hSimConnect, MyDispatchProc1, NULL);
                    Sleep(25);
                }                
            hr = SimConnect_Close(hSimConnect);
            return;
        }
        else {
            std::cout << "\nFailed to Connect!!!!\nPassing to test values...\n";
            int q;
            while (true) {
                m_values_lock.lock();

                RPOS_VAL.heading = q*8+0.01;
                RPOS_VAL.elevationASL = 2700+q+0.01;
                RPOS_VAL.elevationAGL = 2480+q+0.01;
                RPOS_VAL.roll = q-20+0.01;

                RREF_VAL.speed = 95+(q-20)/2+0.01;
                RREF_VAL.rpm = 2300 +(q-20)*10+0.01;

                m_values_lock.unlock();
                q=(q+1)%41;
                Sleep(100);
            };
            return;
            
        }
    }

    private:
    typedef std::set<connection_hdl,std::owner_less<connection_hdl> > con_list;

    server m_server;
    con_list m_connections;
    std::queue<action> m_actions;

    mutex m_action_lock;
    mutex m_connection_lock;
    condition_variable m_action_cond;
};

void send_RPOS(server* s, websocketpp::connection_hdl hdl) {
    char cache_RPOS[4][5];
    m_values_lock.lock();
    float floats_RPOS[4] = {RPOS_VAL.heading,RPOS_VAL.elevationASL,RPOS_VAL.elevationAGL,RPOS_VAL.roll};
    m_values_lock.unlock();
    int i;
    for (i=0; i<4; i++){
        float2Bytes(cache_RPOS[i],floats_RPOS[i]);
    }
    sprintf(STR_ENVOI_RPOS,"%d;%.1f;%.1f;%.1f;%.1f\0",0,RPOS_VAL.heading,RPOS_VAL.elevationASL,RPOS_VAL.elevationAGL,RPOS_VAL.roll);

    s->send(hdl, STR_ENVOI_RPOS, opcode_client);
}

void send_RREF(server* s, websocketpp::connection_hdl hdl) {
    char cache_RREF[2][5];
    cache_RREF[0][4]='\0';
    cache_RREF[1][4]='\0';
    m_values_lock.lock();
    sprintf(STR_ENVOI_RREF,"%d;%.1f;%.1f\0",1,RREF_VAL.speed,RREF_VAL.rpm);
    m_values_lock.unlock();
    int i;
    for (i=0; i<2; i++){
      //  float2Bytes(cache_RREF[i],floats_RREF[i]);
    }
    s->send(hdl, STR_ENVOI_RREF, opcode_client);

}

bool one_err=false;
void send_a_message(server* s, websocketpp::connection_hdl hdl, std::string payload) {
    try {
        send_RPOS(s,hdl);
        send_RREF(s,hdl);
        one_err=false;
        std::cout << "\rvaleurs envoyees : " << STR_ENVOI_RREF << " --- " << STR_ENVOI_RPOS;// << "\n";
    }
    catch (websocketpp::exception const & e) {
        m_values_lock.unlock();
        if (!one_err) {
            one_err=true;
            std::cout << "send failed\n";
//            std::cout << "Echo failed because: "<< "(" << e.what() << ")\n";
        }

    }
}

void float2Bytes(char bytes_temp[5],float float_variable){ 
  memcpy(bytes_temp, (unsigned char*) (&float_variable), 4);
}


int main(int argc, char *argv[]) {
    printf("initilisation du serveur\n");
    int nport;
    if (argc > 1) {
        nport=std::stoi(argv[1]);
    }
    else {
        nport=9002;
    }

    try {
    broadcast_server server_instance;

    // Start a thread to run the processing loop  
    thread t(bind(&broadcast_server::process_messages,&server_instance));
    thread t2(bind(&broadcast_server::send_messages,&server_instance));
    thread t3(bind(&broadcast_server::process_simconnect,&server_instance));

    printf("attente de connexion\n");
    // Run the asio loop with the main thread
    server_instance.run(nport);

    t.join();
    printf("sortie d'un thread\n");
    t2.join();
    printf("sortie deux threads\n");
    t3.join();
    printf("sortie trois threads\n");
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}