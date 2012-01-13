#include <libusb-1.0/libusb.h>
#include <iostream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <sys/stat.h>
#include "logo.h"
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <linux/uinput.h>
#include <fcntl.h>
#include "g13.h"

using namespace std;


int g13_count;

void g13_set_key_color(libusb_device_handle *handle, int red, int green, int blue);
void g13_set_mode_leds(libusb_device_handle *handle, int leds);



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
void g13_parse_joystick(unsigned char *buf, g13_keypad *g13) {
  int stick_x = buf[1];
  int stick_y = buf[2];
  int key_left = g13->stick_keys[STICK_LEFT];
  int key_right = g13->stick_keys[STICK_RIGHT];
  int key_up = g13->stick_keys[STICK_UP];
  int key_down = g13->stick_keys[STICK_DOWN];
  if(g13->stick_mode == STICK_ABSOLUTE) {
    send_event(g13->uinput_file, EV_ABS, ABS_X, stick_x);
    send_event(g13->uinput_file, EV_ABS, ABS_Y, stick_y);
  } else if(g13->stick_mode == STICK_KEYS) {
    if(stick_x < 255/6) {
      send_event(g13->uinput_file, EV_KEY, key_left, 1);
      send_event(g13->uinput_file, EV_KEY, key_right, 0);
    } else if(stick_x > 255/6*5) {
      send_event(g13->uinput_file, EV_KEY, key_left, 0);
      send_event(g13->uinput_file, EV_KEY, key_right, 1);
    } else {
      send_event(g13->uinput_file, EV_KEY, key_left, 0);
      send_event(g13->uinput_file, EV_KEY, key_right, 0);
    }
    if(stick_y < 255/6) {
      send_event(g13->uinput_file, EV_KEY, key_up, 1);
      send_event(g13->uinput_file, EV_KEY, key_down, 0);
    } else if(stick_y > 255/6*5) {
      send_event(g13->uinput_file, EV_KEY, key_up, 0);
      send_event(g13->uinput_file, EV_KEY, key_down, 1);
    } else {
      send_event(g13->uinput_file, EV_KEY, key_up, 0);
      send_event(g13->uinput_file, EV_KEY, key_down, 0);
    }
  } else {
    /*    send_event(g13->uinput_file, EV_REL, REL_X, stick_x/16 - 8);
          send_event(g13->uinput_file, EV_REL, REL_Y, stick_y/16 - 8);*/
  }
}
void g13_parse_key(int key, unsigned char *byte, g13_keypad *g13) {
  unsigned char actual_byte = byte[key / 8];
  unsigned char mask = 1 << (key % 8);
  if(bool value = g13->update(key, actual_byte & mask)) {
//    cout << g13->mapping(key) << ": " << g13->is_set(key) << endl;
    send_event(g13->uinput_file, EV_KEY, g13->mapping(key), g13->is_set(key));
  }
}
void g13_parse_keys(unsigned char *buf, g13_keypad *g13) {
  g13_parse_key(G13_KEY_G1, buf+3, g13);
  g13_parse_key(G13_KEY_G2, buf+3, g13);
  g13_parse_key(G13_KEY_G3, buf+3, g13);
  g13_parse_key(G13_KEY_G4, buf+3, g13);
  g13_parse_key(G13_KEY_G5, buf+3, g13);
  g13_parse_key(G13_KEY_G6, buf+3, g13);
  g13_parse_key(G13_KEY_G7, buf+3, g13);
  g13_parse_key(G13_KEY_G8, buf+3, g13);

  g13_parse_key(G13_KEY_G9, buf+3, g13);
  g13_parse_key(G13_KEY_G10, buf+3, g13);
  g13_parse_key(G13_KEY_G11, buf+3, g13);
  g13_parse_key(G13_KEY_G12, buf+3, g13);
  g13_parse_key(G13_KEY_G13, buf+3, g13);
  g13_parse_key(G13_KEY_G14, buf+3, g13);
  g13_parse_key(G13_KEY_G15, buf+3, g13);
  g13_parse_key(G13_KEY_G16, buf+3, g13);

  g13_parse_key(G13_KEY_G17, buf+3, g13);
  g13_parse_key(G13_KEY_G18, buf+3, g13);
  g13_parse_key(G13_KEY_G19, buf+3, g13);
  g13_parse_key(G13_KEY_G20, buf+3, g13);
  g13_parse_key(G13_KEY_G21, buf+3, g13);
  g13_parse_key(G13_KEY_G22, buf+3, g13);
  //  g13_parse_key(G13_KEY_LIGHT_STATE, buf+3, g13);

  g13_parse_key(G13_KEY_BD, buf+3, g13);
  g13_parse_key(G13_KEY_L1, buf+3, g13);
  g13_parse_key(G13_KEY_L2, buf+3, g13);
  g13_parse_key(G13_KEY_L3, buf+3, g13);
  g13_parse_key(G13_KEY_L4, buf+3, g13);
  g13_parse_key(G13_KEY_M1, buf+3, g13);
  g13_parse_key(G13_KEY_M2, buf+3, g13);

  g13_parse_key(G13_KEY_M3, buf+3, g13);
  g13_parse_key(G13_KEY_MR, buf+3, g13);
  g13_parse_key(G13_KEY_LEFT, buf+3, g13);
  g13_parse_key(G13_KEY_DOWN, buf+3, g13);
  g13_parse_key(G13_KEY_TOP, buf+3, g13);
  g13_parse_key(G13_KEY_LIGHT, buf+3, g13);
  //  g13_parse_key(G13_KEY_LIGHT2, buf+3, file);
  /*  cout << hex << setw(2) << setfill('0') << (int)buf[7];
  cout << hex << setw(2) << setfill('0') << (int)buf[6];
  cout << hex << setw(2) << setfill('0') << (int)buf[5];
  cout << hex << setw(2) << setfill('0') << (int)buf[4];
  cout << hex << setw(2) << setfill('0') << (int)buf[3] << endl;*/
}

