#include "oemcmd.hpp"
#include <host-ipmid/ipmid-api.h>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <systemd/sd-bus.h>
#include <endian.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include <linux/types.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/timer.hpp>
#include <gpiod.hpp>

void register_netfn_mct_oem() __attribute__((constructor));

int execmd(char* cmd,char* result) {
        char buffer[128];
        FILE* pipe = popen(cmd, "r");
        if (!pipe)
                return -1;

        while(!feof(pipe)) {
                if(fgets(buffer, 128, pipe)){
                        strcat(result,buffer);
                }
        }
        pclose(pipe);
        return 0;
}

std::unique_ptr<phosphor::Timer> clrCmosTimer = nullptr;
#define AST_GPIO_F1_PIN 41 
bool clrCmos()
{
#if 0    
    //todo : move to device tree by assigned name 
    auto line = gpiod::find_line("CLR_CMOS");
        
    if (!line)
    {
        return false;
    }
#endif 
    auto chip = gpiod::chip("gpiochip0");
    auto line = chip.get_line(AST_GPIO_F1_PIN);
    if (!line)
    {
        std::cerr << "Error requesting gpio AST_GPIO_F1_PIN\n";
        return false;
    }
#if 0
    auto dir = line.direction();
    std::cerr << "direction :" <<dir<<"\n";
#endif 
    int value = 1;

    try
    {
        line.request({"ipmid", gpiod::line_request::DIRECTION_OUTPUT,0}, value);
    }
    catch (std::system_error&)
    {
        std::cerr << "Error requesting gpio\n";
        return false;
    }

    line.set_value(0);
    sleep(3);
    line.set_value(1);
    
    line.release();
    return true;

}
void createTimer()
{
    if (clrCmosTimer == nullptr)
    {
        clrCmosTimer = std::make_unique<phosphor::Timer>(clrCmos);
    }
}

/*
    NetFun: 0x3e
    Cmd : 0x3a
    Request:
*/
ipmi_ret_t ipmiOpmaClearCmos(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    ipmi_ret_t ipmi_rc = IPMI_CC_OK;

    //todo, check pgood status
    createTimer();
    if (clrCmosTimer == nullptr)
    {
        return IPMI_CC_RESPONSE_ERROR;
    }
    
    if(clrCmosTimer->isRunning())
    {
        return IPMI_CC_RESPONSE_ERROR;
    }
    clrCmosTimer->start(std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::seconds(0)));    
   
    return ipmi_rc;

}

