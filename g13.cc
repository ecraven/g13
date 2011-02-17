#include <libusb-1.0/libusb.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include "logo.h"
#include <string.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <linux/uinput.h>
#include <fcntl.h>

using namespace std;

#define null 0

#define G13_INTERFACE 0
#define G13_KEY_ENDPOINT 1
#define G13_LCD_ENDPOINT 2
#define G13_KEY_READ_TIMEOUT 0
#define G13_VENDOR_ID 0x046d
#define G13_PRODUCT_ID 0xc21c
#define G13_REPORT_SIZE 8
#define G13_LCD_BUFFER_SIZE 0x3c0

enum G13_KEYS {
    /* byte 3 */
    G13_KEY_G1 = 0,
    G13_KEY_G2,
    G13_KEY_G3,
    G13_KEY_G4,

    G13_KEY_G5,
    G13_KEY_G6,
    G13_KEY_G7,
    G13_KEY_G8,

    /* byte 4 */
    G13_KEY_G9,
    G13_KEY_G10,
    G13_KEY_G11,
    G13_KEY_G12,

    G13_KEY_G13,
    G13_KEY_G14,
    G13_KEY_G15,
    G13_KEY_G16,

    /* byte 5 */
    G13_KEY_G17,
    G13_KEY_G18,
    G13_KEY_G19,
    G13_KEY_G20,

    G13_KEY_G21,
    G13_KEY_G22,
    G13_KEY_UNDEF1,
    G13_KEY_LIGHT_STATE,

    /* byte 6 */
    G13_KEY_BD,
    G13_KEY_L1,
    G13_KEY_L2,
    G13_KEY_L3,

    G13_KEY_L4,
    G13_KEY_M1,
    G13_KEY_M2,
    G13_KEY_M3,

    /* byte 7 */
    G13_KEY_MR,
    G13_KEY_LEFT,
    G13_KEY_DOWN,
    G13_KEY_TOP,

    G13_KEY_UNDEF3,
    G13_KEY_LIGHT,
    G13_KEY_LIGHT2,
    G13_KEY_MISC_TOGGLE
};

uint64_t inline _BV(uint64_t x) { return (1UL << x); }
#define G13_KEY_ONLY_MASK  (_BV(G13_G1)  | \
                            _BV(G13_G2)  | \
                            _BV(G13_G3)  | \
                            _BV(G13_G4)  | \
                            _BV(G13_G5)  | \
                            _BV(G13_G6)  | \
                            _BV(G13_G7)  | \
                            _BV(G13_G8)  | \
                            _BV(G13_G9)  | \
                            _BV(G13_G10) | \
                            _BV(G13_G11) | \
                            _BV(G13_G12) | \
                            _BV(G13_G13) | \
                            _BV(G13_G14) | \
                            _BV(G13_G15) | \
                            _BV(G13_G16) | \
                            _BV(G13_G17) | \
                            _BV(G13_G18) | \
                            _BV(G13_G19) | \
                            _BV(G13_G20) | \
                            _BV(G13_G21) | \
                            _BV(G13_G22) | \
                            _BV(G13_BD)  | \
                            _BV(G13_L1)  | \
                            _BV(G13_L2)  | \
                            _BV(G13_L3)  | \
                            _BV(G13_L4)  | \
                            _BV(G13_M1)  | \
                            _BV(G13_M2)  | \
                            _BV(G13_M3)  | \
                            _BV(G13_MR)  | \
                            _BV(G13_LIGHT))


static inline int g13_key_pressed(uint64_t code, int key_enum) {
    return (code & (1UL << key_enum)) ? 1 : 0;
}

void discover_g13s(libusb_device **devs, ssize_t count, vector<libusb_device_handle*>& g13s);

void register_g13(libusb_context *ctx, libusb_device_handle *dev);

