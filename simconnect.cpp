#include <string>
#include <vector>
#include <windows.h>
#include "SimConnect.h"
#include "sim_server.h"

#include <map>
using namespace std;

map<string,int> Reqs;
map<string,int> Defs;

void add_Req(string name) {
    int nbr = Reqs.size();
    Defs[name] = nbr;
  //  Defs.insert(pair(name,nbr));
    return;
}

void add_Def(string name) {
    int nbr = Defs.size();
    Defs[name] = nbr;
  //  Defs.insert(pair(name,nbr));
    return;
}

void add_data(string def, char name[], char unit[], SIMCONNECT_DATATYPE datatype=SIMCONNECT_DATATYPE_FLOAT32) {
    if ( Defs.find(def) == Defs.end() ) {
        add_Def(def);
    }
    hr = SimConnect_AddToDataDefinition(hSimConnect, Defs[def], name, unit, datatype);
    return;
}

void validate_req(string reqnum, string def, SIMCONNECT_PERIOD simper) {
    hr = SimConnect_RequestDataOnSimObject(hSimConnect, Reqs[reqnum], Defs[def], SIMCONNECT_OBJECT_ID_USER, simper);
    return;
}


Inputs isfired;
Radpan radpan;

void CALLBACK MyDispatchProc1(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext) {
	switch (pData->dwID)
	{

	case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
	{
		SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;

		switch (pObjData->dwRequestID)
		{
        case REQUEST_RPOS:{
            RPOS_resp* provi = (RPOS_resp*)&pObjData->dwData;
            m_values_lock.lock();
            RPOS_VAL.heading = provi->heading;
            RPOS_VAL.elevationASL = provi->elevationASL;
            RPOS_VAL.elevationAGL = provi->elevationAGL;
            RPOS_VAL.roll = provi->roll;
		//	printf("\rval simco RPOS : %f %lf %lf %f",RPOS_VAL.heading,RPOS_VAL.elevationASL,RPOS_VAL.elevationAGL,RPOS_VAL.roll);
            m_values_lock.unlock();
        }
        case REQUEST_RREF:{
            RREF_resp* provi = (RREF_resp*)&pObjData->dwData;
            m_values_lock.lock();
            RREF_VAL.speed = provi->speed;
            RREF_VAL.rpm = provi->rpm;
            m_values_lock.unlock();
        }

		}
		break;
	}

	case SIMCONNECT_RECV_ID_QUIT:
	{
		quit = 1;
		break;
	}

	default:
		break;
	}
}

enum GROUP_ID {
    GROUP0,
};


HRESULT hr;
int initSimEvents() {
        
        if (SUCCEEDED(SimConnect_Open(&hSimConnect, "Client Event Demo", NULL, 0, NULL, 0))) {
            std::cout << "\nConnected To Microsoft Flight Simulator 2020!\n";

            //DEFINITION <=> priority 
            // DATA
            hr = SimConnect_AddToDataDefinition(hSimConnect, DEF_RPOS, "HEADING INDICATOR", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
            hr = SimConnect_AddToDataDefinition(hSimConnect, DEF_RPOS, "Indicated Altitude", "feet", SIMCONNECT_DATATYPE_FLOAT32);
            hr = SimConnect_AddToDataDefinition(hSimConnect, DEF_RPOS, "Radio Height", "feet", SIMCONNECT_DATATYPE_FLOAT32);
            hr = SimConnect_AddToDataDefinition(hSimConnect, DEF_RPOS, "ATTITUDE INDICATOR BANK DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32);

            hr = SimConnect_AddToDataDefinition(hSimConnect, DEF_RREF, "Airspeed Indicated", "knots", SIMCONNECT_DATATYPE_FLOAT32);
            hr = SimConnect_AddToDataDefinition(hSimConnect, DEF_RREF, "General Eng RPM:1", "Rpm", SIMCONNECT_DATATYPE_FLOAT32);


            hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_BREAKDOWNS, "ENG ON FIRE", "Bool", SIMCONNECT_DATATYPE_INT32);

            hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_RPOS, DEF_RPOS, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME);
            hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_RREF, DEF_RREF, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME);

            // Process incoming SimConnect Server messages
            return 1;
	    }
        else {
            return 0;
        }
}

int parser(std::string strings) {
    char *buffer = &strings[0];
    COMMAND command;
    char values[250];
    sscanf(buffer,"%d %s", &command, values);
    printf("datum:%d,value:%s\n",command,values);
    switch (command) {
        case ADD_REQ:{
            char req[50];
            sscanf(values, "%s",req);
            string toSR(req);
            add_Req(toSR);
            printf("req %s added\n", req);
        }

        case ADD_VAL_DEF:{
            char defi[50]; char name[50];char unit[50];SIMCONNECT_DATATYPE dtt;
            sscanf(values, "[^;];[^;];[^;];%d",defi, name, unit, &dtt);
            string toS(defi);
            add_data(toS, name, unit, dtt);
            printf("%s def has received the value %s\n",defi,name);

        }
        case DEF_REQ:{
            char reqname[50]; char defi2[50]; int simper;
            sscanf(values, "[^;];[^;];%d",reqname, defi2, &simper);
            string toSreq(reqname);
            string toSdef(defi2);
            validate_req(toSreq, toSdef, (SIMCONNECT_PERIOD)simper);
            

        }

       
        return 0;
    }
    return 1;
}