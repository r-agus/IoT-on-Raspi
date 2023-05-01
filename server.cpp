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

#include <cmath>
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

t_rcv_data data[10];
t_rcv_data data_mean, data_max, data_min, data_deviation;

char color_sensor_msg[1500];

void generate_color_sensor_msg(t_rcv_data data);
void print_accel_msg(t_rcv_data data);

void calc_stadistics(t_rcv_data data_raw[]);
t_rcv_data calc_mean(t_rcv_data data_raw[]);
t_rcv_data calc_maximum(t_rcv_data data_raw[]);
t_rcv_data calc_minimum(t_rcv_data data_raw[]);
void calc_deviation(t_rcv_data data_raw[], t_rcv_data mean);

void print_raw_data(t_rcv_data data_raw[10]);
void print_stadistics(t_rcv_data mean, t_rcv_data max, t_rcv_data min, t_rcv_data deviation, t_rcv_data data);

  /*
  This thread is used for terminate the thread
  */
void* terminatorThread(void*)
{
	char a;
	printf("Waiting for a client to extract and represent data \n\r");
	cout<<"Enter any key and press enter to shut down the server: "<<endl;
	printf("\n\r");
	cin>>a;
	close(sock_fd);
    printf("\033[2J");      // Erase the screen
    printf("\033[?25h");    // Make cursor visible
    printf("\033[H");       // Return to (0,0)
	exit(0); //exit the whole server program
}

int main(int argc, char *argv[])
{

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
	
//    printf("\033[2J");      	  // Erase the screen
	printf("\033[?25l");          // Command to hide cursor
	char messageBuffer[sizeof(data[10])];					// Probably should i change the argument of sizeof to data[10]
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
		printf("\033[H");		// Return to (0,0) position
		printf("---------------------------------------------------------------------\n\r");
		
		cout<<"Enter any key and press enter to shut down the server: "<<endl;
		printf("\n\r");

		// Deserialize data
		memcpy(reinterpret_cast<char*>(&data), messageBuffer, sizeof(t_rcv_data));

		cout<<"Data Received from client ("<<inet_ntoa(client.sin_addr)<<"):  "<<messageBuffer<<endl<<endl;
/*
		if(data.flags & 0x03){
			generate_color_sensor_msg(data);
			print_accel_msg(data);
			printf("\n");
			printf("%s\n", color_sensor_msg);
			//printf("\033[3F");
			printf("\n\n---------------------------------------------------------------------");
		}
		else if(data.flags & 0x01){
			print_accel_msg(data);
			printf("\n");
			printf("\033[3F");		// Move the cursor at the beginning of next line, 4 lines down
		}
		else if(data.flags & 0x02){
			printf("\033[1F");		// Move the cursor at the beginning of next line, 2 line down
			printf("\n");		
			generate_color_sensor_msg(data);
			printf("%s", color_sensor_msg);
		}
*/

		print_raw_data(data);

		 calc_stadistics(data);
		 //print_stadistics(data_mean, data_max, data_min, data_deviation, data[9]);
		fflush(stdout);

		
		
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
	printf("Acceleration: x = %.2f, y = %.2f, z = %.2f\n\r", data.acceleration.acc_x, data.acceleration.acc_y, data.acceleration.acc_z);
}

/*
		To be tested
*/
void calc_stadistics(t_rcv_data data_raw[]){
	data_mean = calc_mean(data_raw);
	data_max = calc_maximum(data_raw);
	data_min = calc_minimum(data_raw);
	calc_deviation(data_raw, data_mean);
}

