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

#include <complex.h>
#include <fcntl.h>
#include <liquid/liquid.h>
#include <math.h>
#include <signal.h>
#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define TAU (2 * M_PI)

#define SOAPYSDR_CHECK(funcall) \
{ \
  int e = funcall; \
  if(e != 0) \
  { \
    fprintf(stderr, "Error: %s\n", SoapySDRDevice_lastError()); \
    exit(-1); \
  } \
}

typedef enum
  {
    IO,
    SOAPYSDR
  } radio_type_t;

typedef union
{
  SoapySDRDevice *soapysdr;
} radio_device_t;

typedef struct
{
  radio_type_t type;
  radio_device_t device;
  char id[5];
  unsigned long int frequency;
  unsigned long int center_frequency;
  SoapySDRStream *stream;
} radio_t;

FILE *file = NULL;
unsigned char stop = 0;
unsigned char verbose = 0;

FILE *dump = NULL;

void dump_samples(complex float *samples, unsigned int samples_size)
{
  fwrite(samples, sizeof(complex float), samples_size, dump);
}

int read_data(unsigned char *payload, unsigned int payload_size)
{
  int n;

  if(feof(file))
  {
    return(-1);
  }

  n = fread(payload, 1, payload_size, file);
  if(n == 0)
  {
    usleep(1);
  }

  return(n);
}

void write_data(unsigned char *payload, unsigned int payload_size)
{
  fwrite(payload, 1, payload_size, file);
  if(file == stdout)
  {
    fflush(file);
  }
}

void send_to_radio(radio_t *radio, complex float *samples,
                   unsigned int samples_size, int last)
{
  unsigned int n;
  unsigned int size;
  int flags = 0;
  size_t mask = 0;
  long long int timestamp = 0;
  int r;
  const void *buffers[1];

  if(dump)
  {
    dump_samples(samples, samples_size);
  }

  switch(radio->type)
  {
  case IO:
    fwrite(samples, sizeof(complex float), samples_size, stdout);
    break;

  case SOAPYSDR:
    n = 0;
    while((n < samples_size) && (!stop))
    {
      buffers[0] = &samples[n];
      size = samples_size - n;
      r = SoapySDRDevice_writeStream(radio->device.soapysdr,
                                     radio->stream,
                                     buffers,
                                     size,
                                     &flags,
                                     0,
                                     10000);
      if(r > 0)
      {
        n += r;
      }
    }
    if(last)
    {
      /* Complete the remaining buffer to ensure that SoapySDR will process it */
      size = SoapySDRDevice_getStreamMTU(radio->device.soapysdr, radio->stream);
      bzero(samples, samples_size * sizeof(complex float));
      buffers[0] = samples;
      while((size > 0) && (!stop))
      {
        n = (samples_size < size) ? samples_size : size;
        r = SoapySDRDevice_writeStream(radio->device.soapysdr,
                                       radio->stream,
                                       buffers,
                                       n,
                                       &flags,
                                       0,
                                       10000);
        if(r > 0)
        {
          size -= r;
        }
      }
      flags = SOAPY_SDR_END_BURST;
      do
      {
        if(stop)
        {
          break;
        }
        r = SoapySDRDevice_readStreamStatus(radio->device.soapysdr,
                                            radio->stream,
                                            &mask,
                                            &flags,
                                            &timestamp,
                                            10000);
      }
      while((r != SOAPY_SDR_UNDERFLOW) && (!stop));

      /* Give enough time to the hardware to send the last samples */
      usleep(1000000);
    }
    break;
  }
}

unsigned int receive_from_radio(radio_t *radio, complex float *samples,
                                unsigned int samples_size)
{
  unsigned int n = 0;
  int flags;
  long long int timestamp;
  int r;
  void *buffers[1];

  switch(radio->type)
  {
  case IO:
    n = fread(samples, sizeof(complex float), samples_size, stdin);
    break;

  case SOAPYSDR:
    buffers[0] = samples;
    r = SoapySDRDevice_readStream(radio->device.soapysdr,
                                  radio->stream,
                                  buffers,
                                  samples_size,
                                  &flags,
                                  &timestamp,
                                  10000);
    if(r >= 0)
    {
      n = r;
    }
    break;
  }
  return(n);
}

