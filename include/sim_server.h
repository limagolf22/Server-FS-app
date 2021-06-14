#ifndef __SIM_SERVER_H__
#define __SIM_SERVER_H__

#include <string>
#include <windows.h>
#include "SimConnect.h"
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <mutex>          // std::mutex


enum COMMAND {
	ADD_REQ,
	ADD_VAL_DEF,
	DEF_REQ,
	TP_REQ
};

struct RPOS_resp {
	float heading;
	float elevationASL;
	float elevationAGL;
	float roll;
};

struct RREF_resp {
	float speed;
	float rpm;
};

struct Pos
{
    double  kohlsmann=29.92;
    double  altitude=5000.0;
    double  latitude;
    double  longitude;
	double 	roll=0.0;
	double  pitch=0.0;
	double  heading;
	double speed=95.0;
};

enum DATA_DEFINE_ID {
    DEFINITION_BREAKDOWNS,
	DEFINITION_RADPANNELS,
	DEF_RPOS,
	DEF_RREF,
	DEF_POS
}; //ID de datum (set de datas)

enum DATA_REQUEST_ID {
	REQUEST_RADPANNEL,
	REQUEST_2,
	REQUEST_RPOS,
	REQUEST_RREF
}; //ID d'un request


struct Inputs {
    int32_t IsFired=0;
};

struct Radpan {
    float VHF1 = 108;
};

void CALLBACK MyDispatchProc1(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
int initSimEvents();
int parser(std::string strings);

extern int quit;
extern HANDLE hSimConnect;

extern RPOS_resp RPOS_VAL;
extern RREF_resp RREF_VAL;

extern std::mutex m_values_lock;

extern HRESULT hr;

extern Inputs isfired; 

struct Simthrottle {
    int32_t throttle;
};
extern Simthrottle tc;

#endif

