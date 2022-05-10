/*
Example of use of ofdm-transfer's API to make a duplex link.

Copyright 2022 Guillaume LE VAILLANT

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
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DOWNLINK_RADIO "driver=rtlsdr"
#define DOWNLINK_SAMPLE_RATE 250000
#define DOWNLINK_GAIN "30"
#define DOWNLINK_FREQUENCY_OFFSET 100000
#define UPLINK_RADIO "driver=hackrf"
#define UPLINK_SAMPLE_RATE 4000000
#define UPLINK_GAIN "36"
#define UPLINK_FREQUENCY_OFFSET 100000
#define BIT_RATE 38400
#define SUBCARRIER_MODULATION "bpsk"
#define SUBCARRIERS 64
#define CYCLIC_PREFIX_LENGTH 16
#define TAPER_LENGTH 4
#define INNER_FEC "none"
#define OUTER_FEC "secded3932"

void usage()
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  duplex <downlink frequency> <uplink frequency>\n");
}

void signal_handler(int signum)
{
  ofdm_transfer_stop_all();
}

void * transfer_thread(void *arg)
{
  ofdm_transfer_t *transfer = (ofdm_transfer_t *) arg;

  ofdm_transfer_start(*transfer);

  return(NULL);
}

int main(int argc, char **argv)
{
  unsigned long int downlink_frequency;
  ofdm_transfer_t downlink;
  pthread_t downlink_thread;
  unsigned long int uplink_frequency;
  ofdm_transfer_t uplink;
  pthread_t uplink_thread;

  if(argc != 3)
  {
    usage();
    return(EXIT_FAILURE);
  }

  downlink_frequency = strtoul(argv[1], NULL, 10);
  uplink_frequency = strtoul(argv[2], NULL, 10);

  downlink = ofdm_transfer_create(DOWNLINK_RADIO,
                                  0,
                                  NULL,
                                  DOWNLINK_SAMPLE_RATE,
                                  BIT_RATE,
                                  downlink_frequency,
                                  DOWNLINK_FREQUENCY_OFFSET,
                                  DOWNLINK_GAIN,
                                  0,
                                  SUBCARRIER_MODULATION,
                                  SUBCARRIERS,
                                  CYCLIC_PREFIX_LENGTH,
                                  TAPER_LENGTH,
                                  INNER_FEC,
                                  OUTER_FEC,
                                  "",
                                  NULL,
                                  0,
                                  0);
  if(downlink == NULL)
  {
    fprintf(stderr, "Error: Failed to initialize downlink.\n");
    return(EXIT_FAILURE);
  }

  uplink = ofdm_transfer_create(UPLINK_RADIO,
                                1,
                                NULL,
                                UPLINK_SAMPLE_RATE,
                                BIT_RATE,
                                uplink_frequency,
                                UPLINK_FREQUENCY_OFFSET,
                                UPLINK_GAIN,
                                0,
                                SUBCARRIER_MODULATION,
                                SUBCARRIERS,
                                CYCLIC_PREFIX_LENGTH,
                                TAPER_LENGTH,
                                INNER_FEC,
                                OUTER_FEC,
                                "",
                                NULL,
                                0,
                                0);
  if(uplink == NULL)
  {
    fprintf(stderr, "Error: Failed to initialize uplink.\n");
    return(EXIT_FAILURE);
  }

  if(pthread_create(&downlink_thread, NULL, transfer_thread, &downlink) != 0)
  {
    fprintf(stderr, "Error: Failed to start downlink thread.\n");
    return(EXIT_FAILURE);
  }

  /* Some function in the fftw library can cause errors when it is called from
   * several threads at the same time. Waiting 1 second before starting the
   * second thread seems to avoid the issue. */
  sleep(1);

  if(pthread_create(&uplink_thread, NULL, transfer_thread, &uplink) != 0)
  {
    fprintf(stderr, "Error: Failed to start uplink thread.\n");
    ofdm_transfer_stop_all();
    pthread_join(downlink_thread, NULL);
    ofdm_transfer_free(downlink);
    return(EXIT_FAILURE);
  }

  signal(SIGINT, &signal_handler);
  signal(SIGTERM, &signal_handler);
  signal(SIGABRT, &signal_handler);
  fprintf(stderr, "Use CTRL-C to quit.\n");

  pthread_join(uplink_thread, NULL);
  pthread_join(downlink_thread, NULL);
  ofdm_transfer_free(uplink);
  ofdm_transfer_free(downlink);
  fprintf(stderr, "\n");

  return(EXIT_SUCCESS);
}
