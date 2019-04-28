#include <signal.h>
#include <cstddef>
#include <iostream>

class SigHandler
{
private:
    static bool sigIntFlag;

public:
    SigHandler(){};
    ~SigHandler(){};

    static bool getSigIntFlag();
    static void setSigIntFlag(bool newFlag);
    static void sigIntHandler(int signum);
    void setupSigIntHandler();

};