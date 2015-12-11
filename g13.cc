
#include "g13.h"
#include "logo.h"
using namespace std;

namespace G13 {


int g13_count;

#define G13_KEY_SEQ															\
	/* byte 3 */	(G1)(G2)(G3)(G4)(G5)(G6)(G7)(G8)						\
	/* byte 4 */	(G9)(G10)(G11)(G12)(G13)(G14)(G15)(G16)					\
	/* byte 5 */	(G17)(G18)(G19)(G20)(G21)(G22)(UNDEF1)(LIGHT_STATE)		\
	/* byte 6 */	(BD)(L1)(L2)(L3)(L4)(M1)(M2)(M3)						\
	/* byte 7 */	(MR)(LEFT)(DOWN)(TOP)(UNDEF3)(LIGHT)(LIGHT2)(MISC_TOGGLE)

#define G13_NONPARSED_KEY_SEQ												\
		(UNDEF1)(LIGHT_STATE)(UNDEF3)(LIGHT)(LIGHT2)(UNDEF3)(MISC_TOGGLE)	\

void G13_Profile::_init_keys() {
	int key_index = 0;

#define INIT_KEY( r, data, elem )											\
		{																	\
			G13_Key key( *this, BOOST_PP_STRINGIZE(elem), key_index++ );	\
			_keys.push_back( key );											\
		}																	\

	BOOST_PP_SEQ_FOR_EACH( INIT_KEY, _, G13_KEY_SEQ )

	assert( _keys.size() == G13_NUM_KEYS );

#define MARK_NON_PARSED_KEY( r, data, elem )								\
		{																	\
			G13_Key *key = find_key( BOOST_PP_STRINGIZE(elem) );			\
			assert(key);													\
			key->_should_parse = false;										\
		}																	\

	BOOST_PP_SEQ_FOR_EACH( MARK_NON_PARSED_KEY, _, G13_NONPARSED_KEY_SEQ )
}

// *************************************************************************

void send_event(int file, int type, int code, int val) {
  struct timeval tvl;
  struct input_event event;

  memset(&event, 0, sizeof(event));
  gettimeofday(&event.time, G13_NULL);
  event.type = type;
  event.code = code;
  event.value = val;
  write(file, &event, sizeof(event));
}

void G13_KeyPad::set_mode_leds(int leds) {

  unsigned char usb_data[] = { 5, 0, 0, 0, 0 };
  usb_data[1] = leds;
  int r = libusb_control_transfer(handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x305, 0, usb_data, 5, 1000);
  if(r != 5) {
    cerr << "Problem sending data" << endl;
    return;
  }
}
void G13_KeyPad::set_key_color( int red, int green, int blue) {
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


void G13_KeyPad::parse_joystick(unsigned char *buf ) {
	G13_KeyPad *g13 = this;
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

void G13_Key::set_mapping( int key ) {
	_mapped_key = key;
}

void G13_Key::parse_key( unsigned char *byte, G13_KeyPad *g13) {
	if( _should_parse ) {
		unsigned char actual_byte = byte[_index.offset];
		if(bool value = g13->update(_index.index, actual_byte & _index.mask)) {
			bool is_down = g13->is_set(_index.index);

			if( _action ) {
				_action->act( *g13,  is_down );
			} else {
				//send_event( g13->uinput_file, EV_KEY, g13->mapping(_index.index), is_down );
				send_event( g13->uinput_file, EV_KEY, mapped_key(), is_down );
			}
		}
	}
}

void G13_Profile::parse_keys( unsigned char *buf ) {
	buf += 3;
	for( size_t i = 0; i < _keys.size(); i++ ) {
		_keys[i].parse_key( buf, &_keypad );
	}
}


void G13_Manager::discover_g13s(libusb_device **devs, ssize_t count, vector<G13_KeyPad*>& g13s) {
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
      g13s.push_back(new G13_KeyPad(*this, handle, g13_count++));
    }
  }
}

int g13_create_fifo(G13_KeyPad *g13) {

  // mkfifo(g13->fifo_name(), 0777); - didn't work
  mkfifo(g13->fifo_name(), 0666);
  chmod( g13->fifo_name(), 0777 );

  return open(g13->fifo_name(), O_RDWR | O_NONBLOCK);
}
int g13_create_uinput(G13_KeyPad *g13) {
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

void G13_KeyPad::register_context( libusb_context *ctx) {
	 G13_KeyPad *g13 = this;
  int leds = 0;
  int red = 0;
  int green = 0;
  int blue = 255;
  g13->init_lcd();
  set_mode_leds(leds);
  set_key_color(red, green, blue);
  g13->write_lcd(ctx, g13_logo, sizeof(g13_logo));
  g13->uinput_file = g13_create_uinput(g13);
  g13->fifo = g13_create_fifo(g13);
  if( g13->fifo == -1 ) {
	  cerr << "failed opening pipe" << endl;

  }
}

int G13_KeyPad::read_keys() {
	G13_KeyPad *g13 = this;
  unsigned char buffer[G13_REPORT_SIZE];
  int size;
  int error = libusb_interrupt_transfer(g13->handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer, G13_REPORT_SIZE, &size, 100);
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
    parse_joystick(buffer);
    current_profile->parse_keys(buffer);

    send_event(g13->uinput_file, EV_SYN, SYN_REPORT, 0);
  }
  return 0;
}

void G13_KeyPad::cleanup() {
	remove(fifo_name());
	ioctl(uinput_file, UI_DEV_DESTROY);
	close(uinput_file);
	libusb_release_interface(handle, 0);
	libusb_close(handle);
}

void G13_Manager::cleanup() {
  cout << "cleaning up" << endl;
  for(int i = 0; i < g13s.size(); i++) {
	  g13s[i]->cleanup();
	  delete g13s[i];
  }
  libusb_exit(ctx);
}
bool running = true;
void set_stop(int) {
  running = false;
}
void G13_KeyPad::read_commands() {
	G13_KeyPad *g13 = this;
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
      g13->lcd().image(buf, ret);
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

G13_Key * G13_Profile::find_key( const std::string &keyname ) {

  if(_keypad._manager.name_to_key.find(keyname) != _keypad._manager.name_to_key.end()) {
    int key = _keypad._manager.name_to_key[keyname];
    if( key >= 0 && key < _keys.size() ) {
    	return &_keys[key];
    }
  }
  return 0;
}


void G13_Manager::init_keynames() {

	int key_index = 0;

	#define ADD_G13_KEY_MAPPING( r, data, elem )							\
		{																	\
			std::string name = BOOST_PP_STRINGIZE(elem);					\
			key_to_name[key_index] = name; 									\
			name_to_key[name] = key_index;									\
			key_index++;													\
		}																	\

	BOOST_PP_SEQ_FOR_EACH( ADD_G13_KEY_MAPPING, _, G13_KEY_SEQ )

#define KB_INPUT_KEY_SEQ													\
		(ESC)(1)(2)(3)(4)(5)(6)(7)(8)(9)(0)									\
		(MINUS)(EQUAL)(BACKSPACE)(TAB)										\
		(Q)(W)(E)(R)(T)(Y)(U)(I)(O)(P)										\
		(LEFTBRACE)(RIGHTBRACE)(ENTER)(LEFTCTRL)(RIGHTCTRL)					\
		(A)(S)(D)(F)(G)(H)(J)(K)(L)											\
		(SEMICOLON)(APOSTROPHE)(GRAVE)(LEFTSHIFT)(BACKSLASH)				\
		(Z)(X)(C)(V)(B)(N)(M)												\
		(COMMA)(DOT)(SLASH)(RIGHTSHIFT)(KPASTERISK)							\
		(LEFTALT)(RIGHTALT)(SPACE)(CAPSLOCK)								\
		(F1)(F2)(F3)(F4)(F5)(F6)(F7)(F8)(F9)(F10)(F11)(F12)					\
		(NUMLOCK)(SCROLLLOCK)												\
		(KP7)(KP8)(KP9)(KPMINUS)(KP4)(KP5)(KP6)(KPPLUS)						\
		(KP1)(KP2)(KP3)(KP0)(KPDOT)											\
		(LEFT)(RIGHT)(UP)(DOWN)												\
		(PAGEUP)(PAGEDOWN)(HOME)(END)(INSERT)(DELETE)						\


#define ADD_KB_KEY_MAPPING( r, data, elem )									\
	{																		\
		std::string name = "KEY_" BOOST_PP_STRINGIZE(elem);					\
		int keyval = BOOST_PP_CAT( KEY_, elem );							\
		input_key_to_name[keyval] = name; 									\
		input_name_to_key[name] = keyval;									\
	}																		\


	BOOST_PP_SEQ_FOR_EACH( ADD_KB_KEY_MAPPING, _, KB_INPUT_KEY_SEQ )
}

void G13_Manager::display_keys() {
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

G13_KeyPad::G13_KeyPad(G13_Manager &manager, libusb_device_handle *handle, int id) : _manager(manager), _lcd(*this)
{
    this->handle = handle;
    this->id = id;
    this->stick_mode = STICK_KEYS;
    this->stick_keys[STICK_LEFT] = KEY_LEFT;
    this->stick_keys[STICK_RIGHT] = KEY_RIGHT;
    this->stick_keys[STICK_UP] = KEY_UP;
    this->stick_keys[STICK_DOWN] = KEY_DOWN;
    uinput_file = -1;

    current_profile = ProfilePtr( new G13_Profile(*this) );
    _profiles["default"] = current_profile;

    for(int i = 0; i < sizeof(keys); i++)
      keys[i] = false;
    //for(int i = 0; i < G13_NUM_KEYS; i++)
    //  map[i] = KEY_A;
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

    lcd().image_clear();


    #define CONTROL_DIR std::string("/tmp/")
    this->_fifo_name = CONTROL_DIR + "g13-" + boost::lexical_cast<std::string>(id);
    _init_fonts();
}

FontPtr G13_KeyPad::switch_to_font( const std::string &name ) {
	  FontPtr rv = _fonts[name];
	  if( rv ) {
		  current_font = rv;
	  }
	  return rv;
}

void G13_KeyPad::switch_to_profile( const std::string &name ) {
	current_profile = profile( name );

}

ProfilePtr G13_KeyPad::profile( const std::string &name ) {
	  ProfilePtr rv = _profiles[name];
	  if( !rv ) {
		  rv = ProfilePtr( new G13_Profile( *current_profile) );
		  _profiles[name] = rv;
	  }
	  return rv;
}

G13_Action::~G13_Action() {}

G13_Action_Keys::G13_Action_Keys( G13_KeyPad & keypad, const std::string &keys_string ) : G13_Action(keypad )
{
    std::vector<std::string> keys;
    boost::split(keys, keys_string, boost::is_any_of("+"));
    BOOST_FOREACH(std::string const &key, keys) {
        if(keypad._manager.input_name_to_key.find(key) == keypad._manager.input_name_to_key.end()) {
        	throw G13_CommandException( "unknown key : " + key );
        }
        _keys.push_back( keypad._manager.input_name_to_key[key] );
    }
	std::vector<int> _keys;
}

G13_Action_Keys::~G13_Action_Keys() {}

void G13_Action_Keys::act( G13_KeyPad &g13, bool is_down ) {
	if( is_down ) {
		for( int i = 0; i < _keys.size(); i++ ) {
			send_event( g13.uinput_file, EV_KEY, _keys[i], is_down );
		}
	} else {
		for( int i = _keys.size() - 1; i >= 0; i-- ) {
			send_event( g13.uinput_file, EV_KEY, _keys[i], is_down );
		}

	}
}

void G13_KeyPad::command(char const *str) {
    int red, green, blue, mod, row, col;;
    char keyname[256];
    char binding[256];
    char c;
    if(sscanf(str, "rgb %i %i %i", &red, &green, &blue) == 3) {
      set_key_color(red, green, blue);
    } else if(sscanf(str, "mod %i", &mod) == 1) {
      set_mode_leds( mod);
    } else if(sscanf(str, "mbind %255s %255s", keyname, binding) == 2) {
        auto key = current_profile->find_key(keyname);
        if(key) {
        	try {
        		key->_action = G13_ActionPtr( new G13_Action_Keys( *this, binding ) );
        	}
        	catch( const std::exception &ex ) {
        		cerr << "mbind " << keyname << " " << binding << " failed : " << ex.what() <<  endl;
        	}
        }
    } else if(sscanf(str, "bind %255s %255s", keyname, binding) == 2) {
      std::string key_name(keyname);
      if(_manager.input_name_to_key.find(binding) != _manager.input_name_to_key.end()) {
        int bind = _manager.input_name_to_key[binding];
        auto key = current_profile->find_key(keyname);
        if(key) {
          key->set_mapping(bind);
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
    } else if(sscanf(str, "image %i %i", &red, &green) == 2) {
    	lcd().image_test(red,green);
    } else if(sscanf(str, "write_char %c %i %i", &c, &row, &col) == 3) {
    	lcd().write_char( c, row, col );
    } else if(sscanf(str, "pos %i %i", &row, &col) == 2) {
    	lcd().write_pos( row, col );
    } else if(sscanf(str, "write_char %c", &c ) == 1) {
    	lcd().write_char( c );
    } else if( !strncmp( str, "out ", 4 ) ) {
    	lcd().write_string( str+4 );
    } else if( !strncmp( str, "clear", 5 ) ) {
    	lcd().image_clear();
    	lcd().image_send();
    } else if(sscanf(str, "textmode %i", &mod ) == 1) {
    	lcd().text_mode = mod;
    } else if( !strncmp( str, "refresh", 6 ) ) {
    	lcd().image_send();
    } else if(sscanf(str, "profile %255s", binding) == 1) {
    	switch_to_profile( binding );
    } else if(sscanf(str, "font %255s", binding) == 1) {
    	switch_to_font( binding );
    } else {
      cerr << "unknown command: <" << str << ">" <<  endl;
    }
}



G13_Manager::G13_Manager() :
	ctx(0), devs(0)
{
}

int G13_Manager::run() {

  init_keynames();
  display_keys();
  g13_count = 0;
  //string filename;


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
    // register_g13(ctx, g13s[i]);
	  g13s[i]->register_context(ctx);
  }
  signal(SIGINT, set_stop);
  if(g13s.size() > 0 && logo_filename.size()) {
	  g13s[0]->write_lcd_file(ctx, logo_filename);
  }
  do {
    if(g13s.size() > 0)
      for(int i = 0; i < g13s.size(); i++) {
        int status = g13s[i]->read_keys();
        g13s[i]->read_commands();
        if(status < 0)
          running = false;
      }
  } while(running);
  cleanup();
}
} // namespace G13

using namespace G13;


int main(int argc, char *argv[]) {

	G13_Manager manager;
	if(argc == 2) {
		cout << "Setting logo: " << argv[1] << endl;
		manager.set_logo( argv[1] );
	}

	manager.run();
};