void set_counter(unsigned char *header, unsigned int counter)
{
  header[4] = (counter >> 24) & 255;
  header[5] = (counter >> 16) & 255;
  header[6] = (counter >> 8) & 255;
  header[7] = counter & 255;
}

unsigned int get_counter(unsigned char *header)
{
  return((header[4] << 24) | (header[5] << 16) | (header[6] << 8) | header[7]);
}

void send_dummy_samples(radio_t *radio, msresamp_crcf resampler, nco_crcf oscillator,
                        complex float *frame_samples, unsigned int frame_samples_size,
                        complex float *samples, float frequency_offset, int last)
{
  unsigned int n;

  for(n = 0; n < frame_samples_size; n++)
  {
    frame_samples[n] = 0;
  }
  msresamp_crcf_execute(resampler, frame_samples, frame_samples_size, samples, &n);
  if(frequency_offset != 0)
  {
    nco_crcf_mix_block_up(oscillator, samples, samples, n);
  }
  send_to_radio(radio, samples, n, last);
}

void send_frames(radio_t *radio, float sample_rate, unsigned int bit_rate,
                 crc_scheme crc, fec_scheme inner_fec, fec_scheme outer_fec)
{
  unsigned int subcarriers = 64;
  unsigned int cyclic_prefix_size = 16;
  unsigned int taper_size = 4;
  modulation_scheme subcarrier_modulation = LIQUID_MODEM_QPSK;
  unsigned int subcarrier_symbol_bits = 2;
  float samples_per_bit = 2.0 / subcarrier_symbol_bits;
  ofdmflexframegenprops_s frame_properties;
  ofdmflexframegen frame_generator;
  float resampling_ratio = sample_rate / (bit_rate * samples_per_bit);
  msresamp_crcf resampler = msresamp_crcf_create(resampling_ratio, 60);
  unsigned int delay = (unsigned int) ceilf(msresamp_crcf_get_delay(resampler));
  unsigned int header_size = 8;
  unsigned char header[header_size];
  unsigned int payload_size = 1000;
  unsigned char payload[payload_size];
  int r;
  unsigned int n;
  unsigned int frame_samples_size = (bit_rate * samples_per_bit) / 20; /* 50 ms */
  unsigned int samples_size = ceilf((frame_samples_size + delay) * resampling_ratio);
  int frame_complete;
  float frequency_offset = (float) radio->frequency - radio->center_frequency;
  float center_frequency = frequency_offset / sample_rate;
  nco_crcf oscillator = nco_crcf_create(LIQUID_NCO);
  unsigned int counter = 0;
  complex float *frame_samples = malloc(frame_samples_size * sizeof(complex float));
  complex float *samples = malloc(samples_size * sizeof(complex float));

  if((frame_samples == NULL) || (samples == NULL))
  {
    fprintf(stderr, "Error: Memory allocation failed\n");
    exit(-1);
  }

  nco_crcf_set_phase(oscillator, 0);
  nco_crcf_set_frequency(oscillator, TAU * center_frequency);

  ofdmflexframegenprops_init_default(&frame_properties);
  frame_properties.check = crc;
  frame_properties.fec0 = inner_fec;
  frame_properties.fec1 = outer_fec;
  frame_properties.mod_scheme = subcarrier_modulation;
  frame_generator = ofdmflexframegen_create(subcarriers,
                                            cyclic_prefix_size,
                                            taper_size,
                                            NULL,
                                            &frame_properties);
  ofdmflexframegen_set_header_len(frame_generator, header_size);
  memcpy(header, radio->id, 4);
  set_counter(header, counter);

  while(!stop)
  {
    r = read_data(payload, payload_size);
    if(r < 0)
    {
      break;
    }
    n = r;
    if(n > 0)
    {
      ofdmflexframegen_assemble(frame_generator, header, payload, n);
      frame_complete = 0;
      while(!frame_complete)
      {
        frame_complete = ofdmflexframegen_write(frame_generator,
                                                frame_samples,
                                                frame_samples_size);
        /* Don't send the padding 0 bytes */
        for(n = frame_samples_size; n > 0; n--)
        {
          if(frame_samples[n - 1] != 0)
          {
            break;
          }
        }
        /* Reduce the amplitude of samples because the frame generator
         * may produce samples with an amplitude greater than 1.0 */
        liquid_vectorcf_mulscalar(frame_samples, n, 0.22, frame_samples);
        msresamp_crcf_execute(resampler, frame_samples, n, samples, &n);
        if(frequency_offset != 0)
        {
          nco_crcf_mix_block_up(oscillator, samples, samples, n);
        }
        send_to_radio(radio, samples, n, 0);
      }
      counter++;
      set_counter(header, counter);
    }
    else
    {
      /* Underrun when reading from stdin. Send some dummy samples to get the
       * remaining output samples for the end of current frame (because of
       * resampler and filter delays) and send them */
      send_dummy_samples(radio, resampler, oscillator, frame_samples, frame_samples_size,
                         samples, frequency_offset, 0);
    }
  }

  /* Send some dummy samples to get the remaining output samples (because of
   * resampler and filter delays) */
  send_dummy_samples(radio, resampler, oscillator, frame_samples, frame_samples_size,
                     samples, frequency_offset, 1);

  free(samples);
  free(frame_samples);
  nco_crcf_destroy(oscillator);
  msresamp_crcf_destroy(resampler);
  ofdmflexframegen_destroy(frame_generator);
}