t_rcv_data calc_mean(t_rcv_data data_raw[]){
	t_rcv_data sum;
	for(int i = 0; i < 10; i++){
		sum.acceleration.acc_x = sum.acceleration.acc_x + data_raw[i].acceleration.acc_x;
		sum.acceleration.acc_y = sum.acceleration.acc_y + data_raw[i].acceleration.acc_y;
		sum.acceleration.acc_z = sum.acceleration.acc_z + data_raw[i].acceleration.acc_z;

		sum.color.ir 	= sum.color.ir + data_raw[i].color.ir;
		sum.color.clear = sum.color.clear + data_raw[i].color.clear;
		sum.color.red 	= sum.color.red + data_raw[i].color.red;
		sum.color.green = sum.color.green + data_raw[i].color.green;
		sum.color.blue 	= sum.color.blue + data_raw[i].color.blue;

	}
	sum.acceleration.acc_x = sum.acceleration.acc_x/10;
	sum.acceleration.acc_y = sum.acceleration.acc_y/10;
	sum.acceleration.acc_z = sum.acceleration.acc_z/10;

	sum.color.ir 	= sum.color.ir/10;
	sum.color.clear = sum.color.clear/10;
	sum.color.red 	= sum.color.red/10;
	sum.color.green = sum.color.green/10;
	sum.color.blue 	= sum.color.blue/10;
	return sum;
}

t_rcv_data calc_maximum(t_rcv_data data_raw[]){
	t_rcv_data max = data_raw[0];
	for(int i = 1; i < 0; i++){
		max.acceleration.acc_x = (max.acceleration.acc_x > data_raw[i].acceleration.acc_x) ? max.acceleration.acc_x : data_raw[i].acceleration.acc_x;
		max.acceleration.acc_y = (max.acceleration.acc_y > data_raw[i].acceleration.acc_y) ? max.acceleration.acc_y : data_raw[i].acceleration.acc_y;
		max.acceleration.acc_z = (max.acceleration.acc_z > data_raw[i].acceleration.acc_z) ? max.acceleration.acc_z : data_raw[i].acceleration.acc_z;

		max.color.ir 	= (max.color.ir 	> data_raw[i].color.ir		) 	? max.color.ir 		: data_raw[i].color.ir;
		max.color.clear = (max.color.clear 	> data_raw[i].color.clear	) 	? max.color.clear 	: data_raw[i].color.clear;
		max.color.red 	= (max.color.red 	> data_raw[i].color.red		) 	? max.color.red 	: data_raw[i].color.red;
		max.color.green = (max.color.green 	> data_raw[i].color.green	) 	? max.color.green 	: data_raw[i].color.green;
		max.color.blue 	= (max.color.blue 	> data_raw[i].color.blue	) 	? max.color.blue 	: data_raw[i].color.blue;
	}
	return max;
}
t_rcv_data calc_minimum(t_rcv_data data_raw[]){
	t_rcv_data min = data_raw[0];
	for(int i = 1; i < 0; i++){
		min.acceleration.acc_x = (min.acceleration.acc_x > data_raw[i].acceleration.acc_x) ? min.acceleration.acc_x : data_raw[i].acceleration.acc_x;
		min.acceleration.acc_y = (min.acceleration.acc_y > data_raw[i].acceleration.acc_y) ? min.acceleration.acc_y : data_raw[i].acceleration.acc_y;
		min.acceleration.acc_z = (min.acceleration.acc_z > data_raw[i].acceleration.acc_z) ? min.acceleration.acc_z : data_raw[i].acceleration.acc_z;

		min.color.ir 	= (min.color.ir 	> data_raw[i].color.ir		) 	? min.color.ir 		: data_raw[i].color.ir;
		min.color.clear = (min.color.clear 	> data_raw[i].color.clear	) 	? min.color.clear 	: data_raw[i].color.clear;
		min.color.red 	= (min.color.red 	> data_raw[i].color.red		) 	? min.color.red 	: data_raw[i].color.red;
		min.color.green = (min.color.green 	> data_raw[i].color.green	) 	? min.color.green 	: data_raw[i].color.green;
		min.color.blue 	= (min.color.blue 	> data_raw[i].color.blue	) 	? min.color.blue 	: data_raw[i].color.blue;
	}
	return min;
}
void calc_deviation(t_rcv_data data_raw[], t_rcv_data mean){

	for( int i = 0; i < 10; i++) {
		data_deviation.acceleration.acc_x += pow(data_raw[i].acceleration.acc_x - mean.acceleration.acc_x, 2);
		data_deviation.acceleration.acc_y += pow(data_raw[i].acceleration.acc_y - mean.acceleration.acc_y, 2);
		data_deviation.acceleration.acc_z += pow(data_raw[i].acceleration.acc_z - mean.acceleration.acc_z, 2);

		data_deviation.color.ir 	+= pow(data_raw[i].color.ir 	- mean.color.ir, 	2);
		data_deviation.color.clear 	+= pow(data_raw[i].color.clear 	- mean.color.clear, 2);
		data_deviation.color.red 	+= pow(data_raw[i].color.red 	- mean.color.red, 	2);
		data_deviation.color.green 	+= pow(data_raw[i].color.green 	- mean.color.green, 2);
		data_deviation.color.blue 	+= pow(data_raw[i].color.blue 	- mean.color.blue, 	2);
	}
	data_deviation.acceleration.acc_x = sqrt(data_deviation.acceleration.acc_x / 10);
	data_deviation.acceleration.acc_y = sqrt(data_deviation.acceleration.acc_y / 10);
	data_deviation.acceleration.acc_z = sqrt(data_deviation.acceleration.acc_z / 10);

	data_deviation.color.ir = sqrt(data_deviation.color.ir / 10);
	data_deviation.color.clear = sqrt(data_deviation.color.clear / 10);
	data_deviation.color.red = sqrt(data_deviation.color.red / 10);
	data_deviation.color.green = sqrt(data_deviation.color.green / 10);
	data_deviation.color.blue = sqrt(data_deviation.color.blue / 10);
}

