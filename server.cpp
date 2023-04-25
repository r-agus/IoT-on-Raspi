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
using namespace std;

#define MAX_STRING_LENGTH 100
int sock_fd;

typedef struct{
	float acc_x, acc_y, acc_z;
}t_acc_data;

typedef struct{
	float ir;
	float clear;
	float red, green, blue;		// RGB values
}t_proc_color;


/*
	This structure is used for represent data of accelerometer and color sensor. 
	It also has some flags used to choose what want to represent
*/
typedef struct{
	uint8_t flags;      // Mask 1: To see the status of accelerometer     Mask 2: To see the status of color sensor
	t_proc_color color;
	t_acc_data acceleration;
}t_rcv_data;

char color_sensor_msg[1500];

void generate_color_sensor_msg(t_rcv_data data);
void print_accel_msg(t_rcv_data data);
  /*
  This thread is used for terminate the thread
  */
void* terminatorThread(void*)
{
	char a;
	cout<<"Enter any key and press enter to shut down the server: "<<endl;
	cin>>a;
	close(sock_fd);
	exit(0); //exit the whole server program
}

int main(int argc, char *argv[])
{
	t_rcv_data data;

	if (argc==1)
	{
		cout<<"Please pass the port number on which you want the server to listen."<<endl;
		exit(-1);
	}

	int tid=0;
	
	/*
	Here we create a thread to delete the main thread
	*/
	if (pthread_create((pthread_t*) &tid, NULL, terminatorThread, NULL)==-1)
	{
		perror("pthread_create: ");
		exit(-1);
	}

	sockaddr_in server, client;
	
	/*
	Here we create a socket which is used to connect with RPI
	*/
	if ((sock_fd= socket(AF_INET, SOCK_DGRAM, 0))==-1)
	{
		perror("socket: ");
		exit(-1);
	}

	server.sin_family= AF_INET;
	server.sin_port= htons(atoi(argv[1]));
	server.sin_addr.s_addr= INADDR_ANY;
	bzero(&server.sin_zero, 8);

	/*
	Here we bind a socket to Server Address
	*/
	if ((bind(sock_fd, reinterpret_cast<struct sockaddr*>(&server), sizeof(server)))==-1)
	{
		perror("bind:");
		exit(-1);
	}

	char messageBuffer[sizeof(t_rcv_data)];
	/*
	This loop is used to recieve data and represent them on the terminal
	*/
	while (true)
	{
		int len=sizeof(client);

		// Receive serialized data
		if (recvfrom(sock_fd, messageBuffer, sizeof(messageBuffer), 0, reinterpret_cast<struct sockaddr*>(&client), (socklen_t*) &len)==-1)
		{
			perror("recvfrom:");
			continue;
		}

		// Deserialize data
		memcpy(reinterpret_cast<char*>(&data), messageBuffer, sizeof(t_rcv_data));

		cout<<"Data Received from client ("<<inet_ntoa(client.sin_addr)<<"):  "<<messageBuffer<<endl<<endl;

		if(data.flags & 0x03){
			generate_color_sensor_msg(data);
			print_accel_msg(data);
			printf("%s", color_sensor_msg);
			fflush(stdout);
		}
		else if(data.flags & 0x01){
			print_accel_msg(data);
			fflush(stdout);
		}
		else if(data.flags & 0x02){
			generate_color_sensor_msg(data);
			printf("%s", color_sensor_msg);
			fflush(stdout);
		}

//		if (sendto(sock_fd, (void*) messageBuffer, (size_t) strlen(messageBuffer)+1, 0, (sockaddr*) &client, (socklen_t) len)==-1)
//		{
//			perror("sendto: ");
//			continue;
//		}

	}//end of while loop

}

void generate_color_sensor_msg(t_rcv_data data){
	char tmp[100] = "";
	sprintf(color_sensor_msg, "\033[?25l");
	sprintf(tmp, "\033[38;2;255;0;0mR: %.0f        \r", data.color.red);
	(data.flags & 0x4) ? strcat(color_sensor_msg, "\033[38;2;255;0;0mIR > RED      \r")
			: strcat(color_sensor_msg, tmp);  // Set foreground color to red and write red value
	*tmp = 0;
	sprintf(tmp, "\n\033[38;2;0;255;0mG: %.0f      \r", data.color.green);
	(data.flags & 0x8) ? strcat(color_sensor_msg, "\n\033[38;2;0;255;0mIR > GREEN  \r")
			: strcat(color_sensor_msg, tmp);  // Set foreground color to green and write green value
	*tmp = 0;
	sprintf(tmp, "\n\033[38;2;0;0;255mB: %.0f      \r\033[2A\033[38;2;255;255;255m", data.color.blue);
	(data.flags & 0x10) ? strcat(color_sensor_msg, "\n\033[38;2;0;0;255mIR > BLUE   \r\033[2A\033[38;2;255;255;255m")
			: strcat(color_sensor_msg, tmp);  // Set foreground color to blue and write blue value

}

void print_accel_msg(t_rcv_data data){
	printf("Acceleration: x = %.2f, y = %.2f, z = %.2f\r", data.acceleration.acc_x, data.acceleration.acc_y, data.acceleration.acc_z);
}
