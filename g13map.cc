#include <iostream>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <linux/uinput.h>
#include "g13map.h"

g13map::g13map(const int g13_num_keys) :
   m_bind(g13_num_keys),
   m_keya(1)
{
   std::cout << "g13map ctor"<< std::endl;
   m_keya.push_back(KEY_A); // from <linux/uinput.h>
}

g13map::~g13map()
{
   std::cout << "g13map dtor"<< std::endl;
}

bool
g13map::bind(const int g13key, const char* cmd,
             const std::map<std::string, int>& lut)
{
   assert(g13key>=0);
   assert(g13key<m_bind.size());

   m_bind[g13key].clear();

   std::vector<std::string> tok;

   boost::split(tok, cmd, boost::is_any_of(" "));

   for (int i=2; i<tok.size(); i++) {
      std::cout << "g13map::bind " << i << " <" << tok[i] << ">" << std::endl;

      if (tok[i].size() < 1 || tok[i][0] == '#')
         break;
      else if (lut.count(tok[i])==1)
         m_bind[g13key].push_back(lut.at(tok[i]));
      else {
         std::cerr << "Keyboard key not found for: " << tok[i] << std::endl;
         return false;
      }
   }

   return true;
}

const std::list<int>&
g13map::mapping(const int g13key) const
{
   if ((g13key>=0) && (g13key<m_bind.size()))

      if (m_bind[g13key].size()<1)
         return m_keya;
      else
         return m_bind[g13key];

   else {
      std::cerr << "Bad G13 key: " << g13key << std::endl;
      return m_keya;
   }
}
