// #include "FindHwmon.h"
#include <stdio.h>
#include <iostream>
#include <string>
// #include <cstring>
#include <algorithm>  // std::max
#include <fstream>    //read  and write files
#include <filesystem> // file browsing
#include <unistd.h>   // sleep library
// #include <stdexcept> //error handling
// #include <gtk/gtk.h> // gtk interface

// #include <getopt.h> //parsing

#include <err.h>
#include <fcntl.h>
#include <unistd.h>

namespace fs = std::filesystem;

#define ECio "/sys/kernel/debug/ec/ec0/io"

// Byte offsets
#define ManualECMode_cpu 147 // 0x93
#define ManualECMode_gpu 150 // 0x96
#define GPUaddr 151 // 0x97
#define CPUaddr 148 //0x94

// Hex fan speeds
#define ZERO 255 // 0xFF -- 0 RPM
#define SLOW 240 // 0xF0 -- 2000RPM
#define MEDIUM 200 // 204 is 0xCC -- 2400 RPM
#define NORMAL 163 // 0xA3 -- 3000 RPM
#define FAST 102 // 0x66 -- 4800 RPM
#define BOOST 91 // 0x5B -- 5400 RPM

//Temperature thresholds
#define mou 5
uint8_t t1;
uint8_t t2;
uint8_t t3;
uint8_t t4=0;

const std::string hwmon = "/sys/class/hwmon";

// harware variables
int cpu_temp;
int gpu_temp;
int cpu_fan;
int gpu_fan;

// Dellsmm pathes
std::string GPU_path;
std::string CPU_path;
std::string CPU_fan_path;
std::string GPU_fan_path; 
std::string dellsmm="";


//  Gets the needed paths.
void Hwmon_get()
{
    
    for (const auto &entry : fs::directory_iterator(hwmon))
    {
        std::ifstream namepath = std::ifstream(entry.path().string() + "/name");
        std::string name = std::string((std::istreambuf_iterator<char>(namepath)),
                                       (std::istreambuf_iterator<char>()));
        if (name == "dell_smm\n")
        {
            dellsmm = entry.path().string();
            for (const auto & file : fs::directory_iterator(dellsmm)){

                std::ifstream a;
                std::string file_path = file.path().string();
                a.open(file_path);
                std::string sensor_name;
                a >> sensor_name;
                a.close();

                if (sensor_name == "GPU"){
                    GPU_path = file_path;
                    GPU_path.replace(GPU_path.length()-5,5,"input");
                }
                else if (sensor_name =="CPU")
                {
                    CPU_path = file_path;
                    CPU_path.replace(CPU_path.length()-5,5,"input");
                }
                else if (sensor_name =="Processor")
                {
                    CPU_fan_path = file_path;
                    CPU_fan_path.replace(CPU_fan_path.length()-5,5,"input");
                }
                else if (sensor_name =="Video")
                {
                    GPU_fan_path = file_path;
                    GPU_fan_path.replace(GPU_fan_path.length()-5,5,"input");
                }
            }
        }
        // else if (name == "zenpower\n")
        // {
        //     CPU = entry.path().string();
        // }
        // else if (name == "k10temp\n")
        // {
        //     CPU = entry.path().string();
        // }
        // else if (name == "amdgpu\n")
        // {
        //     // There are two amd gpus (iGPU and dGPU) an easy way to differentiate them is to check the presence of pwm1. Another way would be to check vbios version (may be more robust).
        //     if (fs::exists(entry.path().string() + "/pwm1"))
        //     {
        //         dGPU = entry.path().string();
        //     }
        // }
    }
    if (dellsmm == ""){
        std::cout << "Cannot find Dell-smm-hwmon. Try 'modprobe dell-smm-hwmon restricted=0 ignore_dmi=1'. " << std::endl;
        exit(EXIT_FAILURE);
    }
};