void g13_set_mode_leds(libusb_device_handle *handle, int leds) {
  unsigned char usb_data[] = { 5, 0, 0, 0, 0 };
  usb_data[1] = leds;
  int r = libusb_control_transfer(handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x305, 0, usb_data, 5, 1000);
  if(r != 5) {
    cerr << "Problem sending data" << endl;
    return;
  }
}
void g13_set_key_color(libusb_device_handle *handle, int red, int green, int blue) {
  int error;
  unsigned char usb_data[] = { 5, 0, 0, 0, 0 };
  usb_data[1] = red;
  usb_data[2] = green;
  usb_data[3] = blue;

  error = libusb_control_transfer(handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x307, 0, usb_data, 5, 1000);
  if(error != 5) {
    cerr << "Problem sending data" << endl;
    return;
  }
}
void send_event(int file, int type, int code, int val) {
  struct timeval tvl;
  struct input_event event;

  memset(&event, 0, sizeof(event));
  gettimeofday(&event.time, null);
  event.type = type;
  event.code = code;
  event.value = val;
  write(file, &event, sizeof(event));
}
void g13_parse_joystick(unsigned char *buf, int file) {
  int stick_x = buf[1];
  int stick_y = buf[2];

  send_event(file, EV_ABS, ABS_X, stick_x);
  send_event(file, EV_ABS, ABS_Y, stick_y);
}
void g13_parse_key(int key, unsigned char *byte, int file) {
  unsigned char actual_byte = byte[key / 8];
  unsigned char mask = 1 << (key % 8);
  //  cout << "key: " << key << ", ab: " << (int)actual_byte << ", mask: " << hex << (int)mask << endl;
  if(actual_byte & mask)
    cout << key << endl;
}
void g13_parse_keys(unsigned char *buf, int file) {
  g13_parse_key(G13_KEY_G1, buf+3, file);
  g13_parse_key(G13_KEY_G2, buf+3, file);
  g13_parse_key(G13_KEY_G3, buf+3, file);
  g13_parse_key(G13_KEY_G4, buf+3, file);
  g13_parse_key(G13_KEY_G5, buf+3, file);
  g13_parse_key(G13_KEY_G6, buf+3, file);
  g13_parse_key(G13_KEY_G7, buf+3, file);
  g13_parse_key(G13_KEY_G8, buf+3, file);

  /*  cout << hex << setw(2) << setfill('0') << (int)buf[7];
  cout << hex << setw(2) << setfill('0') << (int)buf[6];
  cout << hex << setw(2) << setfill('0') << (int)buf[5];
  cout << hex << setw(2) << setfill('0') << (int)buf[4];
  cout << hex << setw(2) << setfill('0') << (int)buf[3] << endl;*
}

void g13_init_lcd(libusb_device_handle *handle) {
  int error = libusb_control_transfer(handle, 0, 9, 1, 0, null, 0, 1000);
  if(error) {
    cerr << "Error when initialising lcd endpoint" << endl;
  }
}
void g13_deregister(libusb_device_handle *handle) {
  libusb_release_interface(handle, 0);
  libusb_close(handle);
}
void discover_g13s(libusb_device **devs, ssize_t count, vector<libusb_device_handle*>& g13s) {
  for(int i = 0; i < count; i++) {
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(devs[i], &desc);
    if(r < 0) {
      cout << "Failed to get device descriptor" << endl;
      return;
    }
    if(desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
      libusb_device_handle *handle;
      int r = libusb_open(devs[i], &handle);
      if(r != 0) {
        cerr << "Error opening G13 device" << endl;
        return;
      }
      if(libusb_kernel_driver_active(handle, 0) == 1)
        if(libusb_detach_kernel_driver(handle, 0) == 0)
          cout << "Kernel driver detached" << endl;
      r = libusb_claim_interface(handle, 0);
      if(r < 0) {
        cerr << "Cannot Claim Interface" << endl;
        return;
      }
      g13s.push_back(handle);
    }
  }
}



void g13_write_lcd(libusb_context *ctx, libusb_device_handle *handle, unsigned char *data, size_t size) {
  g13_init_lcd(handle);
  if(size != G13_LCD_BUFFER_SIZE) {
    cerr << "Invalid LCD data size " << size << ", should be " << G13_LCD_BUFFER_SIZE;
    return;
  }
  unsigned char buffer[G13_LCD_BUFFER_SIZE + 32];
  memset(buffer, 0, G13_LCD_BUFFER_SIZE + 32);
  buffer[0] = 0x03;
  memcpy(buffer + 32, data, G13_LCD_BUFFER_SIZE);
  int bytes_written;
  int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_OUT | G13_LCD_ENDPOINT, buffer, G13_LCD_BUFFER_SIZE + 32, &bytes_written, 1000);
  if(error)
    cerr << "Error when transfering image: " << error << ", " << bytes_written << " bytes written" << endl;
}
void register_g13(libusb_context *ctx, libusb_device_handle *handle) {
  int leds = 15;
  int red = 0;
  int green = 255;
  int blue = 0;
  g13_init_lcd(handle);
  g13_set_mode_leds(handle, leds);
  g13_set_key_color(handle, red, green, blue);
  g13_write_lcd(ctx, handle, g13_logo, sizeof(g13_logo));
}

void g13_write_lcd_file(libusb_context *ctx, libusb_device_handle *handle, string filename) {
  filebuf *pbuf;
  ifstream filestr;
  size_t size;

  filestr.open(filename.c_str());
  pbuf = filestr.rdbuf();

  size = pbuf->pubseekoff(0, ios::end, ios::in);
  pbuf->pubseekpos(0, ios::in);

  char buffer[size];

  pbuf->sgetn(buffer, size);

  filestr.close();
  g13_write_lcd(ctx, handle, (unsigned char *)buffer, size);
}
void g13_read_keys(libusb_device_handle *handle, int file) {
  unsigned char buffer[G13_REPORT_SIZE];
  int size;
  int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer, G13_REPORT_SIZE, &size, 1000);
  if(error && error != LIBUSB_ERROR_TIMEOUT) {
    cerr << "Error while reading keys: " << error << endl;
    return;
  }
  if(size == G13_REPORT_SIZE) {
    g13_parse_joystick(buffer, file);
    g13_parse_keys(buffer, file);
    send_event(file, EV_SYN, SYN_REPORT, 0);
  }
}
int g13_create_uinput() {
  struct uinput_user_dev uinp;
  struct input_event event;
  int ufile = open("/dev/uinput", O_WRONLY | O_NDELAY);
  if(ufile == 0) {
    cerr << "Could not open uinput" << endl;
    return -1;
  }
  memset(&uinp, 0, sizeof(uinp));
  char name[] = "G13";
  strncpy(uinp.name, name, sizeof(name));
  uinp.id.version = 1;
  uinp.id.bustype = BUS_USB;
  uinp.id.product = G13_PRODUCT_ID;
  uinp.id.vendor = G13_VENDOR_ID;
  uinp.absmin[ABS_X] = 0;
  uinp.absmin[ABS_Y] = 0;
  uinp.absmax[ABS_X] = 0xff;
  uinp.absmax[ABS_Y] = 0xff;
  //  uinp.absfuzz[ABS_X] = 4;
  //  uinp.absfuzz[ABS_Y] = 4;
  //  uinp.absflat[ABS_X] = 0x80;
  //  uinp.absflat[ABS_Y] = 0x80;

  ioctl(ufile, UI_SET_EVBIT, EV_KEY);
  ioctl(ufile, UI_SET_EVBIT, EV_ABS);
  ioctl(ufile, UI_SET_MSCBIT, MSC_SCAN);
  ioctl(ufile, UI_SET_ABSBIT, ABS_X);
  ioctl(ufile, UI_SET_ABSBIT, ABS_Y);
  for(int i = 0; i < 256; i++)
    ioctl(ufile, UI_SET_KEYBIT, i);
  ioctl(ufile, UI_SET_KEYBIT, BTN_THUMB);

  int retcode = write(ufile, &uinp, sizeof(uinp));
  if(retcode < 0) {
    cerr << "Writing uinput descriptor returned " << retcode << endl;
    return -1;
  }
  retcode = ioctl(ufile, UI_DEV_CREATE);
  if(retcode) {
    cerr << "Error creating uinput device" << endl;
    return -1;
  }
  return ufile;
  //  ioctl(ufile, UI_DEV_DESTROY);
}
int main(int argc, char *argv[]) {
  string filename;
  if(argc == 2) {
    filename = argv[1];
    cout << "Setting logo: " << filename << endl;
  }
  libusb_device **devs;
  libusb_context *ctx = null;
  ssize_t cnt;
  int ret;
  ret = libusb_init(&ctx);
  if(ret < 0) {
    cout << "Initialization error: " << ret << endl;
    return 1;
  }
  libusb_set_debug(ctx, 3);
  cnt = libusb_get_device_list(ctx, &devs);
  if(cnt < 0) {
    cout << "Error while getting device list" << endl;
    return 1;
  }
  vector<libusb_device_handle*> g13s;
  discover_g13s(devs, cnt, g13s);
  libusb_free_device_list(devs, 1);
  cout << "Found " << g13s.size() << " G13s" << endl;
  for(int i = 0; i < g13s.size(); i++) {
    register_g13(ctx, g13s[i]);
  }
  if(g13s.size() > 0 && argc == 2) {
    g13_write_lcd_file(ctx, g13s[0], filename);
  }
  int file = g13_create_uinput();
  do {
    if(g13s.size() > 0)
      g13_read_keys(g13s[0], file);
  } while(1);
  libusb_exit(ctx);
}
