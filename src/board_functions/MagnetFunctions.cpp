#include "MagnetFunctions.h"
// #include "iProtocol.h"

using std::cin;
using std::cout;
using std::endl;

uint8_t *tmp1;


void whatToDoIfThermistorsTest(housekeeping_hdr_t *hdr_in) {
  cout << "converting to float resistance value in ohms (first appears the raw bytes) : " << endl;
  uint8_t array_temp[4];
  float res=0;
  tmp1 = (uint8_t *)array_temp;
// reverse the bytes for float conversion ugh
  for (int i = 0; i < hdr_in->len; i++) {
    *tmp1 = *((uint8_t *)hdr_in + 4 + i);
    cout << (int) *tmp1 << endl;
    tmp1 = tmp1 + 1;
  }
  memcpy(&res,&array_temp,4);
  cout << res << endl;

// use a union?

}

void whatToDoIfTempProbes(housekeeping_hdr_t *hdr_in){
  cout << "converting to float (first appears the raw bytes) : " << endl;
  uint8_t array_temp[4];
  float res=0;
  tmp1 = (uint8_t *)array_temp;

  for (int i = 0; i < hdr_in->len; i++) {
    *tmp1 = *((uint8_t *)hdr_in + 4 + i);
    cout << (int) *tmp1 << endl;
    tmp1 = tmp1 + 1;
  }
  memcpy(&res,&array_temp,4);
  cout << res << endl;

}

void whatToDoIfFloat(housekeeping_hdr_t *hdr_in){
  cout << "converting to float (first appears the raw bytes) : " << endl;
  uint8_t array_temp[4];
  float res=0;
  tmp1 = (uint8_t *)array_temp;

  for (int i = 0; i < hdr_in->len; i++) {
    *tmp1 = *((uint8_t *)hdr_in + 4 + i);
    cout << "raw byte " << i << ": "  << (int) *tmp1 << endl;
    tmp1 = tmp1 + 1;
  }
  memcpy(&res,&array_temp,4);
  cout << "float: " << res << endl;

}

void whatToDoIfPressure(housekeeping_hdr_t *hdr_in){
  cout << "converting to readable format : " << endl;
  uint8_t array_t[hdr_in->len];
  double pressure_value=0;
  double temperature_value=0;
  char typeOfPressure;
  char error_code[100];

  tmp1 = (uint8_t *)array_t;

  for (int i = 0; i < hdr_in->len; i++) {
    *tmp1 = *((uint8_t *)hdr_in + 4 + i);
//    cout << (int) *tmp1 << endl;
    tmp1 = tmp1 + 1;
  }

  if(array_t[hdr_in->len-1]==0){
    memcpy(&pressure_value,&array_t,sizeof(pressure_value));
    memcpy(&temperature_value,&array_t+sizeof(pressure_value),sizeof(temperature_value));
    memcpy(&typeOfPressure,&array_t+sizeof(pressure_value)+sizeof(temperature_value),sizeof(typeOfPressure));
    cout << "Pressure: " << pressure_value << " , " << temperature_value << " , " << typeOfPressure << endl;
  }
  else{
    memcpy(&error_code,&array_t,sizeof(error_code));
    for(int i =0; i<100; i++) {
      cout << error_code[i];
    }
  }
  cout << " " << endl;
  cout << "DONE" << endl;

}
void whatToDoIfFlow(housekeeping_hdr_t *hdr_in){
  cout << "converting to readable format : " << endl;
  uint8_t array_t[hdr_in->len];
  double gas_data[4];
  char gas_type[100];
  char error_code[100];
  tmp1 = (uint8_t *)array_t;

  for (int i = 0; i < hdr_in->len; i++) {
    *tmp1 = *((uint8_t *)hdr_in + 4 + i);
//    cout << (int) *tmp1 << " , ";
    tmp1 = tmp1 + 1;
  }

  if(array_t[hdr_in->len-1]==0){
    memcpy(&gas_data,&array_t,sizeof(gas_data));
    memcpy(&gas_type,&array_t+sizeof(gas_data),sizeof(gas_type));
    cout << "Gas Data: " << gas_data[0] << " , " << gas_data[1] << " , " << gas_data[2] << " , " << gas_data[3] << endl;
    cout << "Gas Type: ";
    for(int i =0; i<100; i++) {
      cout << gas_type[i];
    }
  }
  else{
    memcpy(&error_code,&array_t,sizeof(error_code));
    for(int i =0; i<100; i++) {
      cout << error_code[i];
    }
  }
  cout << " " << endl;
  cout << "DONE" << endl;

}