// Updates the thermals and fan variables.
void update_vars()
{
    std::ifstream a;
    a.open(CPU_path); //CPU dellsmm
    a >> cpu_temp;
    a.close();
    a.open(GPU_path); //GPU dellsmm
    a >> gpu_temp;
    a.close();
    a.open(CPU_fan_path); //Processor fan
    a >> cpu_fan;
    a.close();
    a.open(GPU_fan_path); // Video fan
    a >> gpu_fan;
    a.close();
};

// Set cpu fans to selected speed. Input should be in the set {0,128,256}.
void set_cpu_fan_smm(int left)
{
    // Force left to be in [0,256]
    int l = std::max(0, std::min(255, left));
    // Writes to hwmon
    std::ofstream pwm;
    pwm.open(dellsmm + "/pwm1");
    pwm << l;
    pwm.close();
};
// Set gpu fans to selected speed. Input should be in the set {0,128,256}.
void set_gpu_fan_smm(int right)
{
    // Force right to be in [0,256]
    int r = std::max(0, std::min(255, right));
    // Writes to hwmon
    std::ofstream pwm;
    pwm.open(dellsmm + "/pwm3");
    pwm << r;
    pwm.close();
};


void write_to_ec(int byte_offset, uint8_t value){
    int fd = open(ECio, O_WRONLY);
    int error;

	error = lseek(fd, byte_offset, SEEK_SET);
	if (error != byte_offset)
		err(EXIT_FAILURE, "Cannot set offset to 0x%.2x", byte_offset);

	error = write(fd, &value, 1);
	if (error != 1)
		err(EXIT_FAILURE, "Cannot write value 0x%.2x to offset 0x%.2x",
		    value, byte_offset);
}

void manual_fan_mode(bool on)
{
    if(on){
        write_to_ec(ManualECMode_cpu, 255);
        write_to_ec(ManualECMode_gpu, 255);
    }
    else
    {
        write_to_ec(ManualECMode_cpu, 4);
        write_to_ec(ManualECMode_gpu, 4); 
    }
}



void check_fan_write_permission()
{
    std::ofstream pwm;
    pwm.open(dellsmm + "/pwm1");
    if (!pwm.is_open())
    {
        std::cout << "Cannot change fan speed. Are you running the script with root permission ?" << std::endl;
        exit(EXIT_FAILURE);
    }
    pwm.close();
};

void set_cpu_fan(int speed){
    write_to_ec(CPUaddr, speed);
};
void set_gpu_fan(int speed)
{
    write_to_ec(GPUaddr, speed);
};

// Updates fans accordings to temp.
void update_fans__OLD()
{
    
};

uint8_t hex_to_EC(uint8_t hex){
    return std::min(std::max(255-hex, 91),255);
};

void print_status()
{
    std::cout << "Current fan speeds : " << cpu_fan << " RPM and " << gpu_fan << " RPM.      " << std::endl;
    std::cout << "CPU and GPU temperatures : " << cpu_temp/1000 << "°C and " << gpu_temp/1000 << "°C.  " << std::endl;
    std::cout << "\033[2F";
};


int fan_curve(uint8_t current_temp, uint8_t current_fan){
    if (current_temp <t1)
    {
        if(current_temp <t1-mou){
            return ZERO;
        }
        return -1;
    }
    if (current_temp <t2)
    {
        if((current_temp <t2-mou && current_fan >2500) || current_fan < 1500){
            return SLOW;
        }
        return -1;
    }
    if (current_temp <t3)
    {
        if((current_temp <t3-mou && current_fan > 3500 ) || current_fan < 2500){
            return NORMAL;
        }
        return -1;
    }
    if (current_temp <t4 )
    {
        if((current_temp <t4-mou && current_fan > 4900) || current_fan < 3500){
            return FAST;
        }
        return -1;
    }
    return BOOST;
}

