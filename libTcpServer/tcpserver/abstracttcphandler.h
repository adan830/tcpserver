#ifndef ABSTRACTTCPHANDLER_H
#define ABSTRACTTCPHANDLER_H

#include "epollinterface.h"
#include <vector>
using namespace std;

/* this is a abstract class for you to inherit to do your real job.
 * all the methods defined here may be(it depends on the threadCount
 * you initialized the EpollTcpServer) run in seperate threads, so
 * please keep the methods thread-safe yourself.
 *
 */
class AbstractTcpHandler
{
public:
    AbstractTcpHandler(){}
    virtual ~AbstractTcpHandler()
    {
    }

    /* when a new client income, this method will be called.
     */
    virtual int onsocketIncome(const ClientSocketInfo &csi, const char *ip) = 0;

    /* when a bundle of data income, this method will be called.
     */
    virtual int ondataIncome(int fd) = 0;

    /* when the system detected a socket is close by the client side,
     * this method will be called.
     */
    virtual int onsocketClose(int fd) = 0;

    /* will be call when the TcpHandlerThread was initialized by EpollTcpServer.
     */
    virtual int registerEpollInterface(EpollInterface *interface)
    {
        this->mepollInterfaces.push_back(interface);

        return 0;
    }

protected:
    /* if you want to stop the connection from the server side, call this
     * method, to let the system stop receiving any data first.
     * if this method called, the onsocketClose() method will be called by
     * system, after it stop catch read events.
     */
    int stopSocketToClose(const ClientSocketInfo &csi)
    {
        for(unsigned int i = 0; i < this->mepollInterfaces.size(); ++i)
        {
            this->mepollInterfaces[i]->stopSocketToClose(csi);
        }

        return 0;
    }

protected:
    vector<EpollInterface *> mepollInterfaces;
};

#endif // ABSTRACTTCPHANDLER_H
