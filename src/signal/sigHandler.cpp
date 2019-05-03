#include "headers/sigHandler.hpp"

bool SigHandler::sigIntFlag = false;


bool SigHandler::getSigIntFlag(){return sigIntFlag;}

void SigHandler::setSigIntFlag(bool newFlag)
{
    sigIntFlag = newFlag;
}

void SigHandler::sigIntHandler(int signum)
{
    (void)signum;
    sigIntFlag = true;
    std::cout<<"\rCtr+C pressed. Exiting..."<<std::endl;
}

void SigHandler::setupSigIntHandler()
{
    // signal((int) SIGINT, SigHandler::sigIntHandler);
    sigIntFlag = false;
    struct sigaction sigIntAction;
    sigIntAction.sa_handler = SigHandler::sigIntHandler;
    sigemptyset(&sigIntAction.sa_mask);
    sigIntAction.sa_flags = 0;
    sigaction(SIGINT, &sigIntAction, NULL);
}
