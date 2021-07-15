/*
This file is part of ofdm-transfer, a program to send or receive data
by software defined radio using the OFDM modulation.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ofdm-transfer.h"

int identical(char *message_file, char *decoded_file)
{
  FILE *message;
  FILE *decoded;
  unsigned int i1;
  unsigned int i2;
  unsigned char buffer1[1024];
  unsigned char buffer2[1024];

  if((message = fopen(message_file, "rb")) == NULL)
  {
    fprintf(stderr, "Error: Failed to open '%s'\n", message_file);
    return(0);
  }
  if((decoded = fopen(decoded_file, "rb")) == NULL)
  {
    fprintf(stderr, "Error: Failed to open '%s'\n", decoded_file);
    fclose(message);
    return(0);
  }

  while(1)
  {
    i1 = fread(buffer1, 1, 1024, message);
    i2 = fread(buffer2, 1, 1024, decoded);
    if((i1 == 0) && (i2 == 0))
    {
      break;
    }
    if((i1 != i2) || (memcmp(buffer1, buffer2, i1) != 0))
    {
      fclose(message);
      fclose(decoded);
      return(0);
    }
  }

  return(1);
}

int main()
{
  ofdm_transfer_t send;
  ofdm_transfer_t receive;
  char message[] = "This is a test transmission using ofdm-transfer.";
  char message_file[] = "/tmp/message.XXXXXX";
  int message_fd = mkstemp(message_file);
  char decoded_file[] = "/tmp/decoded.XXXXXX";
  int decoded_fd = mkstemp(decoded_file);
  char samples_file[] = "/tmp/samples.XXXXXX";
  int samples_fd = mkstemp(samples_file);
  int ok = 0;

  fprintf(stderr, "Test: Send and receive file\n");

  if((message_fd == -1) || (decoded_fd == -1) || (samples_fd == -1))
  {
    fprintf(stderr, "Error: Failed to create temporary files\n");
    return(EXIT_FAILURE);
  }
  write(message_fd, message, strlen(message));
  close(message_fd);
  close(decoded_fd);

  if(dup2(samples_fd, STDIN_FILENO) == -1)
  {
    fprintf(stderr, "Error: Failed to redirect standard input\n");
    return(EXIT_FAILURE);
  }
  if(dup2(samples_fd, STDOUT_FILENO) == -1)
  {
    fprintf(stderr, "Error: Failed to redirect standard output\n");
    return(EXIT_FAILURE);
  }

  send = ofdm_transfer_create("io",
                              1,
                              message_file,
                              2000000,
                              38400,
                              434000000,
                              0,
                              0,
                              0,
                              "qpsk",
                              64,
                              16,
                              4,
                              "h128",
                              "none",
                              "",
                              NULL);
  if(send == NULL)
  {
    fprintf(stderr, "Error: Failed to initialize transfer\n");
    return(EXIT_FAILURE);
  }
  ofdm_transfer_start(send);
  ofdm_transfer_free(send);

  lseek(samples_fd, 0, SEEK_SET);
  receive = ofdm_transfer_create("io",
                                 0,
                                 decoded_file,
                                 2000000,
                                 38400,
                                 434000000,
                                 0,
                                 0,
                                 0,
                                 "qpsk",
                                 64,
                                 16,
                                 4,
                                 "h128",
                                 "none",
                                 "",
                                 NULL);
  if(receive == NULL)
  {
    fprintf(stderr, "Error: Failed to initialize transfer\n");
    return(EXIT_FAILURE);
  }
  ofdm_transfer_set_verbose(1);
  ofdm_transfer_start(receive);
  ofdm_transfer_free(receive);

  ok = identical(message_file, decoded_file);
  unlink(message_file);
  unlink(decoded_file);
  close(samples_fd);
  unlink(samples_file);

  if(ok)
  {
    return(EXIT_SUCCESS);
  }
  else
  {
    return(EXIT_FAILURE);
  }
}