int frame_received(unsigned char *header, int header_valid,
                   unsigned char *payload, unsigned int payload_size,
                   int payload_valid, framesyncstats_s stats, void *user_data)
{
  radio_t *radio = (radio_t *) user_data;
  char id[5];
  unsigned int counter;

  memcpy(id, header, 4);
  id[4] = '\0';
  counter = get_counter(header);

  if(!header_valid || !payload_valid)
  {
    if(verbose)
    {
      if(!header_valid)
      {
        fprintf(stderr, "Frame %u for '%s': corrupted header\n", counter, id);
      }
      if(!payload_valid)
      {
        fprintf(stderr, "Frame %u for '%s': corrupted payload\n", counter, id);
      }
      fflush(stderr);
    }
  }
  if(memcmp(id, radio->id, 4) != 0)
  {
    if(verbose)
    {
      fprintf(stderr, "Frame %u for '%s': ignored\n", counter, id);
      fflush(stderr);
    }
  }
  else
  {
    write_data(payload, payload_size);
  }
  return(0);
}

void receive_frames(radio_t *radio, float sample_rate, unsigned int bit_rate)
{
  unsigned int subcarriers = 64;
  unsigned int cyclic_prefix_size = 16;
  unsigned int taper_size = 4;
  unsigned int subcarrier_symbol_bits = 2; /* QPSK */
  float samples_per_bit = 2.0 / subcarrier_symbol_bits;
  ofdmflexframesync frame_synchronizer;
  float resampling_ratio = (bit_rate * samples_per_bit) / sample_rate;
  msresamp_crcf resampler = msresamp_crcf_create(resampling_ratio, 60);
  unsigned int delay = (unsigned int) ceilf(msresamp_crcf_get_delay(resampler));
  unsigned int n;
  unsigned int frame_samples_size = (bit_rate * samples_per_bit) / 20; /* 50 ms */
  unsigned int samples_size = (unsigned int) floorf(frame_samples_size / resampling_ratio) + delay;
  float frequency_offset = (float) radio->frequency - radio->center_frequency;
  nco_crcf oscillator = nco_crcf_create(LIQUID_NCO);
  complex float *frame_samples = malloc((frame_samples_size + delay) * sizeof(complex float));
  complex float *samples = malloc(samples_size * sizeof(complex float));

  if((frame_samples == NULL) || (samples == NULL))
  {
    fprintf(stderr, "Error: Memory allocation failed\n");
    exit(-1);
  }

  nco_crcf_set_phase(oscillator, 0);
  nco_crcf_set_frequency(oscillator, TAU * (frequency_offset / sample_rate));

  frame_synchronizer = ofdmflexframesync_create(subcarriers,
                                                cyclic_prefix_size,
                                                taper_size,
                                                NULL,
                                                frame_received,
                                                radio);

  while(!stop)
  {
    n = receive_from_radio(radio, samples, samples_size);
    if((n == 0) && (radio->type == IO))
    {
      break;
    }
    if(dump)
    {
      dump_samples(samples, n);
    }
    if(frequency_offset != 0)
    {
      nco_crcf_mix_block_down(oscillator, samples, samples, n);
    }
    msresamp_crcf_execute(resampler, samples, n, frame_samples, &n);
    ofdmflexframesync_execute(frame_synchronizer, frame_samples, n);
  }

  for(n = 0; n < delay; n++)
  {
    samples[n] = 0;
  }
  msresamp_crcf_execute(resampler, samples, delay, frame_samples, &n);
  ofdmflexframesync_execute(frame_synchronizer, frame_samples, n);

  free(samples);
  free(frame_samples);
  nco_crcf_destroy(oscillator);
  msresamp_crcf_destroy(resampler);
  ofdmflexframesync_destroy(frame_synchronizer);
}

