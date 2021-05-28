#ifndef STUBBORN_h
#define STUBBORN_h

// Mission specific values. These can be shared between the Rover and the Ground State
#define EVT_MAX_HANDLERS 8 

#define CI_CMD_EXT_STOP  4
#define CI_CMD_EXT_FWD   5
#define CI_CMD_EXT_RT    6
#define CI_CMD_EXT_LT    7
#define CI_CMD_EXT_BCK   8
#define CI_MAX_CMDS 10

#define TO_PARAM_ERROR    1
#define TO_PARAM_MILLIS   2 
#define TO_PARAM_LOOP     3 
#define TO_PARAM_COM_SEQ  10
#define TO_PARAM_RFM_RSSI 20
#define TO_PARAM_MOTOR_A 40
#define TO_PARAM_MOTOR_B 41
#define TO_PARAM_IMPACT  50

#endif