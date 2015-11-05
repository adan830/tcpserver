#include <QCoreApplication>
#include <qstring.h>
#include "tcpserver/abstracttcphandler.h"
#include "tcpserver/epolltcpserver.h"
#include "tcpserver/tcpserverconfigs.h"
#include "log4qt/propertyconfigurator.h"
#include <unistd.h>
#include <list>
using namespace std;

/* This is a demo to use the TcpServer library
 *
 *
 */


/* firstly, you should realize a sub-class from AbstractTcpHandler to do your
 * real job.
 *
 */
class TcpHandlerImp: public AbstractTcpHandler
{
public:
    TcpHandlerImp(){}

    int onsocketIncome(const ClientSocketInfo &csi, const char *ip)
    {
        DECLEAR_TCPSERVER_LOGGER;
        int ret = 0;

        this->mcsis.push_back(csi); // save the ClientSocketInfo from system

        LOGGER_DEBUG("TcpHandlerImp::onsocketIncome: got socket(%1) from(%2) with ip: %3.\n", csi.socketfd(), csi.epoll(), QString::fromLatin1(ip));

        return ret;
    }

    int ondataIncome(int fd)
    {
        DECLEAR_TCPSERVER_LOGGER;
        int ret = 0;
        char buff[1024];

        ret = read(fd, buff, 1023);
        buff[ret] = '\0';
        LOGGER_DEBUG("TcpHandlerImp::ondataIncome: got data event from socket(%1): %2.\n", fd, QString::fromLatin1(buff));

        // for testing, if we receive a 'q' from the client socket, then we tell the system to close the socket.
        if(buff[0] == 'q')
        {
            list<ClientSocketInfo>::iterator iter;
            for(iter = this->mcsis.begin();iter != this->mcsis.end(); iter++)
            {
                if(iter->socketfd() == fd)
                {
                    this->stopSocketToClose(*iter);
                    break;
                }
            }
        }

        return ret;
    }

    int onsocketClose(int fd)
    {
        DECLEAR_TCPSERVER_LOGGER;
        int ret = 0;

        LOGGER_DEBUG("TcpHandlerImp::onsocketClose: socket(%1) close.\n", fd);

        // release the resource bind to this client.
        list<ClientSocketInfo>::iterator iter;
        for(iter = this->mcsis.begin();iter != this->mcsis.end();)
        {
            if(iter->socketfd() == fd)
            {
                close(fd);
                iter = this->mcsis.erase(iter);
                break;
            }
            else
            {
                iter++;
            }
        }

        return ret;
    }

private:
    list<ClientSocketInfo> mcsis;
};

/* finally, use the sub-class to initialize the EpollTcpServer, and then
 * start the server, the job you defined in the sub-class would be run in
 * threads.
 *
 */
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Log4Qt::PropertyConfigurator::configure(argv[1]);

    TcpHandlerImp thi;
    EpollTcpServer ets;

    ets.init(2223, &thi, 4);
    ets.startServer();

    return a.exec();
}
