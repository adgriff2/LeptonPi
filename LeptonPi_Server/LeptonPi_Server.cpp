#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <iostream>

using namespace std;

void error(char *msg) {
	perror(msg);
	exit(1);
}

static uint8_t bits = 8;
static uint32_t speed = 16000000;
static uint16_t delay;
static const unsigned int height = 60, width = 80, bytes_per_pixel = 2, bytes_per_header = 4;
static const unsigned int VOSPI_PACKET_SIZE = bytes_per_header + width * bytes_per_pixel;
static uint8_t frame[VOSPI_PACKET_SIZE*height];

void *handleLepton(void *threadid)
{
	int fd;
	char *device = new char[20];
	strcpy(device, "/dev/spidev0.0");
	if ((fd = open(device, O_RDWR)) < 0)
	{
		cout << "ERROR on device open" << endl;
	}
	unsigned int i, j;
	uint8_t packet[VOSPI_PACKET_SIZE];
	uint8_t tx[VOSPI_PACKET_SIZE];
	memset(tx, 0, VOSPI_PACKET_SIZE * sizeof(uint8_t));
	struct spi_ioc_transfer tr;
	tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)packet;
	tr.len = VOSPI_PACKET_SIZE;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;
	while (1)
	{
		packet[1] = 60;
		while (packet[0] == 0xff || packet[1] > 59 || packet[2] == 0 || packet[3] == 0)
		{
			ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
		}

		for (i = 0; i < height; i++)
		{
			for (j = 0; j < VOSPI_PACKET_SIZE; j++)
			{
				frame[j + i * VOSPI_PACKET_SIZE] = packet[j];
			}
			ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
		}
	}
}

int main(int argc, char *argv[]) {

	pthread_t Lepton_thread;
	pthread_create(&Lepton_thread, NULL, handleLepton, (void *)1);

	int sockfd, newsockfd, portno = 5000, clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	printf("using port #%d\n", portno);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		error(const_cast<char *>("ERROR opening socket"));
	}
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		error(const_cast<char *>("ERROR on binding"));
	}
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	while (1) {
		printf("waiting for new client...\n");
		if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&clilen)) < 0)
		{
			error(const_cast<char *>("ERROR on accept"));
		}
		printf("opened new communication with client\n");
		n = 1;
		while (n > 0) {
			n = write(newsockfd, frame, VOSPI_PACKET_SIZE*height);
			usleep(111000);
		}
		printf("closing Newsock\n");
		fflush(stdout);
		close(newsockfd);
	}
	return 0;
}