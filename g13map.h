#ifndef _g13map_
#define _g13map_ 1
#include <map>
#include <string>
#include <vector>
#include <list>

/** This class maps between a single key pressed on the G13 and one or
 *  key presses which will be generated for passing via the uinput
 *  interface.
 */
class g13map {
public:

  // ctor: create a map capable of holding g13_num_keys number of mappings
  g13map(const int g13_num_keys);

  // dtor
  virtual ~g13map();

  // add a mapping between g13key and the to-be decoded normal keyboard
  // mappings in cmd (cmd holds the full bind command line, e.g.
  // 'bind G12 KEY_U') - use the look up table lut to map between the strings
  // found in cmd and key numbers
  virtual bool bind(const int g13key, const char* cmd,
                    const std::map<std::string, int>& lut);

  // return the current mapping for the G13 key g13key as a list of one or
  // more keys
  virtual const std::list<int>& mapping(const int g13key) const;

private:

  // the mapping between the G13 key and a list of one or more key codes
  // for each g13key
  std::vector<std::list<int> > m_bind;

  // a list to return when the mapping has not been set for a key
  // (contains KEY_A)
  std::list<int> m_keya;

  // disabled
  g13map();

};

#endif // _g13map_