void signal_handler(int signum)
{
  if(verbose)
  {
    fprintf(stderr, "\nStopping (signal %d)\n", signum);
  }
  else
  {
    fprintf(stderr, "\n");
  }
  stop = 1;
  fclose(stdin);
}

void usage()
{
  size_t size;
  unsigned int i;
  unsigned int j;
  char *driver;
  char *serial;
  SoapySDRKwargs *devices = SoapySDRDevice_enumerate(NULL, &size);

  printf("ofdm-transfer version 1.0.0\n");
  printf("\n");
  printf("Usage: ofdm-transfer [options] [filename]\n");
  printf("\n");
  printf("Options:\n");
  printf("  -b <bit rate>  (default: 9600 b/s)\n");
  printf("    Bit rate of the OFDM transmission.\n");
  printf("  -c <ppm>  (default: 0.0, can be negative)\n");
  printf("    Correction for the radio clock.\n");
  printf("  -d <filename>\n");
  printf("    Dump a copy of the samples sent to or received from\n");
  printf("    the radio.\n");
  printf("  -e <fec[,fec]>  (default: h128,none)\n");
  printf("    Inner and outer forward error correction codes to use.\n");
  printf("  -f <frequency>  (default: 434000000 Hz)\n");
  printf("    Frequency of the OFDM transmission.\n");
  printf("  -g <gain>  (default: 0)\n");
  printf("    Gain of the radio transceiver.\n");
  printf("  -h\n");
  printf("    This help.\n");
  printf("  -i <id>  (default: \"\")\n");
  printf("    Transfer id (at most 4 bytes). When receiving, the frames\n");
  printf("    with a different id will be ignored.\n");
  printf("  -o <offset>  (default: 0 Hz, can be negative)\n");
  printf("    Set the central frequency of the transceiver 'offset' Hz\n");
  printf("    lower than the signal frequency to send or receive.\n");
  printf("  -r <radio>  (default: \"\")\n");
  printf("    Radio to use.\n");
  printf("  -s <sample rate>  (default: 2000000 S/s)\n");
  printf("    Sample rate to use.\n");
  printf("  -t\n");
  printf("    Use transmit mode.\n");
  printf("  -v\n");
  printf("    Print debug messages.\n");
  printf("\n");
  printf("By default the program is in 'receive' mode.\n");
  printf("Use the '-t' option to use the 'transmit' mode.\n");
  printf("\n");
  printf("In 'receive' mode, the samples are received from the radio,\n");
  printf("and the decoded data is written either to 'filename' if it\n");
  printf("is specified, or to standard output.\n");
  printf("In 'transmit' mode, the data to send is read either from\n");
  printf("'filename' if it is specified, or from standard input,\n");
  printf("and the samples are sent to the radio.\n");
  printf("\n");
  printf("Instead of a real radio transceiver, the 'io' radio type uses\n");
  printf("standard input in 'receive' mode, and standard output in\n");
  printf("'transmit' mode. The samples must be in 'complex float' format\n");
  printf("(32 bits for the real part, 32 bits for the imaginary part).\n");
  printf("\n");
  printf("Available radios (via SoapySDR):\n");
  if(size == 0)
  {
    printf("  No radio detected\n");
  }
  else
  {
    for(i = 0; i < size; i++)
    {
      driver = NULL;
      serial = NULL;
      for(j = 0; j < devices[i].size; j++)
      {
        if(strcasecmp(devices[i].keys[j], "driver") == 0)
        {
          driver = devices[i].vals[j];
        }
        else if(strcasecmp(devices[i].keys[j], "serial") == 0)
        {
          serial = devices[i].vals[j];
          if(strlen(serial) > 8)
          {
            serial = &serial[strlen(serial) - 8];
          }
        }
      }
      printf("  - driver=%s,serial=%s\n", driver, serial);
    }
  }
  SoapySDRKwargsList_clear(devices, size);
  printf("\n");
  printf("Available forward error correction codes:\n");
  liquid_print_fec_schemes();
}

