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

struct context_s
{
  unsigned char data[128];
  unsigned int size;
  unsigned int index;
};

int read_data(void *context, unsigned char *payload, unsigned int payload_size)
{
  struct context_s *ctx = (struct context_s *) context;
  unsigned int size = payload_size;

  if(ctx->index == ctx->size)
  {
    return(-1);
  }
  if(ctx->index + size > ctx->size)
  {
    size = ctx->size - ctx->index;
  }
  memcpy(payload, ctx->data + ctx->index, size);
  ctx->index += size;

  return(size);
}

int write_data(void *context, unsigned char *payload, unsigned int payload_size)
{
  struct context_s *ctx = (struct context_s *) context;

  /* Note: The callback of a real application would make sure that it can write
   * all the payload without buffer overflow.
   */
  memcpy(ctx->data + ctx->size, payload, payload_size);
  ctx->size += payload_size;

  return(payload_size);
}

int main()
{
  ofdm_transfer_t send;
  ofdm_transfer_t receive;
  struct context_s context;
  char message[] = "This is a test transmission using ofdm-transfer.";
  char samples_file[] = "/tmp/samples.XXXXXX";
  int samples_fd = mkstemp(samples_file);
  int ok = 0;

  fprintf(stderr, "Test: Send and receive using callbacks\n");

  strcpy(context.data, message);
  context.size = strlen(message);
  context.index = 0;

  if(samples_fd == -1)
  {
    fprintf(stderr, "Error: Failed to create temporary file\n");
    return(EXIT_FAILURE);
  }

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

  send = ofdm_transfer_create_callback("io",
                                       1,
                                       read_data,
                                       &context,
                                       2000000,
                                       9600,
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
                                       NULL,
                                       0);
  if(send == NULL)
  {
    fprintf(stderr, "Error: Failed to initialize transfer\n");
    return(EXIT_FAILURE);
  }
  ofdm_transfer_start(send);
  ofdm_transfer_free(send);

  lseek(samples_fd, 0, SEEK_SET);
  bzero(&context, sizeof(context));
  receive = ofdm_transfer_create_callback("io",
                                          0,
                                          write_data,
                                          &context,
                                          2000000,
                                          9600,
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
                                          NULL,
                                          0);
  if(receive == NULL)
  {
    fprintf(stderr, "Error: Failed to initialize transfer\n");
    return(EXIT_FAILURE);
  }
  ofdm_transfer_start(receive);
  ofdm_transfer_free(receive);

  ok = (strcmp(message, context.data) == 0);
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
