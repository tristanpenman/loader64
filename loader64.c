/**
 * loader64
 *
 * A simple command line tool for uploading data to the EverDrive-64.
 *
 * This is based on code by saturnu <tt@anpa.nl>, which was originally shared at:
 * http://krikzz.com/forum/index.php?topic=1407.msg14076
 *
 * It was later updated by James Friend (https://github.com/jsdf/loader64) to work on macOS.
 *
 * This version includes various changes to make it work with EverDrive OS V3.05, and to simplify
 * the command line interface.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libftdi1/ftdi.h>

#include "gopt/gopt.h"

#define FILE_CHUNK 0x8000
#define USB_VENDOR 0x0403
#define USB_DEVICE 0x6001
#define USB_READ_TIMEOUT 1500
#define USB_WRITE_TIMEOUT 1500
#define MAJOR_VERSION 0
#define MINOR_VERSION 1

unsigned char send_buff[512];
unsigned char recv_buff[512];

int main(int argc, const char **argv) {
  int arg_fail = 1;

  const char *filename;

  int verbose;
  int print_help;

  void *options = gopt_sort(&argc, argv, gopt_start(
      gopt_option('h', 0, gopt_shorts('h'), gopt_longs("help")),
      gopt_option('v', 0, gopt_shorts('v'), gopt_longs("verbose")),
      gopt_option('f', GOPT_ARG, gopt_shorts('f'), gopt_longs("file"))
  ));

  if (gopt(options, 'h')) {
    fprintf(stdout, "loader64 - Everdrive64 USB-tool\n\n");
    fprintf(stdout, " -h, --help            display this help and exit\n");
    fprintf(stdout, " -v, --verbose         verbose\n");
    fprintf(stdout, " -f, --file=rom.z64    write rom to sdram\n");
    exit(EXIT_SUCCESS);
  }

  print_help = 1;

  if (gopt(options, 'f') || gopt(options, 'r') || gopt(options, 'h') || gopt(options, 'v')) {
    print_help = 0;
    if (gopt(options, 'f')) {
      arg_fail = 0;
    }
  }

  verbose = gopt(options, 'v');

  if (arg_fail) {
    printf("%s: missing operand\n",  argv[0]);
    printf("Try '%s --help' for more information.\n",  argv[0]);
    exit(EXIT_FAILURE);
  }

  int ret;
  struct ftdi_context *ftdi;

  ftdi = ftdi_new();
  if (ftdi == 0) {
    fprintf(stderr, "ftdi_new failed\n");
    return EXIT_FAILURE;
  }

  if ((ret = ftdi_usb_open(ftdi, USB_VENDOR, USB_DEVICE)) < 0) {
    fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
    ftdi_free(ftdi);
    return EXIT_FAILURE;
  }

  //read/write timeout e.g. 500ms
  ftdi->usb_read_timeout = USB_READ_TIMEOUT;
  ftdi->usb_write_timeout = USB_WRITE_TIMEOUT;

  if (ftdi->type == TYPE_R) {
    unsigned int chipid;
    if (verbose){
      printf("ftdi_read_chipid: %d\n", ftdi_read_chipid(ftdi, &chipid));
      printf("FTDI chipid: %X\n", chipid);
    }
  }

  //init usb transfer buffer
  memset(send_buff, 0, 512);
  memset(recv_buff, 0, 512);

  send_buff[0] = 'c';
  send_buff[1] = 'm';
  send_buff[2] = 'd';
  send_buff[3] = 't'; // test

  int ret_s = ftdi_write_data(ftdi, send_buff, 512);
  if (verbose) {
    printf("sent: %i bytes\n", ret_s);
  }

  int ret_r = ftdi_read_data(ftdi, recv_buff, 512);
  if (verbose) {
    printf("recv: %i bytes\n", ret_r);
  }

  if (recv_buff[3] == 'r') {
    printf("init test: ok\n");
  } else {
    printf("init test: faild - ED64 not running?\n");
    ftdi_free(ftdi);
    exit(EXIT_SUCCESS);
  }

  memset(send_buff, 0, 512);
  memset(recv_buff, 0, 512);
  ret_s = 0;
  ret_r = 0;

  if (!gopt_arg(options, 'f', &filename)) {
    printf("Failed to parse filename\n");
    exit(EXIT_FAILURE);
  }

  FILE *fp = fopen(filename, "rb");

  int fsize;
  struct stat st;
  stat(filename, &st);
  fsize = st.st_size;

  if (verbose) {
    printf("test_size: %d\n", fsize);
  }

  int length = (int) fsize;
  if (verbose) {
    printf("length: %d\n", length);
  }

  if (((length / 0x10000) * 0x10000) != fsize) {
    length = (int) (((fsize / 0x10000) * 0x10000) + 0x10000);
  }

  if (verbose) {
    printf("length_buffer: %d\n",length);
  }

  unsigned char buffer[FILE_CHUNK];
  memset(buffer, 0, FILE_CHUNK);

  const int crclen = 0x100000 + 4096;
  if (length < crclen) {
    if (verbose) {
      printf("needs filling\n");
    }

    send_buff[0] = 'c';
    send_buff[1] = 'm';
    send_buff[2] = 'd';
    send_buff[3] = 'c'; // fill

    // offset
    int offset = 0x10000000;
    send_buff[4] = (char) (((offset)) >> 24);
    send_buff[5] = (char) (((offset)) >> 16);
    send_buff[6] = (char) (((offset)) >> 8);
    send_buff[7] = (char) (offset);

    // length
    send_buff[8] = (char) (((crclen / 512)) >> 24);
    send_buff[9] = (char) (((crclen / 512)) >> 16);
    send_buff[10] = (char) (((crclen / 512)) >> 8);
    send_buff[11] = (char) (crclen / 512);

    ret_s = ftdi_write_data(ftdi, send_buff, 16);

    if (verbose) {
      printf("sent: %i bytes\n", ret_s);
    }

    sleep(1);

    send_buff[0] = 'c';
    send_buff[1] = 'm';
    send_buff[2] = 'd';
    send_buff[3] = 't'; // test

    ret_s = ftdi_write_data(ftdi, send_buff, 16);

    int ret_r = ftdi_read_data(ftdi, recv_buff, 16);

    if (verbose) {
      printf("recv: %i bytes\n",ret_r);
    }

    if (recv_buff[3] == 'r') {
      printf("fill test: ok\n");
      memset(send_buff, 0, 512);
      memset(recv_buff, 0, 512);
      ret_s = 0;
      ret_r = 0;
    } else {
      printf("fill test: error, %d\n", (int)(recv_buff[3]));
      ftdi_free(ftdi);
      exit(EXIT_SUCCESS);
    }
  }

  memset(send_buff, 0, 512);
  memset(recv_buff, 0, 512);

  send_buff[0] = 'c';
  send_buff[1] = 'm';
  send_buff[2] = 'd';
  send_buff[3] = 'W'; // write

  // offset
  int offset = 0x10000000;
  send_buff[4] = (char) (((offset)) >> 24);
  send_buff[5] = (char) (((offset)) >> 16);
  send_buff[6] = (char) (((offset)) >> 8);
  send_buff[7] = (char) (offset);

  // length
  send_buff[8] = (char) ((length / 512) >> 24);
  send_buff[9] = (char) ((length / 512) >> 16);
  send_buff[10] = (char) ((length / 512) >> 8);
  send_buff[11] = (char) ((length / 512));

  ret_s = ftdi_write_data(ftdi, send_buff, 16);

  if (verbose) {
    printf("send write cmd: %i bytes\n", ret_s);
  }

  // now write in 0x8000 chunks -> 32768
  printf("sending");
  for (int s = 0; s < length; s += 0x8000) {
    int fret = fread(buffer, sizeof(buffer[0]), sizeof(buffer) / sizeof(buffer[0]), fp);
    if (fret > 0) {
      printf(".");
    }

    ret_s = ftdi_write_data(ftdi, buffer, 0x8000);
    if (ret_s < 0) {
      printf("\nfailed to write data: %d\n", ret_s);
      exit(EXIT_FAILURE);
    }
  }

  printf("\ndone\n");

  memset(send_buff, 0, 512);
  memset(recv_buff, 0, 512);

  send_buff[0] = 'c';
  send_buff[1] = 'm';
  send_buff[2] = 'd';
  send_buff[3] = 's'; // pif boot

  ret_s = ftdi_write_data(ftdi, send_buff, 16);

  if (verbose) {
    printf("pif simulation instructed\n");
  }

  ret = ftdi_usb_close(ftdi);
  if (ret < 0) {
    fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
    ftdi_free(ftdi);
    return EXIT_FAILURE;
  }

  ftdi_free(ftdi);

  gopt_free(options);

  return EXIT_SUCCESS;
}