int get_fec_schemes(char *str, fec_scheme *inner_fec, fec_scheme *outer_fec)
{
  unsigned int size = strlen(str);
  char spec[size + 1];
  char *separation;

  strcpy(spec, str);
  if((separation = strchr(spec, ',')) != NULL)
  {
    *separation = '\0';
  }

  *inner_fec = liquid_getopt_str2fec(spec);
  if(*inner_fec == LIQUID_FEC_UNKNOWN)
  {
    return(-1);
  }

  if(separation != NULL)
  {
    *outer_fec = liquid_getopt_str2fec(separation + 1);
    if(*outer_fec == LIQUID_FEC_UNKNOWN)
    {
      return(-1);
    }
  }
  else
  {
    *outer_fec = LIQUID_FEC_NONE;
  }

  return(0);
}

int main(int argc, char **argv)
{
  int opt;
  int flags;
  float sample_rate = 2000000;
  unsigned int bit_rate = 9600;
  radio_t radio;
  unsigned int emit = 0;
  unsigned int gain = 0;
  int offset = 0;
  float ppm = 0;
  crc_scheme crc = LIQUID_CRC_32;
  fec_scheme inner_fec = LIQUID_FEC_HAMMING128;
  fec_scheme outer_fec = LIQUID_FEC_NONE;
  char *soapysdr_driver = "";

  bzero(&radio, sizeof(radio));
  radio.type = SOAPYSDR;
  radio.frequency = 434000000;

  while((opt = getopt(argc, argv, "b:c:d:e:f:g:hi:o:r:s:tv")) != -1)
  {
    switch(opt)
    {
    case 'b':
      bit_rate = strtoul(optarg, NULL, 10);
      if(bit_rate == 0)
      {
        fprintf(stderr, "Error: Invalid bit rate: '%s'\n", optarg);
        return(-1);
      }
      break;

    case 'c':
      ppm = strtof(optarg, NULL);
      break;

    case 'd':
      dump = fopen(optarg, "wb");
      if(dump == NULL)
      {
        fprintf(stderr, "Error: Failed to open '%s'\n", optarg);
        return(-1);
      }
      break;

    case 'e':
      if(get_fec_schemes(optarg, &inner_fec, &outer_fec) != 0)
      {
        fprintf(stderr, "Error: Unknown FEC schemes: '%s'\n", optarg);
        return(-1);
      }
      break;

    case 'f':
      radio.frequency = strtoul(optarg, NULL, 10);
      if(radio.frequency == 0)
      {
        fprintf(stderr, "Error: Invalid frequency: '%s'\n", optarg);
        return(-1);
      }
      break;

    case 'g':
      gain = strtoul(optarg, NULL, 10);
      break;

    case 'h':
      usage();
      return(0);

    case 'i':
      if(strlen(optarg) <= 4)
      {
        strcpy(radio.id, optarg);
      }
      else
      {
        fprintf(stderr, "Error: Id must be at most 4 bytes long\n");
        return(-1);
      }
      break;

    case 'o':
      offset = strtol(optarg, NULL, 10);
      break;

    case 's':
      sample_rate = strtoul(optarg, NULL, 10);
      if(sample_rate == 0)
      {
        fprintf(stderr, "Error: Invalid sample rate: '%s'\n", optarg);
        return(-1);
      }
      break;

    case 'r':
      if(strcasecmp(optarg, "io") == 0)
      {
        radio.type = IO;
      }
      else
      {
        radio.type = SOAPYSDR;
        soapysdr_driver = optarg;
      }
      break;

    case 't':
      emit = 1;
      break;

    case 'v':
      verbose = 1;
      break;

    default:
      fprintf(stderr, "Error: Unknown parameter: '-%c %s'\n", opt, optarg);
      return(-1);
    }
  }
  if(optind < argc)
  {
    if(emit)
    {
      file = fopen(argv[optind], "rb");
    }
    else
    {
      file = fopen(argv[optind], "wb");
    }
    if(file == NULL)
    {
      fprintf(stderr, "Error: Failed to open '%s'\n", argv[optind]);
      return(-1);
    }
  }
  else
  {
    if(emit)
    {
      file = stdin;
      flags = fcntl(STDIN_FILENO, F_GETFL);
      fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }
    else
    {
      file = stdout;
    }
  }

  if(ppm != 0)
  {
    sample_rate = sample_rate * ((1000000.0 - ppm) / 1000000.0);
    radio.frequency = (unsigned long int) (radio.frequency * ((1000000.0 - ppm) / 1000000.0));
  }
  radio.center_frequency = radio.frequency - offset;

  signal(SIGINT, &signal_handler);
  signal(SIGTERM, &signal_handler);
  signal(SIGABRT, &signal_handler);

  switch(radio.type)
  {
  case IO:
    if(verbose)
    {
      fprintf(stderr, "Info: Using IO pseudo-radio\n");
    }
    if(emit)
    {
      send_frames(&radio, sample_rate, bit_rate, crc, inner_fec, outer_fec);
    }
    else
    {
      receive_frames(&radio, sample_rate, bit_rate);
    }
    break;

  case SOAPYSDR:
    radio.device.soapysdr = SoapySDRDevice_makeStrArgs(soapysdr_driver);
    if(radio.device.soapysdr == NULL)
    {
      fprintf(stderr, "Error: %s\n", SoapySDRDevice_lastError());
      return(-1);
    }
    if(emit)
    {
      SOAPYSDR_CHECK(SoapySDRDevice_setSampleRate(radio.device.soapysdr,
                                                  SOAPY_SDR_TX,
                                                  0,
                                                  sample_rate));
      SOAPYSDR_CHECK(SoapySDRDevice_setFrequency(radio.device.soapysdr,
                                                 SOAPY_SDR_TX,
                                                 0,
                                                 radio.center_frequency,
                                                 NULL));
      SOAPYSDR_CHECK(SoapySDRDevice_setGain(radio.device.soapysdr,
                                            SOAPY_SDR_TX,
                                            0,
                                            gain));
      radio.stream = SoapySDRDevice_setupStream(radio.device.soapysdr,
                                                SOAPY_SDR_TX,
                                                SOAPY_SDR_CF32,
                                                NULL,
                                                0,
                                                NULL);
      if(radio.stream == NULL)
      {
        fprintf(stderr, "Error: %s\n", SoapySDRDevice_lastError());
        SoapySDRDevice_unmake(radio.device.soapysdr);
        return(-1);
      }
      SoapySDRDevice_activateStream(radio.device.soapysdr, radio.stream, 0, 0, 0);
      send_frames(&radio, sample_rate, bit_rate, crc, inner_fec, outer_fec);
    }
    else
    {
      SOAPYSDR_CHECK(SoapySDRDevice_setSampleRate(radio.device.soapysdr,
                                                  SOAPY_SDR_RX,
                                                  0,
                                                  sample_rate));
      SOAPYSDR_CHECK(SoapySDRDevice_setFrequency(radio.device.soapysdr,
                                                 SOAPY_SDR_RX,
                                                 0,
                                                 radio.center_frequency,
                                                 NULL));
      SOAPYSDR_CHECK(SoapySDRDevice_setGain(radio.device.soapysdr,
                                            SOAPY_SDR_RX,
                                            0,
                                            gain));
      radio.stream = SoapySDRDevice_setupStream(radio.device.soapysdr,
                                                SOAPY_SDR_RX,
                                                SOAPY_SDR_CF32,
                                                NULL,
                                                0,
                                                NULL);
      if(radio.stream == NULL)
      {
        fprintf(stderr, "Error: %s\n", SoapySDRDevice_lastError());
        SoapySDRDevice_unmake(radio.device.soapysdr);
        return(-1);
      }
      SoapySDRDevice_activateStream(radio.device.soapysdr, radio.stream, 0, 0, 0);
      receive_frames(&radio, sample_rate, bit_rate);
    }
    SoapySDRDevice_deactivateStream(radio.device.soapysdr, radio.stream, 0, 0);
    SoapySDRDevice_closeStream(radio.device.soapysdr, radio.stream);
    SoapySDRDevice_unmake(radio.device.soapysdr);
    break;

  default:
    fprintf(stderr, "Error: Unknown radio type\n");
    return(-1);
  }

  fclose(file);
  if(dump)
  {
    fclose(dump);
  }
  if(verbose)
  {
    fprintf(stderr, "\n");
  }

  return(0);
}