void fan_update(){
    int cpu_update = fan_curve(cpu_temp/1000,cpu_fan/1000);
    if (cpu_update != -1 )
        set_cpu_fan(cpu_update);

    int gpu_update = fan_curve(gpu_temp/1000,gpu_fan/1000);
    if (gpu_update != -1 ) 
        set_gpu_fan(gpu_update);

}
void usage(char* prog_name, int status){
    printf("Usage :\n");
    printf("sudo %s [-s left_fan_speed right_fan_speed] [-l t1 t2 t3 t4] [-t timer] [-r ] [-b]\n\n", prog_name);
    printf("Arguments description :\n");
    printf(" -h, --help             show this help message and exit.\n");
    printf(" -s, --set left right   sets left/rights fan to selected speed (from 0 to 255).\n");
    printf(" -l, --loop t1 t2 t3 t4 given the temperature thresholds t1, t2, t3, t4 (in °C),\n");
    printf("                        adjusts the fan speed accordingly with a loop.\n");
    printf(" -t, --timer t          set the loop timer (in seconds). Default is 20s.\n");
    printf(" -r, --restore          Gives back the fan control to the BIOS.\n");
    printf(" -b, --boost            Set fan speed to BOOST (as fn+F7).\n");
    exit(status);
}


int main(int argc, char* argv[])
{
    uint timer=20;
    // Argument Parsing
    for(int i=1; i<argc; i++)
    {
        if (std::string(argv[i])=="--restore" || std::string(argv[i])=="-r")
        {
            manual_fan_mode(false);
            printf("Returned to BIOS Fan control.\n");
            exit(EXIT_SUCCESS);
        }
        if (std::string(argv[i])=="--boost" || std::string(argv[i])=="-b")
        {
            manual_fan_mode(true);
            set_gpu_fan(BOOST);
            set_cpu_fan(BOOST);
            printf("Boost speed. Be carefull, manual fan mode is on.");
            exit(EXIT_SUCCESS);
        }
        if (std::string(argv[i])=="--set" || std::string(argv[i])=="-s")
        {
            if (argc < i+3)
            {
                printf("Need more arguments.\n");
                exit(EXIT_FAILURE);
            }
            
            manual_fan_mode(true);
            
            uint8_t left = std::stoi(argv[i+1]);
            set_cpu_fan(hex_to_EC(left));
            uint8_t right = std::stoi(argv[i+2]);
            set_gpu_fan(hex_to_EC(right));

            std::cout << "Set fans to "<< std::stoi(argv[i+1]) <<" and " << std::stoi(argv[i+2])<< ". Be carefull, manual fan mode is on."<< std::endl;
            exit(EXIT_SUCCESS);
        }
        if (std::string(argv[i])=="--timer" || std::string(argv[i])=="-t")
        {
            if (argc <i+1)
            {
                exit(EXIT_FAILURE);
            }
            timer = std::stoi(argv[i+1]);
        }
        if (std::string(argv[i])=="--loop" || std::string(argv[i])=="-l")
        {
            if (argc < i+5)
            {
                printf("Need 4 thresholds.\n");
                exit(EXIT_FAILURE);
            }
            t1 = std::stoi(argv[i+1]);
            t2 = std::stoi(argv[i+2]);
            t3 = std::stoi(argv[i+3]);
            t4 = std::stoi(argv[i+4]);
        }
        if (std::string(argv[i])=="--help" || std::string(argv[i])=="-h")
            usage(argv[0],EXIT_SUCCESS);
        
    }
    if (t4==0) usage(argv[0],EXIT_FAILURE);
    // Get hwmon variables
    Hwmon_get();
    // Check if launched with enough permissions.
    check_fan_write_permission();
    // Set fans to manual mode.
    // manual_fan_mode(true);
    printf("Set fans to manual mode.\n");
    // Fan update loop
    while (true)
    {
        //First update the variables.
        update_vars();
        //Then update the fan speed accordingly.
        fan_update();
        //Prints current status
        print_status();
        // wait $timer seconds
        sleep(timer);
    }
    return EXIT_SUCCESS;
}

