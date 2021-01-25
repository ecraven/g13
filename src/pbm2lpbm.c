#include <iostream>
#include <string>
#include <string.h>
#include <cstdio>
using namespace std;
// convert a .pbm raw file to our custom .lpbm format

int main(int argc, char *argv[]) {
  unsigned char c;
  const int LEN = 256;
  char s[LEN];
  cin.getline(s,LEN);
  if(strncmp(s,"P4",2)) {
    cerr << "input file is not .pbm (P4)" << endl;
    return -1;
  }
  cin.getline(s,LEN);
  while(s[0] == '#' || s[0] == ' ')
    cin.getline(s,LEN);
  unsigned int w=0, h=0;
  if(std::sscanf(s,"%d %d", &w, &h) != 2) {
    cerr << "height and width not found" << endl;
    return -1;
  }
  if(w != 160 || h != 43) {
    cerr << "incorrect width / height, mandated: 160x43, found: " << w << "x" << h << endl;
    return -1;
  }
  cin >> noskipws;
  int i = 0, row = -1;
  unsigned char buf[160*48];
  memset(buf, 0, 160*43);
  while(cin >> c) {
    if(i%20 == 0)
      row++;
    if(row == 8)
      row = 0;
    buf[7+(i%20)*8+i/160*160] |= ((c >> 0) & 0x01) << row;
    buf[6+(i%20)*8+i/160*160] |= ((c >> 1) & 0x01) << row;
    buf[5+(i%20)*8+i/160*160] |= ((c >> 2) & 0x01) << row;
    buf[4+(i%20)*8+i/160*160] |= ((c >> 3) & 0x01) << row;
    buf[3+(i%20)*8+i/160*160] |= ((c >> 4) & 0x01) << row;
    buf[2+(i%20)*8+i/160*160] |= ((c >> 5) & 0x01) << row;
    buf[1+(i%20)*8+i/160*160] |= ((c >> 6) & 0x01) << row;
    buf[0+(i%20)*8+i/160*160] |= ((c >> 7) & 0x01) << row;
    i++;
  }
  if(i != 160*43/8) {
    cerr << "wrong number of bytes, expected " << 160*43/8 << ", got " << i << endl;
  }
  for(int i = 0; i < 160*48/8;i++) {
    cout << hex << (char)buf[i];
  }
}