void print_stadistics(t_rcv_data mean, t_rcv_data max, t_rcv_data min, t_rcv_data deviation,  t_rcv_data data){
	/*  Mean messages  */
	printf("Mean\n\r");
	if(data.flags & 0x03){
		printf("Acceleration: \n\r");
		printf("X: %.2f / Y: %.2f / Z: %.2f\n\r", mean.acceleration.acc_x, mean.acceleration.acc_y, mean.acceleration.acc_z);

		printf("Colors: \033[?25l \n\r");
		printf("\033[38;2;255;0;0mR: %.0f  \n\r", mean.color.red);
		printf("\033[38;2;0;255;0mG: %.0f  \n\r", mean.color.green);
		printf("\033[38;2;0;0;255mB: %.0f  \n\r", mean.color.blue);
		printf("\033[38;2;255;255;255m");

		printf("\n---------------------------------------------------------------------\n");
		/*  Maximum messages  */
		printf("Maximum\n\r");
		printf("X: %.2f / Y: %.2f / Z: %.2f\n\r", max.acceleration.acc_x, max.acceleration.acc_y, max.acceleration.acc_z);

		printf("Colors: \033[?25l \n\r");
		printf("\033[38;2;255;0;0mR: %.0f  \n\r", max.color.red);
		printf("\033[38;2;0;255;0mG: %.0f  \n\r", max.color.green);
		printf("\033[38;2;0;0;255mB: %.0f  \n\r", max.color.blue);
		printf("\033[38;2;255;255;255m");

		printf("\n---------------------------------------------------------------------\n");
		/*  Minimum messages  */
		printf("Minimum\n\r");
		printf("X: %.2f / Y: %.2f / Z: %.2f\n\r", min.acceleration.acc_x, min.acceleration.acc_y, min.acceleration.acc_z);

		printf("Colors: \033[?25l \n\r");
		printf("\033[38;2;255;0;0mR: %.0f  \n\r", min.color.red);
		printf("\033[38;2;0;255;0mG: %.0f  \n\r", min.color.green);
		printf("\033[38;2;0;0;255mB: %.0f  \n\r", min.color.blue);
		printf("\033[38;2;255;255;255m");

		printf("\n---------------------------------------------------------------------\n");
		/*  Stantard Deviation messages  */
		printf("Standard Deviation \n\r");
		printf("X: %.2f / Y: %.2f / Z: %.2f\n\r", deviation.acceleration.acc_x, deviation.acceleration.acc_y, deviation.acceleration.acc_z);

		printf("Colors: \033[?25l \n\r");
		printf("\033[38;2;255;0;0mR: %.0f  \n\r", deviation.color.red);
		printf("\033[38;2;0;255;0mG: %.0f  \n\r", deviation.color.green);
		printf("\033[38;2;0;0;255mB: %.0f  \n\r", deviation.color.blue);
		printf("\033[38;2;255;255;255m");

		printf("\n---------------------------------------------------------------------\n");
	}
	
	else if(data.flags & 0x02){
		printf("Colors: \033[?25l \n\r");
		printf("\033[38;2;255;0;0mR: %.0f  \n\r", mean.color.red);
		printf("\033[38;2;0;255;0mG: %.0f  \n\r", mean.color.green);
		printf("\033[38;2;0;0;255mB: %.0f  \n\r", mean.color.blue);
		printf("\033[38;2;255;255;255m");

		printf("\n---------------------------------------------------------------------\n");
		/*  Maximum messages  */
		printf("Colors: \033[?25l \n\r");
		printf("\033[38;2;255;0;0mR: %.0f  \n\r", max.color.red);
		printf("\033[38;2;0;255;0mG: %.0f  \n\r", max.color.green);
		printf("\033[38;2;0;0;255mB: %.0f  \n\r", max.color.blue);
		printf("\033[38;2;255;255;255m");

		printf("\n---------------------------------------------------------------------\n");
		/*  Minimum messages  */
		printf("Colors: \033[?25l \n\r");
		printf("\033[38;2;255;0;0mR: %.0f  \n\r", min.color.red);
		printf("\033[38;2;0;255;0mG: %.0f  \n\r", min.color.green);
		printf("\033[38;2;0;0;255mB: %.0f  \n\r", min.color.blue);
		printf("\033[38;2;255;255;255m");

		printf("\n---------------------------------------------------------------------\n");
		/*  Stantard Deviation messages  */
		printf("Colors: \033[?25l \n\r");
		printf("\033[38;2;255;0;0mR: %.0f  \n\r", deviation.color.red);
		printf("\033[38;2;0;255;0mG: %.0f  \n\r", deviation.color.green);
		printf("\033[38;2;0;0;255mB: %.0f  \n\r", deviation.color.blue);
		printf("\033[38;2;255;255;255m");

		printf("\n---------------------------------------------------------------------\n");
	}
	
	else if(data.flags & 0x01){
		printf("Acceleration: \n\r");
		printf("X: %.2f / Y: %.2f / Z: %.2f\n\r", mean.acceleration.acc_x, mean.acceleration.acc_y, mean.acceleration.acc_z);

		printf("\n---------------------------------------------------------------------\n");
		/*  Maximum messages  */
		printf("Maximum\n\r");
		printf("X: %.2f / Y: %.2f / Z: %.2f\n\r", max.acceleration.acc_x, max.acceleration.acc_y, max.acceleration.acc_z);

		printf("\n---------------------------------------------------------------------\n");
		/*  Minimum messages  */
		printf("Minimum\n\r");
		printf("X: %.2f / Y: %.2f / Z: %.2f\n\r", min.acceleration.acc_x, min.acceleration.acc_y, min.acceleration.acc_z);

		printf("\n---------------------------------------------------------------------\n");
		/*  Stantard Deviation messages  */
		printf("Standard Deviation \n\r");
		printf("X: %.2f / Y: %.2f / Z: %.2f\n\r", deviation.acceleration.acc_x, deviation.acceleration.acc_y, deviation.acceleration.acc_z);

		printf("\n---------------------------------------------------------------------\n");
	}
}

void print_raw_data(t_rcv_data data_raw[]){
	
	for(int i = 0; i < 10; i++){
		printf("Acceleration: \n\r");
		printf("X: %.2f / Y: %.2f / Z: %.2f\n\r", data_raw[i].acceleration.acc_x, data_raw[i].acceleration.acc_y, data_raw[i].acceleration.acc_z);

		printf("Colors: \033[?25l \n\r");
		printf("\033[38;2;255;0;0mR: %.0f  \n\r", data_raw[i].color.red);
		printf("\033[38;2;0;255;0mG: %.0f  \n\r", data_raw[i].color.green);
		printf("\033[38;2;0;0;255mB: %.0f  \n\r", data_raw[i].color.blue);
		printf("\033[38;2;255;255;255m");

		printf("\n---------------------------------------------------------------------\n");
	}
	
}