/* Set Fan Control Enable Command
NetFun: 0x2E/ 0x30
Cmd : 0x06
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 :  0h - Fan Control Disable
                  1h - Fan Control Enable
                  ffh - Get Current Fan Control Status
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte (5) : Current Fan Control Status , present if FFh passed to Enable Fan Control in Request
*/
ipmi_ret_t ipmi_tyan_ManufactureMode(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    ipmi_ret_t ipmi_rc = IPMI_CC_OK;
    std::fstream file;

    int rc=0;
    char command[100];
    char FSCStatus[100];
    uint8_t responseData[4]={0};
    uint8_t currentStatus;

    auto* requestData= reinterpret_cast<ManufactureModeRequest*>(request);

    if((int)*data_len != 4) 
    {
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    for(int i=0; i<(int)((*data_len)-1); i++)
    {
        responseData[i]=requestData->manufactureID[i];
    }

    file.open("/usr/sbin/fsc",std::ios::in);
    if(!file)
    {
        memset(command,0,sizeof(command));
        sprintf(command, "touch /usr/sbin/fsc");
        system(command);

        memset(command,0,sizeof(command));
        sprintf(command, "echo 1 > /usr/sbin/fsc");
        system(command);

        currentStatus=1;
    }
    else
    {
        file.read(FSCStatus,sizeof(FSCStatus));

        currentStatus = strtol(FSCStatus,NULL,16);
        file.close();
    }
    file.close();

    if (requestData->mode == 0)
    {
        // Disable Fan Control
        *data_len=3;
        memset(command,0,sizeof(command));
        sprintf(command, "systemctl stop phosphor-pid-control.service");
        rc = system(command);
    
        memset(command,0,sizeof(command));
        sprintf(command, "echo 0 > /usr/sbin/fsc");
        system(command);
    }
    else if (requestData->mode == 1)
    {
        // Enable Fan Control
        *data_len=3;
        memset(command,0,sizeof(command));
        sprintf(command, "systemctl start phosphor-pid-control.service");
        rc = system(command);

        memset(command,0,sizeof(command));
        sprintf(command, "echo 1 > /usr/sbin/fsc");
        system(command);
    }
    else if (requestData->mode == 0xff)
    {
        // Get Current Fan Control Status
        *data_len=4;
        responseData[3]=currentStatus;
    }
    else
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    if (rc != 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    memcpy(response, responseData, sizeof(responseData));

    return ipmi_rc;
}

/* Set Fan Control PWM Duty Command
NetFun: 0x2E/ 0x30
Cmd : 0x05
Request:
        Byte 1-3 : Tyan Manufactures ID (FD 19 00)
        Byte 4 : PWM ID ( 0h-PWM1 , ... , 4h-PWM5 )
        Byte 5 : Duty Cycle
            0-64h - manual control duty cycle (0%-100%)
            FEh - Get current duty cycle
            FFh - Return to automatic control
Response:
        Byte 1 : Completion Code
        Byte 2-4 : Tyan Manufactures ID
        Byte (5) : Current Duty Cycle , present if 0xFE passed to Duty Cycle in Request
            [7] : 0b : PWM work on automatic
            [6:0] : Duty Cycle
*/
ipmi_ret_t ipmi_tyan_FanPwmDuty(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    ipmi_ret_t ipmi_rc = IPMI_CC_OK;
    std::fstream file;

    int rc=0;
    char command[100];
    char temp[50];
    uint8_t responseData[4]={0};
    uint8_t pwmValue = 0;

    auto* requestData= reinterpret_cast<FanPwmDutyRequest*>(request);

    if((int)*data_len != 5) 
    {
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    for(int i=0; i<(int)((*data_len)-2); i++)
    {
        responseData[i]=requestData->manufactureID[i];
    }

    if (requestData->duty == 0xfe)
    {
        // Get current duty cycle
        *data_len=4;

        switch (requestData->pwmId)
        {
            case 0:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm1");
                    break;
            case 1:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm2");
                    break;
            case 2:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm3");
                    break;
            case 3:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm4");
                    break;
            case 4:
                    memset(command,0,sizeof(command));
                    sprintf(command, "cat /sys/class/hwmon/hwmon0/pwm5");
                    break;
            default:
                    return IPMI_CC_PARM_OUT_OF_RANGE;
        }

        memset(temp, 0, sizeof(temp));
        rc = execmd((char *)command, temp);

        if (rc != 0)
        {
            ipmi_rc = IPMI_CC_UNSPECIFIED_ERROR;
            return ipmi_rc;
        }

        pwmValue = strtol(temp,NULL,10);
        responseData[3] = pwmValue*100/255;
    }
    else if (requestData->duty == 0xff)
    {
        // Return to automatic control
        *data_len=3;

        memset(command,0,sizeof(command));
        sprintf(command, "systemctl start phosphor-pid-control.service");
        rc = system(command);
    }
    else if (requestData->duty <= 0x64)
    {
        // control duty cycle (0%-100%)
        *data_len=3;
        pwmValue = requestData->duty*255/100;

        switch (requestData->pwmId)
        {
            case 0:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm1", pwmValue);
                    break;
            case 1:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm2", pwmValue);
                    break;
            case 2:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm3", pwmValue);
                    break;
            case 3:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm4", pwmValue);
                    break;
            case 4:
                    memset(command,0,sizeof(command));
                    sprintf(command, "echo %d > /sys/class/hwmon/hwmon0/pwm5", pwmValue);
                    break;
            default:
                    return IPMI_CC_PARM_OUT_OF_RANGE;
        }

        rc = system(command);
    }
    else
    {
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    if (rc != 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    memcpy(response, responseData, sizeof(responseData));

    return ipmi_rc;
}

void register_netfn_mct_oem()
{
    ipmi_register_callback(NETFUN_TWITTER_OEM, IPMI_CMD_ClearCmos, NULL, ipmiOpmaClearCmos, PRIVILEGE_ADMIN);
    ipmi_register_callback(NETFUN_TWITTER_OEM, IPMI_CMD_ManufactureMode, NULL, ipmi_tyan_ManufactureMode, SYSTEM_INTERFACE);
    ipmi_register_callback(NETFUN_TWITTER_OEM, IPMI_CMD_FanPwmDuty, NULL, ipmi_tyan_FanPwmDuty, SYSTEM_INTERFACE);
}