void g13_init_lcd(libusb_device_handle *handle) {
  int error = libusb_control_transfer(handle, 0, 9, 1, 0, null, 0, 1000);
  if(error) {
    cerr << "Error when initialising lcd endpoint" << endl;
  }
}
void g13_deregister(g13_keypad *g13) {
  libusb_release_interface(g13->handle, 0);
  libusb_close(g13->handle);
}

void discover_g13s(libusb_device **devs, ssize_t count, vector<g13_keypad*>& g13s) {
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
      g13s.push_back(new g13_keypad(handle, g13_count++));
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
int g13_create_fifo(g13_keypad *g13) {
  mkfifo(g13->fifo_name(), 0666);
  return open(g13->fifo_name(), O_RDWR | O_NONBLOCK);
}
int g13_create_uinput(g13_keypad *g13) {
  struct uinput_user_dev uinp;
  struct input_event event;
  const char* dev_uinput_fname = access("/dev/input/uinput", F_OK)==0 ? "/dev/input/uinput"
                               : access("/dev/uinput", F_OK)==0 ? "/dev/uinput"
                               : 0;
  if(!dev_uinput_fname) {
      cerr << "Could not find an uinput device" << endl;
      return -1;
  }
  if(access(dev_uinput_fname, W_OK) != 0) {
      cerr << dev_uinput_fname << " doesn't grant write permissions" << endl;
      return -1;
  }
  int ufile = open(dev_uinput_fname, O_WRONLY | O_NDELAY);
  if(ufile <= 0) {
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
  /*  ioctl(ufile, UI_SET_EVBIT, EV_REL);*/
  ioctl(ufile, UI_SET_MSCBIT, MSC_SCAN);
  ioctl(ufile, UI_SET_ABSBIT, ABS_X);
  ioctl(ufile, UI_SET_ABSBIT, ABS_Y);
  /*  ioctl(ufile, UI_SET_RELBIT, REL_X);
      ioctl(ufile, UI_SET_RELBIT, REL_Y);*/
  for(int i = 0; i < 256; i++)
    ioctl(ufile, UI_SET_KEYBIT, i);
  ioctl(ufile, UI_SET_KEYBIT, BTN_THUMB);

  int retcode = write(ufile, &uinp, sizeof(uinp));
  if(retcode < 0) {
    cerr << "Could not write to uinput device (" << retcode << ")" << endl;
    return -1;
  }
  retcode = ioctl(ufile, UI_DEV_CREATE);
  if(retcode) {
    cerr << "Error creating uinput device for G13" << endl;
    return -1;
  }
  return ufile;
}

void register_g13(libusb_context *ctx, g13_keypad *g13) {
  int leds = 0;
  int red = 0;
  int green = 0;
  int blue = 255;
  g13_init_lcd(g13->handle);
  g13_set_mode_leds(g13->handle, leds);
  g13_set_key_color(g13->handle, red, green, blue);
  g13_write_lcd(ctx, g13->handle, g13_logo, sizeof(g13_logo));
  g13->uinput_file = g13_create_uinput(g13);
  g13->fifo = g13_create_fifo(g13);
}

void g13_write_lcd_file(libusb_context *ctx, g13_keypad *g13, string filename) {
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
  g13_write_lcd(ctx, g13->handle, (unsigned char *)buffer, size);
}
int g13_read_keys(g13_keypad *g13) {
  unsigned char buffer[G13_REPORT_SIZE];
  int size;
  int error = libusb_interrupt_transfer(g13->handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer, G13_REPORT_SIZE, &size, 1000);
  if(error && error != LIBUSB_ERROR_TIMEOUT) {
    std::map<int,std::string> errors;
    errors[LIBUSB_SUCCESS] = "LIBUSB_SUCCESS";
    errors[LIBUSB_ERROR_IO] = "LIBUSB_ERROR_IO";
    errors[LIBUSB_ERROR_INVALID_PARAM] = "LIBUSB_ERROR_INVALID_PARAM";
    errors[LIBUSB_ERROR_ACCESS] = "LIBUSB_ERROR_ACCESS";
    errors[LIBUSB_ERROR_NO_DEVICE] = "LIBUSB_ERROR_NO_DEVICE";
    errors[LIBUSB_ERROR_NOT_FOUND] = "LIBUSB_ERROR_NOT_FOUND";
    errors[LIBUSB_ERROR_BUSY] = "LIBUSB_ERROR_BUSY";
    errors[LIBUSB_ERROR_TIMEOUT] = "LIBUSB_ERROR_TIMEOUT";
    errors[LIBUSB_ERROR_OVERFLOW] = "LIBUSB_ERROR_OVERFLOW";
    errors[LIBUSB_ERROR_PIPE] = "LIBUSB_ERROR_PIPE";
    errors[LIBUSB_ERROR_INTERRUPTED] = "LIBUSB_ERROR_INTERRUPTED";
    errors[LIBUSB_ERROR_NO_MEM] = "LIBUSB_ERROR_NO_MEM";
    errors[LIBUSB_ERROR_NOT_SUPPORTED] = "LIBUSB_ERROR_NOT_SUPPORTED";
    errors[LIBUSB_ERROR_OTHER    ] = "LIBUSB_ERROR_OTHER    ";
    cerr << "Error while reading keys: " << error << " (" << errors[error] << ")" << endl;
    //    cerr << "Stopping daemon" << endl;
    //    return -1;
  }
  if(size == G13_REPORT_SIZE) {
    g13_parse_joystick(buffer, g13);
    g13_parse_keys(buffer, g13);
    send_event(g13->uinput_file, EV_SYN, SYN_REPORT, 0);
  }
  return 0;
}
void g13_destroy_fifo(g13_keypad *g13) {
  remove(g13->fifo_name());
}
void g13_destroy_uinput(g13_keypad *g13) {
  ioctl(g13->uinput_file, UI_DEV_DESTROY);
  close(g13->uinput_file);
}


libusb_context *ctx = null;
vector<g13_keypad*> g13s;

void cleanup(int n = 0) {
  //  cout << "cleaning up" << endl;
  for(int i = 0; i < g13s.size(); i++) {
    g13_destroy_uinput(g13s[i]);
    g13_destroy_fifo(g13s[i]);
    g13_deregister(g13s[i]);
    delete g13s[i];
  }
  libusb_exit(ctx);
}
bool running = true;
void set_stop(int) {
  running = false;
}
void g13_read_commands(g13_keypad *g13) {
  fd_set set;
  FD_ZERO(&set);
  FD_SET(g13->fifo, &set);
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  int ret = select(g13->fifo + 1, &set, 0, 0, &tv);
  if(ret > 0) {
    unsigned char buf[1024*1024];
    memset(buf, 0, 1024*1024);
    ret = read(g13->fifo, buf, 1024*1024);
    //    std::cout << "INFO: read " << ret << " characters" << std::endl;
    if(ret == 960) { // TODO probably image, for now, don't test, just assume image
      g13->image(buf, ret);
    } else {
      std::string buffer = reinterpret_cast<const char*>(buf);
      std::vector<std::string> lines;
      boost::split(lines, buffer, boost::is_any_of("\n\r"));
      //      std::cout << "INFO: lines: " << lines.size() << std::endl;
      BOOST_FOREACH(std::string const &cmd, lines) {
        std::vector<std::string> command;
        boost::split(command, cmd, boost::is_any_of("#"));
        //        std::cout << "INFO: command [" << command.size() << "]: " << command[0] << " (" << command[0].size() << ")" << std::endl;
        if(command.size() > 0 && command[0] != std::string("")) {
	  cout << "command: " << command[0] << endl;
          g13->command(command[0].c_str());
	}
      }
    }
  }}
std::map<int,std::string> key_to_name;
std::map<std::string,int> name_to_key;
std::map<int,std::string> input_key_to_name;
std::map<std::string,int> input_name_to_key;
void init_keynames() {
#define g13k(symbol,name) { key_to_name[symbol] = name; name_to_key[name] = symbol; }
#define inpk(symbol) { input_key_to_name[symbol] = #symbol; input_name_to_key[#symbol] = symbol; }
  g13k(G13_KEY_G1,"G1");
  g13k(G13_KEY_G2,"G2");
  g13k(G13_KEY_G3,"G3");
  g13k(G13_KEY_G4,"G4");
  g13k(G13_KEY_G5,"G5");
  g13k(G13_KEY_G6,"G6");
  g13k(G13_KEY_G7,"G7");
  g13k(G13_KEY_G8,"G8");
  g13k(G13_KEY_G9,"G9");
  g13k(G13_KEY_G10,"G10");
  g13k(G13_KEY_G11,"G11");
  g13k(G13_KEY_G12,"G12");
  g13k(G13_KEY_G13,"G13");
  g13k(G13_KEY_G14,"G14");
  g13k(G13_KEY_G15,"G15");
  g13k(G13_KEY_G16,"G16");
  g13k(G13_KEY_G17,"G17");
  g13k(G13_KEY_G18,"G18");
  g13k(G13_KEY_G19,"G19");
  g13k(G13_KEY_G20,"G20");
  g13k(G13_KEY_G21,"G21");
  g13k(G13_KEY_G22,"G22");
  g13k(G13_KEY_LIGHT_STATE,"LIGHT_STATE");
  g13k(G13_KEY_BD,"BD");
  g13k(G13_KEY_L1,"L1");
  g13k(G13_KEY_L2,"L2");
  g13k(G13_KEY_L3,"L3");
  g13k(G13_KEY_L4,"L4");
  g13k(G13_KEY_M1,"M1");
  g13k(G13_KEY_M2,"M2");
  g13k(G13_KEY_M3,"M3");
  g13k(G13_KEY_MR,"MR");
  g13k(G13_KEY_LEFT,"LEFT");
  g13k(G13_KEY_DOWN,"DOWN");
  g13k(G13_KEY_TOP,"TOP");
  g13k(G13_KEY_LIGHT,"LIGHT");
  inpk(KEY_ESC);
  inpk(KEY_1);
  inpk(KEY_2);
  inpk(KEY_3);
  inpk(KEY_4);
  inpk(KEY_5);
  inpk(KEY_6);
  inpk(KEY_7);
  inpk(KEY_8);
  inpk(KEY_9);
  inpk(KEY_0);
  inpk(KEY_MINUS);
  inpk(KEY_EQUAL);
  inpk(KEY_BACKSPACE);
  inpk(KEY_TAB);
  inpk(KEY_Q);
  inpk(KEY_W);
  inpk(KEY_E);
  inpk(KEY_R);
  inpk(KEY_T);
  inpk(KEY_Y);
  inpk(KEY_U);
  inpk(KEY_I);
  inpk(KEY_O);
  inpk(KEY_P);
  inpk(KEY_LEFTBRACE);
  inpk(KEY_RIGHTBRACE);
  inpk(KEY_ENTER);
  inpk(KEY_LEFTCTRL);
  inpk(KEY_RIGHTCTRL);
  inpk(KEY_A);
  inpk(KEY_S);
  inpk(KEY_D);
  inpk(KEY_F);
  inpk(KEY_G);
  inpk(KEY_H);
  inpk(KEY_J);
  inpk(KEY_K);
  inpk(KEY_L);
  inpk(KEY_SEMICOLON);
  inpk(KEY_APOSTROPHE);
  inpk(KEY_GRAVE);
  inpk(KEY_LEFTSHIFT);
  inpk(KEY_BACKSLASH);
  inpk(KEY_Z);
  inpk(KEY_X);
  inpk(KEY_C);
  inpk(KEY_V);
  inpk(KEY_B);
  inpk(KEY_N);
  inpk(KEY_M);
  inpk(KEY_COMMA);
  inpk(KEY_DOT);
  inpk(KEY_SLASH);
  inpk(KEY_RIGHTSHIFT);
  inpk(KEY_KPASTERISK);
  inpk(KEY_LEFTALT);
  inpk(KEY_RIGHTALT);
  inpk(KEY_SPACE);
  inpk(KEY_CAPSLOCK);
  inpk(KEY_F1);
  inpk(KEY_F2);
  inpk(KEY_F3);
  inpk(KEY_F4);
  inpk(KEY_F5);
  inpk(KEY_F6);
  inpk(KEY_F7);
  inpk(KEY_F8);
  inpk(KEY_F9);
  inpk(KEY_F10);
  inpk(KEY_NUMLOCK);
  inpk(KEY_SCROLLLOCK);
  inpk(KEY_KP7);
  inpk(KEY_KP8);
  inpk(KEY_KP9);
  inpk(KEY_KPMINUS);
  inpk(KEY_KP4);
  inpk(KEY_KP5);
  inpk(KEY_KP6);
  inpk(KEY_KPPLUS);
  inpk(KEY_KP1);
  inpk(KEY_KP2);
  inpk(KEY_KP3);
  inpk(KEY_KP0);
  inpk(KEY_KPDOT);
  inpk(KEY_LEFT);
  inpk(KEY_RIGHT);
  inpk(KEY_UP);
  inpk(KEY_DOWN);
  inpk(KEY_PAGEUP);
  inpk(KEY_PAGEDOWN);
  inpk(KEY_HOME);
  inpk(KEY_END);
  inpk(KEY_INSERT);
  inpk(KEY_DELETE);

}
void display_keys() {
  typedef std::map<std::string,int> mapType;
  std::cout << "Known keys on G13:" << std::endl;
  BOOST_FOREACH(const mapType::value_type &item, name_to_key) {
    std::cout << item.first << " ";
  }
  std::cout << "STICK_LEFT " << "STICK_RIGHT " << "STICK_UP " << "STICK_DOWN ";
  std::cout << std::endl;
  std::cout << "Known keys to map to:" << std::endl;
  BOOST_FOREACH(const mapType::value_type &item, input_name_to_key) {
    std::cout << item.first << " ";
  }
  std::cout << std::endl;

}
int main(int argc, char *argv[]) {
  init_keynames();
  display_keys();
  g13_count = 0;
  string filename;
  if(argc == 2) {
    filename = argv[1];
    cout << "Setting logo: " << filename << endl;
  }
  libusb_device **devs;
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
  discover_g13s(devs, cnt, g13s);
  libusb_free_device_list(devs, 1);
  cout << "Found " << g13s.size() << " G13s" << endl;
  if(g13s.size() == 0) {
    return 1;
  }
  for(int i = 0; i < g13s.size(); i++) {
    register_g13(ctx, g13s[i]);
  }
  signal(SIGINT, set_stop);
  if(g13s.size() > 0 && argc == 2) {
    g13_write_lcd_file(ctx, g13s[0], filename);
  }
  do {
    if(g13s.size() > 0)
      for(int i = 0; i < g13s.size(); i++) {
        int status = g13_read_keys(g13s[i]);
        g13_read_commands(g13s[i]);
        if(status < 0)
          running = false;
      }
  } while(running);
  cleanup();
}

void g13_keypad::image(unsigned char *data, int size) {
  g13_write_lcd(ctx, this->handle, data, size);
}

g13_keypad::g13_keypad(libusb_device_handle *handle, int id) {
    this->handle = handle;
    this->id = id;
    this->stick_mode = STICK_KEYS;
    this->stick_keys[STICK_LEFT] = KEY_LEFT;
    this->stick_keys[STICK_RIGHT] = KEY_RIGHT;
    this->stick_keys[STICK_UP] = KEY_UP;
    this->stick_keys[STICK_DOWN] = KEY_DOWN;
    uinput_file = -1;
    for(int i = 0; i < sizeof(keys); i++)
      keys[i] = false;
    for(int i = 0; i < G13_NUM_KEYS; i++)
      map[i] = KEY_A;
    /*      map = { KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H,
              KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P,
              KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, 0, 0,
              KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_1, KEY_2, KEY_3, KEY_4,
              KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_F1, KEY_F2 };*/
    // starcraft 2
    // map = { KEY_5, KEY_3, KEY_1, KEY_0, KEY_2, KEY_4, KEY_6,
    //         KEY_V, KEY_F, KEY_E, KEY_C, KEY_B, KEY_G, KEY_I,
    //         KEY_LEFTSHIFT, KEY_M, KEY_T, KEY_L, KEY_H,
    //         KEY_A, KEY_S, KEY_LEFTCTRL, 0, 0,
    //         KEY_F1, KEY_N, KEY_R, KEY_P, KEY_K, KEY_D, KEY_X, KEY_Y,
    // 	    KEY_Z, KEY_TAB, KEY_W, KEY_BACKSPACE, 0, 0, 0, 0};
    }

    void g13_keypad::command(char const *str) {
    int red, green, blue, mod;
    char keyname[256];
    char binding[256];
    if(sscanf(str, "rgb %i %i %i", &red, &green, &blue) == 3) {
      g13_set_key_color(handle, red, green, blue);
    } else if(sscanf(str, "mod %i", &mod) == 1) {
      g13_set_mode_leds(handle, mod);
    } else if(sscanf(str, "bind %255s %255s", keyname, binding) == 2) {
      std::string key_name(keyname);
      if(input_name_to_key.find(binding) != input_name_to_key.end()) {
        int bind = input_name_to_key[binding];
        if(name_to_key.find(keyname) != name_to_key.end()) {
          int key = name_to_key[keyname];
          map[key] = bind;
        } else if(key_name == "STICK_LEFT") {
          this->stick_keys[STICK_LEFT] = bind;
        } else if(key_name == "STICK_RIGHT") {
          this->stick_keys[STICK_RIGHT] = bind;
        } else if(key_name == "STICK_UP") {
          this->stick_keys[STICK_UP] = bind;
        } else if(key_name == "STICK_DOWN") {
          this->stick_keys[STICK_DOWN] = bind;
        } else {
          cerr << "unknown g13 key: " << keyname << endl;
        }
      } else {
        cerr << "unknown key: " << binding << endl;
      }

    } else {
      cerr << "unknown command: <" << str << ">" <<  endl;
    }
  }
