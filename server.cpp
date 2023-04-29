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

typedef struct{
	float mean;
	float maximum;
	float minimum;
	float deviation;
}t_staditical;

/*
	This structure is used for represent data of accelerometer and color sensor. 
	It also has some flags used to choose what want to represent
*/
typedef struct{
	uint8_t flags;      // Mask 1: To see the status of accelerometer     Mask 2: To see the status of color sensor
	t_proc_color color;
	t_acc_data acceleration;
}t_rcv_data;

/*
	0: X acceleration 	1: Y acceleration   2: Z acceleration
	3: Infrarred		4: Clear			5: RED
	6: GREEN			7: BLUE
*/
t_staditical result_calculations[8];
char color_sensor_msg[1500];

void generate_color_sensor_msg(t_rcv_data data);
void print_accel_msg(t_rcv_data data);

void calc_stadistics(t_rcv_data data_raw, t_staditical processed_data[]);
float calc_mean(float data_raw[]);			// Change to array
float calc_maximum(float data_raw[]);			// Change to array
float calc_minimum(float data_raw[]);			// Change to array
float calc_deviation(float data_raw[]);		// Change to array

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
	t_rcv_data data;
	t_staditical_data data_processed;
	
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
		printf("\033[2J");
		printf("\033[H");		// Return to (0,0) position
		printf("---------------------------------------------------------------------\n\r");
		
		cout<<"Enter any key and press enter to shut down the server: "<<endl;
		printf("\n\r");

		// Deserialize data
		memcpy(reinterpret_cast<char*>(&data), messageBuffer, sizeof(t_rcv_data));

		cout<<"Data Received from client ("<<inet_ntoa(client.sin_addr)<<"):  "<<messageBuffer<<endl<<endl;
		//printf("\n");
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
		//printf("---------------------------------------------------------------------");
		fflush(stdout);
		//sleep(1);		// Wait 1 second to execute again the loop
		
		
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
void calc_stadistics(t_rcv_data data_raw, t_staditical processed_data[]){			// Change to array

	for( int i = 0; i < 8: i++ ){
		processed_data[i].mean = calc_mean()
		processed_data[i].maximum = calc_maximum()
		processed_data[i].minimum = calc_minimum()
		processed_data[i].deviation = calc_deviation()
	}


]

float calc_mean(float data_raw[]){
	float sum;
	for(int i = 0; i < 10; i++){
		sum = data_raw[i];
	}
	return sum/10;
}

float calc_maximum(float data_raw[]){
	float max = data_raw[0];
	for(int i = 1; i < 0; i++){
		max = (max > data_raw[i]) ? max : data_raw[i];
	}
	return max;
}
float calc_minimum(float data_raw[]){
	float min = data_raw[0];
	for(int i = 1; i < 0; i++){
		min = (min < data_raw[i]) ? min : data_raw[i];
	}
	return min;
}
float calc_deviation(float data_raw[], float mean){
	float deviation;
	
	for(i = 0; i < 10; i++) {
		deviation += pow(data[i] - mean, 2);
	}
	return sqrt(deviation / 10);
}


