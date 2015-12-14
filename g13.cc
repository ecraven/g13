#include "g13.h"
#include "logo.h"
#include <fstream>
using namespace std;

// *************************************************************************

#define CONTROL_DIR std::string("/tmp/")

namespace G13 {

// *************************************************************************

void G13_Device::send_event(int type, int code, int val) {

	memset(&_event, 0, sizeof(_event));
	gettimeofday(&_event.time, 0 );
	_event.type = type;
	_event.code = code;
	_event.value = val;
	write(_uinput_fid, &_event, sizeof(_event));
}

void G13_Device::write_output_pipe( const std::string &out ) {
	write( _output_pipe_fid, out.c_str(), out.size() );
}

void G13_Device::set_mode_leds(int leds) {

	unsigned char usb_data[] = { 5, 0, 0, 0, 0 };
	usb_data[1] = leds;
	int r = libusb_control_transfer(handle,
			LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x305, 0,
			usb_data, 5, 1000);
	if (r != 5) {
		cerr << "Problem sending data" << endl;
		return;
	}
}
void G13_Device::set_key_color(int red, int green, int blue) {
	int error;
	unsigned char usb_data[] = { 5, 0, 0, 0, 0 };
	usb_data[1] = red;
	usb_data[2] = green;
	usb_data[3] = blue;

	error = libusb_control_transfer(handle,
			LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x307, 0,
			usb_data, 5, 1000);
	if (error != 5) {
		cerr << "Problem sending data" << endl;
		return;
	}
}

// *************************************************************************

void G13_Manager::discover_g13s(libusb_device **devs, ssize_t count,
		vector<G13_Device*>& g13s) {
	for (int i = 0; i < count; i++) {
		libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(devs[i], &desc);
		if (r < 0) {
			cout << "Failed to get device descriptor" << endl;
			return;
		}
		if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
			libusb_device_handle *handle;
			int r = libusb_open(devs[i], &handle);
			if (r != 0) {
				cerr << "Error opening G13 device" << endl;
				return;
			}
			if (libusb_kernel_driver_active(handle, 0) == 1)
				if (libusb_detach_kernel_driver(handle, 0) == 0)
					cout << "Kernel driver detached" << endl;
			r = libusb_claim_interface(handle, 0);
			if (r < 0) {
				cerr << "Cannot Claim Interface" << endl;
				return;
			}
			g13s.push_back(new G13_Device(*this, handle, g13s.size()));
		}
	}
}

// *************************************************************************

int g13_create_fifo(const char *fifo_name) {

	// mkfifo(g13->fifo_name(), 0777); - didn't work
	mkfifo(fifo_name, 0666);
	chmod(fifo_name, 0777);

	return open(fifo_name, O_RDWR | O_NONBLOCK);
}

// *************************************************************************

