#include "controlador.h"
#include "accelerometer.h"
#include "color_sensor.h"
#include "ticker.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <bitset>
#include <atomic>

using namespace std;

#define MAX_STRING_LENGTH 100

#define DEBUG

typedef struct
{
float acc_x, acc_y, acc_z;
} t_acc_data;

typedef struct
{
uint8_t flags;
t_proc_color color;
t_acc_data acceleration;
} t_send_data;

t_send_data send_data = {};

t_raw_color raw_colors = {};
t_proc_color proc_colors = {};

float ax, ay, az; // Acceleration values

extern uint8_t accelerometer_alive, color_sensor_alive;

sockaddr_in server;
int sock_fd;

uint8_t init_comunication = 0, first_msg = 1, ack_ok = 1;

#ifdef MOSQUITTO
#include <mosquitto.h>
#include <mqtt_protocol.h>
std::string generate_json(){
std::string msg = "{";
if (accelerometer_alive)
{
	msg.append("\"acc_x\":");
	msg.append(std::to_string(ax));
	msg.append(",\"acc_y\":");
	msg.append(std::to_string(ay));
	msg.append(",\"acc_z\":");
	msg.append(std::to_string(az));
	msg.append("");
}
if (color_sensor_alive && accelerometer_alive)
	msg.append(",");

if (color_sensor_alive)
{
	msg.append("\"red\": ");
	msg.append(std::to_string(proc_colors.red));
	msg.append(", \"green\": ");
	msg.append(std::to_string(proc_colors.green));
	msg.append(", \"blue\": ");
	msg.append(std::to_string(proc_colors.blue)).append("");
}
msg.append("}");
return msg;
}
void send_data_to_host()
{
if (ntohs(server.sin_port) != 1883)
	cout << "WARNING: Port is not 1883. Current port is: " << ntohs(server.sin_port) << endl;

string msg = "mosquitto_pub -d -q 1 -h \"" + std::string(inet_ntoa(server.sin_addr)) + "\" -p \"" + std::to_string(ntohs(server.sin_port)) + "\" -t \"v1/devices/me/telemetry\" -u \"4RGkIbCxNXEHydBzNLvC\" -m \"" + generate_json() + "\"";

char *char_array = new char[msg.length() + 1];
strcpy(char_array, msg.c_str());
#ifdef DEBUG
cout << "[DEGUG] Data: " << char_array << endl;
cout << (std::string(inet_ntoa(server.sin_addr))).c_str() << endl;
#endif
system(char_array);
}
#endif

#ifdef NON_MOSQUITTO
void wait_ack(){
	int len=sizeof(server);
	char server_reply[50];
	if (recvfrom(sock_fd, (void*) server_reply, (size_t) (MAX_STRING_LENGTH+1), MSG_WAITALL, (sockaddr *) &server, (socklen_t*) &len)==-1) perror("recvfrom:");
	if(first_msg == 1){
		if(std::string(server_reply) == "Hello RPI") first_msg = 0;
	}else
		if(std::string(server_reply) != "Server received data") ack_ok = 0; //ACK msg
	#ifdef DEBUG
	cout<<"The server sent: "<< server_reply << endl << endl;
	#endif
}
void send_data_to_host(){
	if (init_comunication){
	char buffer[sizeof(send_data)];
	memcpy(buffer, reinterpret_cast<char *>(&send_data), sizeof(send_data)); // Serialize the structure
	#ifdef DEBUG
	cout << "[DEBUG] Sending data" << endl;
	#endif
	if (sendto(sock_fd, buffer, sizeof(buffer), 0, (sockaddr *)&server, sizeof(server)) == -1)
		cout << "ERROR SENDING DATA" << endl; // Send to the server
	}
	else{
	char msg_init[30] = "Hello Server";
	init_comunication = 1;
	if (sendto(sock_fd, msg_init, sizeof(msg_init), 0, (sockaddr *)&server, sizeof(server)) == -1)
		cout << "ERROR SENDING DATA" << endl; // Send to the server
	}
	wait_ack();
}
#endif

extern atomic<int> color_sensor_data_ready, acc_data_ready;
char copy_color_sensor_msg[1500];
float copy_acc_values[3];
uint8_t represent_acc_data = 0, represent_color_sensor_data = 0;

uint8_t flash = 0;

char color_sensor_msg[1500] = "";

pthread_t controlador, timer;

int cnt_send = 0; // Counter for sending data

int main(int argc, char *argv[])
{
if (argc < 3){
	cout << "Please pass the ip adress and port number on which the server is listening." << endl;
	exit(-1);
}


if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
	perror("socket: ");
	exit(-1);
}
//   ip_addr = argv[1];
server.sin_family = AF_INET;
server.sin_port = htons(atoi(argv[2]));
server.sin_addr.s_addr = inet_addr(argv[1]);
bzero(&server.sin_zero, 8);

pthread_create(&controlador, NULL, control, NULL);

while (ack_ok){
	if (accelerometer_alive && color_sensor_alive){
		if (atomic_load(&color_sensor_data_ready) == 1)
		{
			atomic_exchange(&color_sensor_data_ready, 0);
			represent_color_sensor_data = 1;
			sprintf(copy_color_sensor_msg, color_sensor_msg);
		}
		if (atomic_load(&acc_data_ready) == 1)
		{
			atomic_exchange(&acc_data_ready, 0);
			represent_acc_data = 1;
			copy_acc_values[0] = ax;
			copy_acc_values[1] = ay;
			copy_acc_values[2] = az;
		}
		if (represent_color_sensor_data && represent_acc_data)
		{
			fflush(stdout);
			represent_acc_data = represent_color_sensor_data = 0;

			send_data.flags = 3; // accelerometer alive && color sensor alive
			send_data.color = proc_colors;
			send_data.acceleration.acc_x = ax;
			send_data.acceleration.acc_y = ay;
			send_data.acceleration.acc_z = az;

			send_data_to_host();
		}
	}
	if (accelerometer_alive && !color_sensor_alive)
	{
		if (atomic_load(&acc_data_ready) == 1)
		{
			atomic_exchange(&acc_data_ready, 0);
			//    		 printf("Acceleration: x = %.2f, y = %.2f, z = %.2f\r", ax, ay, az);
			fflush(stdout);

			send_data.flags = 1; // accelerometer alive && !color sensor alive
			send_data.color = {};
			send_data.acceleration.acc_x = ax;
			send_data.acceleration.acc_y = ay;
			send_data.acceleration.acc_z = az;

			send_data_to_host();
		}
	}
	if (color_sensor_alive && !accelerometer_alive)
	{
		if (atomic_load(&color_sensor_data_ready) == 1)
		{
			atomic_exchange(&color_sensor_data_ready, 0);

			send_data.flags = 2; // !accelerometer alive && color sensor alive
			send_data.color = proc_colors;
			send_data.acceleration.acc_x = 0;
			send_data.acceleration.acc_y = 0;
			send_data.acceleration.acc_z = 0;

			send_data_to_host();
		}
	}
	fflush(stdout);
} // end of while loop
close(sock_fd);
exit(0);
}
