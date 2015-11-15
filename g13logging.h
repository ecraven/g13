#ifndef _g13logging_
#define _g13logging_ 1

class g13logging {
public:

   // ctor
   g13logging(int argc, char* argv[]);

   // dtor
   virtual ~g13logging();

protected:

   // setup up either default logging or from config file
   virtual int init(int argc, char* argv[]);
   virtual void init();
   virtual void init(const std::string& filename);

   virtual void register_attributes();

private:

   // disabled
   g13logging();

};

#endif // _g13logging_
