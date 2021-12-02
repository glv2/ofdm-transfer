/*
Example of use of ofdm-transfer's API to make a server receiving
messages from clients and sending them back in reverse order.

Copyright 2021 Guillaume LE VAILLANT

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ofdm-transfer.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RADIO_DRIVER "driver=hackrf"
#define SAMPLE_RATE 4000000
#define TRANSMISSION_GAIN 36
#define RECEPTION_GAIN 60
#define FREQUENCY_OFFSET 100000
#define BIT_RATE 9600
#define SUBCARRIER_MODULATION "qpsk"
#define SUBCARRIERS 64
#define CYCLIC_PREFIX_LENGTH 16
#define TAPER_LENGTH 4
#define INNER_FEC "rs8"
#define OUTER_FEC "rs8"

struct message_s
{
  unsigned char *data;
  unsigned int size;
  unsigned int done;
};

unsigned char stop_loop = 0;

int transmission_callback(void *context,
                          unsigned char *payload,
                          unsigned int payload_size)
{
  struct message_s *message = (struct message_s *) context;
  unsigned int size = message->size - message->done;

  if(size == 0)
  {
    return(-1);
  }
  size = (payload_size < size) ? payload_size : size;
  memcpy(payload, &message->data[message->done], size);
  message->done += size;
  return(size);
}

void transmit(unsigned char *data,
              unsigned int size,
              unsigned long int frequency)
{
  struct message_s message = {data, size, 0};
  ofdm_transfer_t transfer = ofdm_transfer_create_callback(RADIO_DRIVER,
                                                           1,
                                                           transmission_callback,
                                                           (void *) &message,
                                                           SAMPLE_RATE,
                                                           BIT_RATE,
                                                           frequency,
                                                           FREQUENCY_OFFSET,
                                                           TRANSMISSION_GAIN,
                                                           0,
                                                           SUBCARRIER_MODULATION,
                                                           SUBCARRIERS,
                                                           CYCLIC_PREFIX_LENGTH,
                                                           TAPER_LENGTH,
                                                           INNER_FEC,
                                                           OUTER_FEC,
                                                           "",
                                                           NULL,
                                                           0);
  if(transfer == NULL)
  {
    return;
  }
  ofdm_transfer_start(transfer);
  sleep(1); /* Give time to the hackrf to send the last samples */
  ofdm_transfer_free(transfer);
}

int reception_callback(void *context,
                       unsigned char *payload,
                       unsigned int payload_size)
{
  struct message_s *message = (struct message_s *) context;
  unsigned int size = message->size;

  size = (payload_size < size) ? payload_size : size;
  memcpy(message->data, payload, size);
  message->done = size;
  ofdm_transfer_stop_all();
  return(payload_size);
}

void receive_1(unsigned char *data,
               unsigned int *size,
               unsigned long int frequency)
{
  struct message_s message = {data, *size, 0};
  ofdm_transfer_t transfer = ofdm_transfer_create_callback(RADIO_DRIVER,
                                                           0,
                                                           reception_callback,
                                                           (void *) &message,
                                                           SAMPLE_RATE,
                                                           BIT_RATE,
                                                           frequency,
                                                           FREQUENCY_OFFSET,
                                                           RECEPTION_GAIN,
                                                           0,
                                                           SUBCARRIER_MODULATION,
                                                           SUBCARRIERS,
                                                           CYCLIC_PREFIX_LENGTH,
                                                           TAPER_LENGTH,
                                                           INNER_FEC,
                                                           OUTER_FEC,
                                                           "",
                                                           NULL,
                                                           0);
  if(transfer == NULL)
  {
    return;
  }
  ofdm_transfer_start(transfer);
  ofdm_transfer_free(transfer);
  *size = message.done;
}

void process_request(unsigned char *data, unsigned int size)
{
  unsigned int i;
  unsigned char c;

  for(i = 0; i < size / 2; i++)
  {
    c = data[i];
    data[i] = data[size - 1 - i];
    data[size - 1 - i] = c;
  }
}

void server(unsigned long int frequency)
{
  unsigned char data[1024];
  unsigned int size;

  while(!stop_loop)
  {
    size = sizeof(data) - 1;
    receive_1(data, &size, frequency);
    if(stop_loop)
    {
      return;
    }
    data[size] = '\0';
    printf("\nReceived: %s\n", data);
    process_request(data, size);
    printf("Sending: %s\n", data);
    sleep(1);
    if(stop_loop)
    {
      return;
    }
    transmit(data, size, frequency);
  }
}

void client(unsigned char *data, unsigned int size, unsigned long int frequency)
{
  unsigned char buffer[1024];
  unsigned int n = sizeof(buffer) - 1;

  printf("\nSending: %s\n", data);
  transmit(data, size, frequency);
  receive_1(buffer, &n, frequency);
  buffer[n] = '\0';
  printf("Received: %s\n", buffer);
}

void signal_handler(int signum)
{
  fprintf(stderr, "\n");
  ofdm_transfer_stop_all();
  stop_loop = 1;
}

void usage()
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  echo-server server <frequency>\n");
  fprintf(stderr, "  echo-server client <frequency> <message>\n");
}

int main(int argc, char **argv)
{
  unsigned long int frequency;
  unsigned char *mode;
  unsigned char *data;

  if((argc != 3) && (argc != 4))
  {
    usage();
    return(-1);
  }

  mode = argv[1];
  frequency = strtoul(argv[2], NULL, 10);
  stop_loop = 0;
  signal(SIGINT, &signal_handler);
  signal(SIGTERM, &signal_handler);
  signal(SIGABRT, &signal_handler);

  if(strcmp(mode, "client") == 0)
  {
    if(argc !=4)
    {
      usage();
      return(-1);
    }
    data = argv[3];
    client(data, strlen(data), frequency);
  }
  else if(strcmp(mode, "server") == 0)
  {
    server(frequency);
  }
  else
  {
    usage();
    return(-1);
  }
  return(1);
}