int g13_create_uinput(G13_Device *g13) {
	struct uinput_user_dev uinp;
	struct input_event event;
	const char* dev_uinput_fname =
			access("/dev/input/uinput", F_OK) == 0 ? "/dev/input/uinput" :
			access("/dev/uinput", F_OK) == 0 ? "/dev/uinput" : 0;
	if (!dev_uinput_fname) {
		cerr << "Could not find an uinput device" << endl;
		return -1;
	}
	if (access(dev_uinput_fname, W_OK) != 0) {
		cerr << dev_uinput_fname << " doesn't grant write permissions" << endl;
		return -1;
	}
	int ufile = open(dev_uinput_fname, O_WRONLY | O_NDELAY);
	if (ufile <= 0) {
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
	for (int i = 0; i < 256; i++)
		ioctl(ufile, UI_SET_KEYBIT, i);
	ioctl(ufile, UI_SET_KEYBIT, BTN_THUMB);

	int retcode = write(ufile, &uinp, sizeof(uinp));
	if (retcode < 0) {
		cerr << "Could not write to uinput device (" << retcode << ")" << endl;
		return -1;
	}
	retcode = ioctl(ufile, UI_DEV_CREATE);
	if (retcode) {
		cerr << "Error creating uinput device for G13" << endl;
		return -1;
	}
	return ufile;
}

void G13_Device::register_context(libusb_context *_ctx) {
	ctx = _ctx;

	int leds = 0;
	int red = 0;
	int green = 0;
	int blue = 255;
	init_lcd();

	set_mode_leds(leds);
	set_key_color(red, green, blue);

	write_lcd( g13_logo, sizeof(g13_logo) );

	_uinput_fid = g13_create_uinput(this);


	_input_pipe_name = _manager.make_pipe_name(this,true);
	_input_pipe_fid = g13_create_fifo(_input_pipe_name.c_str());
	_output_pipe_name = _manager.make_pipe_name(this,false);
	_output_pipe_fid = g13_create_fifo(_output_pipe_name.c_str());

	if ( _input_pipe_fid == -1 ) {
		cerr << "failed opening pipe" << endl;
	}
}

void G13_Device::cleanup() {
	remove(_input_pipe_name.c_str());
	remove(_output_pipe_name.c_str());
	ioctl(_uinput_fid, UI_DEV_DESTROY);
	close(_uinput_fid);
	libusb_release_interface(handle, 0);
	libusb_close(handle);
}

void G13_Manager::cleanup() {
	cout << "cleaning up" << endl;
	for (int i = 0; i < g13s.size(); i++) {
		g13s[i]->cleanup();
		delete g13s[i];
	}
	libusb_exit(ctx);
}

// *************************************************************************

static std::string describe_libusb_error_code(int code) {

	#define TEST_libusb_error( r, data, elem )								\
	case BOOST_PP_CAT( LIBUSB_, elem ) :									\
		return BOOST_PP_STRINGIZE( elem );									\

	switch (code) {

	BOOST_PP_SEQ_FOR_EACH(TEST_libusb_error, _,
			(SUCCESS)(ERROR_IO)(ERROR_INVALID_PARAM)(ERROR_ACCESS)
			(ERROR_NO_DEVICE)(ERROR_NOT_FOUND)(ERROR_BUSY)
			(ERROR_TIMEOUT)(ERROR_OVERFLOW)(ERROR_PIPE)
			(ERROR_INTERRUPTED)(ERROR_NO_MEM)(ERROR_NOT_SUPPORTED)
			(ERROR_OTHER))

	}
	return "unknown error";
}

// *************************************************************************

/*! reads and processes key state report from G13
 *
 */
int G13_Device::read_keys() {
	unsigned char buffer[G13_REPORT_SIZE];
	int size;
	int error = libusb_interrupt_transfer( handle,
			LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer, G13_REPORT_SIZE,
			&size, 100);

	if (error && error != LIBUSB_ERROR_TIMEOUT) {

		cerr << "Error while reading keys: " << error << " ("
				<< describe_libusb_error_code(error) << ")" << endl;
		//    cerr << "Stopping daemon" << endl;
		//    return -1;
	}
	if (size == G13_REPORT_SIZE) {
		parse_joystick(buffer);
		_current_profile->parse_keys(buffer);
		send_event( EV_SYN, SYN_REPORT, 0);
	}
	return 0;
}


void G13_Device::read_config_file( const std::string &filename ) {
	std::ifstream s( filename );

	std::cout << "reading configuration from " << filename << std::endl;
	while( s.good() ) {

		// grab a line
		char buf[1024];
		buf[0] = 0;
		buf[sizeof(buf)-1] = 0;
		s.getline( buf, sizeof(buf)-1 );

		// strip comment
		char *comment = strchr(buf,'#');
		if( comment ) {
			comment--;
			while( comment > buf && isspace( *comment ) ) comment--;
			*comment = 0;
		}

		// send it
		if( buf[0] ) {
			std::cout << "  cfg: " << buf << std::endl;
			command( buf );
		}
	}
}

void G13_Device::read_commands() {

	fd_set set;
	FD_ZERO(&set);
	FD_SET(_input_pipe_fid, &set);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	int ret = select(_input_pipe_fid + 1, &set, 0, 0, &tv);
	if (ret > 0) {
		unsigned char buf[1024 * 1024];
		memset(buf, 0, 1024 * 1024);
		ret = read(_input_pipe_fid, buf, 1024 * 1024);
		//    std::cout << "INFO: read " << ret << " characters" << std::endl;
		if (ret == 960) { // TODO probably image, for now, don't test, just assume image
			lcd().image(buf, ret);
		} else {
			std::string buffer = reinterpret_cast<const char*>(buf);
			std::vector<std::string> lines;
			boost::split(lines, buffer, boost::is_any_of("\n\r"));
			//      std::cout << "INFO: lines: " << lines.size() << std::endl;
			BOOST_FOREACH(std::string const &cmd, lines) {
				std::vector<std::string> command_comment;
				boost::split(command_comment, cmd, boost::is_any_of("#"));
				//        std::cout << "INFO: command [" << command.size() << "]: " << command[0] << " (" << command[0].size() << ")" << std::endl;
				if (command_comment.size() > 0 && command_comment[0] != std::string("")) {
					cout << "command: " << command_comment[0] << endl;
					command(command_comment[0].c_str());
				}
			}
		}
	}
}

G13_Device::G13_Device(G13_Manager &manager, libusb_device_handle *handle,
		int _id) :
		_manager(manager),
		_lcd(*this),
		_stick(*this),
		handle(handle),
		_id_within_manager(_id),
		_uinput_fid(-1),
		ctx(0)
{
	_current_profile = ProfilePtr(new G13_Profile(*this));
	_profiles["default"] = _current_profile;



	for (int i = 0; i < sizeof(keys); i++)
		keys[i] = false;

	lcd().image_clear();

	_init_fonts();
}

FontPtr G13_Device::switch_to_font(const std::string &name) {
	FontPtr rv = _fonts[name];
	if (rv) {
		_current_font = rv;
	}
	return rv;
}

void G13_Device::switch_to_profile(const std::string &name) {
	_current_profile = profile(name);

}

ProfilePtr G13_Device::profile(const std::string &name) {
	ProfilePtr rv = _profiles[name];
	if (!rv) {
		rv = ProfilePtr(new G13_Profile(*_current_profile));
		_profiles[name] = rv;
	}
	return rv;
}

// *************************************************************************

G13_Action::~G13_Action() {
}

G13_Action_Keys::G13_Action_Keys(G13_Device & keypad, const std::string &keys_string) :
		G13_Action(keypad)
{
	std::vector<std::string> keys;
	boost::split(keys, keys_string, boost::is_any_of("+"));

	BOOST_FOREACH(std::string const &key, keys) {
		auto kval = manager().find_input_key_value(key);
		if( kval == BAD_KEY_VALUE ) {
			throw G13_CommandException("create action unknown key : " + key);
		}
		_keys.push_back(kval);
	}

	std::vector<int> _keys;
}

G13_Action_Keys::~G13_Action_Keys() {
}

void G13_Action_Keys::act(G13_Device &g13, bool is_down) {
	if (is_down) {
		for (int i = 0; i < _keys.size(); i++) {
			g13.send_event( EV_KEY, _keys[i], is_down);
		}
	} else {
		for (int i = _keys.size() - 1; i >= 0; i--) {
			g13.send_event( EV_KEY, _keys[i], is_down);
		}

	}
}

void G13_Action_Keys::display( std::ostream &out ) const {
	out << " SEND KEYS: ";

	for( size_t i = 0; i < _keys.size(); i++ ) {
		if( i ) out << " + ";
		out << manager().find_input_key_name( _keys[i] );
	}
}

G13_Action_PipeOut::G13_Action_PipeOut(G13_Device & keypad,
		const std::string &out) :
		G13_Action(keypad), _out(out + "\n") {
}
G13_Action_PipeOut::~G13_Action_PipeOut() {
}

void G13_Action_PipeOut::act(G13_Device &kp, bool is_down) {
	if (is_down) {
		kp.write_output_pipe( _out );
	}
}

void G13_Action_PipeOut::display( std::ostream &o ) const {
	o << "WRITE PIPE : " << repr( _out );
}


G13_Action_Command::G13_Action_Command(G13_Device & keypad,
		const std::string &cmd) :
		G13_Action(keypad), _cmd(cmd) {
}
G13_Action_Command::~G13_Action_Command() {
}

void G13_Action_Command::act(G13_Device &kp, bool is_down) {
	if (is_down) {
		keypad().command(_cmd.c_str());
	}
}

void G13_Action_Command::display( std::ostream &o ) const {
	o << "COMMAND : " << repr( _cmd );
}

G13_ActionPtr G13_Device::make_action(const std::string &action) {
	if (!action.size()) {
		throw G13_CommandException("empty action string");
	}
	if (action[0] == '>') {
		return G13_ActionPtr(new G13_Action_PipeOut(*this, &action[1]));
	} else if (action[0] == '!') {
		return G13_ActionPtr(new G13_Action_Command(*this, &action[1]));
	} else {
		return G13_ActionPtr(new G13_Action_Keys(*this, action));
	}
	throw G13_CommandException("can't create action for " + action);
}

// *************************************************************************

typedef const char * CCP;
static const char *advance(CCP &source, std::string &dest) {
	const char *space = source ? strchr(source, ' ') : 0;
	if (space) {
		dest = std::string(source, space - source);
		source = space + 1;
	} else {
		dest = source;
		source = 0;
	}
	return source;
}

void G13_Device::command(char const *str) {
	int red, green, blue, mod, row, col;
	;
	char keyname[256];
	char binding[256];
	char c;
	const char *remainder = str;

	try {

		std::string cmd;
		advance(remainder, cmd);

		if (remainder) {
			if (cmd == "out") {
				lcd().write_string(remainder);
			} else if (cmd == "pos") {
				if (sscanf(str, "pos %i %i", &row, &col) == 2) {
					lcd().write_pos(row, col);
				} else {
					cerr << "bad pos : " << str << endl;
				}

			} else if (cmd == "bind") {
				std::string keyname;
				advance(remainder, keyname);
				std::string action = remainder;
				try {
					if (auto key = _current_profile->find_key(keyname)) {
						key->set_action( make_action(action) );
					} else if (auto stick_key = _stick.zone(keyname)) {
						stick_key->set_action( make_action(action) );
					} else {
						cerr << "bind " << keyname << " unknown" << endl;
					}
					//cout << "bind " << keyname << " [" << action << "]" << endl;
				} catch (const std::exception &ex) {
					cerr << "bind " << keyname << " " << action << " failed : "
							<< ex.what() << endl;
				}
			} else if (cmd == "profile") {
				switch_to_profile(remainder);
			} else if (cmd == "font") {
				switch_to_font(remainder);
			} else if (cmd == "mod") {
				set_mode_leds(atoi(remainder));
			} else if (cmd == "textmode ") {
				lcd().text_mode = atoi(remainder);
			} else if (cmd == "rgb") {
				if (sscanf(str, "rgb %i %i %i", &red, &green, &blue) == 3) {
					set_key_color(red, green, blue);
				} else {
					cerr << "rgb bad format: <" << str << ">" << endl;
				}
			} else if (cmd == "stickmode") {

				std::string mode = remainder;
#define STICKMODE_TEST( r, data, elem )							\
					if( mode == BOOST_PP_STRINGIZE(elem) ) {					\
						_stick.set_mode( BOOST_PP_CAT( STICK_, elem ) );		\
						return;													\
					} else														\

				BOOST_PP_SEQ_FOR_EACH( STICKMODE_TEST, _,
						(ABSOLUTE)(RELATIVE)(KEYS)(CALCENTER)(CALBOUNDS)(CALNORTH) ) {
					cerr << "unknown stick mode : <" << mode << ">" << endl;
				}
			} else if (cmd == "stickzone") {
				std::string operation, zonename;
				advance(remainder, operation);
				advance(remainder, zonename);
				if (operation == "add") {
					G13_StickZone *zone = _stick.zone(zonename, true);
				} else {
					G13_StickZone *zone = _stick.zone(zonename);
					if (!zone) {
						throw G13_CommandException("unknown stick zone");
					}
					if (operation == "action") {
						zone->set_action( make_action(remainder) );
					} else if (operation == "bounds") {
						double x1, y1, x2, y2;
						if (sscanf(remainder, "%lf %lf %lf %lf", &x1, &y1, &x2,
								&y2) != 4) {
							throw G13_CommandException("bad bounds format");
						}
						zone->set_bounds( G13_ZoneBounds(x1, y1, x2, y2) );

					} else if (operation == "del") {
						_stick.remove_zone(*zone);
					}

				}
			} else {
				cerr << "unknown command: <" << str << ">" << endl;
			}
		} else {
			if (cmd == "refresh") {
				lcd().image_send();
			} else if (cmd == "clear") {
				lcd().image_clear();
				lcd().image_send();
			} else {
				cerr << "unknown command: <" << str << ">" << endl;
			}

		}
	} catch (const std::exception &ex) {
		cerr << "command failed : " << ex.what() << endl;
	}
	return;

	if (sscanf(str, "rgb %i %i %i", &red, &green, &blue) == 3) {
		set_key_color(red, green, blue);
	} else if (sscanf(str, "mod %i", &mod) == 1) {
		set_mode_leds(mod);
	} else if (sscanf(str, "image %i %i", &red, &green) == 2) {
		lcd().image_test(red, green);
	} else if (sscanf(str, "write_char %c %i %i", &c, &row, &col) == 3) {
		lcd().write_char(c, row, col);
	} else if (sscanf(str, "pos %i %i", &row, &col) == 2) {
		lcd().write_pos(row, col);
	} else if (sscanf(str, "write_char %c", &c) == 1) {
		lcd().write_char(c);
	} else if (!strncmp(str, "out ", 4)) {
		lcd().write_string(str + 4);
	} else if (!strncmp(str, "clear", 5)) {
		lcd().image_clear();
		lcd().image_send();
	} else if (sscanf(str, "textmode %i", &mod) == 1) {
		lcd().text_mode = mod;
	} else if (!strncmp(str, "refresh", 6)) {
		lcd().image_send();
	} else if (sscanf(str, "profile %255s", binding) == 1) {
		switch_to_profile(binding);
	} else if (sscanf(str, "font %255s", binding) == 1) {
		switch_to_font(binding);
	} else {
		cerr << "unknown command: <" << str << ">" << endl;
	}
}

G13_Manager::G13_Manager() :
		ctx(0), devs(0) {
}




bool G13_Manager::running = true;
void G13_Manager::set_stop(int) {
	running = false;
}

std::string G13_Manager::string_config_value( const std::string &name ) const {
	try {
		return find_or_throw( _string_config_values, name );
	}
	catch( ... )
	{
		return "";
	}
}
void G13_Manager::set_string_config_value( const std::string &name, const std::string &value ) {
	std::cout << "set_string_config_value " << name << " = " << repr(value) << std::endl;
	_string_config_values[name] = value;
}

std::string G13_Manager::make_pipe_name( G13_Device *d, bool is_input ) {
	if( is_input ) {
		std::string config_base = string_config_value( "pipe_in" );
		if( config_base.size() ) {
			if( d->id_within_manager() == 0 ) {
				return config_base;
			} else {
				return config_base + "-" + boost::lexical_cast<std::string>(d->id_within_manager());
			}
		}
		return CONTROL_DIR+ "g13-" + boost::lexical_cast<std::string>(d->id_within_manager());
	} else {
		std::string config_base = string_config_value( "pipe_out" );
		if( config_base.size() ) {
			if( d->id_within_manager() == 0 ) {
				return config_base;
			} else {
				return config_base + "-" + boost::lexical_cast<std::string>(d->id_within_manager());
			}
		}

		return CONTROL_DIR+ "g13-" + boost::lexical_cast<std::string>(d->id_within_manager()) +"_out";
	}
}

int G13_Manager::run() {

	init_keynames();
	display_keys();

	ssize_t cnt;
	int ret;

	ret = libusb_init(&ctx);
	if (ret < 0) {
		cout << "Initialization error: " << ret << endl;
		return 1;
	}

	libusb_set_debug(ctx, 3);
	cnt = libusb_get_device_list(ctx, &devs);
	if (cnt < 0) {
		cout << "Error while getting device list" << endl;
		return 1;
	}

	discover_g13s(devs, cnt, g13s);
	libusb_free_device_list(devs, 1);
	cout << "Found " << g13s.size() << " G13s" << endl;
	if (g13s.size() == 0) {
		return 1;
	}

	std::cout << std::endl;

	for (int i = 0; i < g13s.size(); i++) {
		// register_g13(ctx, g13s[i]);
		g13s[i]->register_context(ctx);
	}
	signal(SIGINT, set_stop);
	if (g13s.size() > 0 && logo_filename.size()) {
		g13s[0]->write_lcd_file( logo_filename );
	}
	std::cout << "Active Stick zones " << std::endl;
	g13s[0]->stick().display_zones(std::cout);

	std::string config_fn = string_config_value( "config" );
	std::cout << "config_fn = " << config_fn << std::endl;
	if( config_fn.size() ) {
		g13s[0]->read_config_file( config_fn );
	}

	do {
		if (g13s.size() > 0)
			for (int i = 0; i < g13s.size(); i++) {
				int status = g13s[i]->read_keys();
				g13s[i]->read_commands();
				if (status < 0)
					running = false;
			}
	} while (running);
	cleanup();
}
} // namespace G13


