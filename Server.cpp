#include "thread.h"
#include "socketserver.h"
#include <list>
#include <vector>
#include <algorithm>

using namespace Sync;

class SocketThread: public Thread {
private:
    Socket& socket;
    ByteArray data;
    bool &terminate;
    std::vector<SocketThread*> &sockThrHolder;
public:
    SocketThread(Socket& socket, bool &terminate, std::vector<SocketThread*> &clientSockThr)
            : socket(socket), terminate(terminate), sockThrHolder(clientSockThr)
    {}

    ~SocketThread() {
        this->terminationEvent.Wait();
    }

    Socket& GetSocket() {
        return socket;
    }

    virtual long ThreadMain() {
        try {
            while(!terminate) {
                socket.Read(data);
                // Manipulate the data, take the string and make it uppercase
                std::string res = data.ToString();
                std::for_each(res.begin(), res.end(), [](char & res) {
                    res = ::toupper(res);
                });

                // If DONE is received from the client, terminate the connection
                if (res=="DONE") {
                    sockThrHolder.erase(std::remove(sockThrHolder.begin(), sockThrHolder.end(), this), sockThrHolder.end());
                    terminate = true;
                    break;
                }

                res.append("-received"); // Append received at the end of the string
                // Send back to client
                socket.Write(ByteArray(res));
            }

        } catch (std::string &s) {
            std::cout<<s<<std::endl;
        }

        catch (std::string err) {
            std::cout<<err<<std::endl;
        }
        std::cout<<"Client Disconnected" <<std::endl;
        return 0;
    }
};

class ServerThread: public Thread {
private:
    SocketServer& server;
    bool terminate = false;
    std::vector<SocketThread*> sockThrHolder;
public:
    ServerThread(SocketServer& server)
            : server(server)
    {}

    ~ServerThread() {
        // Cleanup
        for (auto thread: sockThrHolder) {
            try {
                Socket& toClose = thread->GetSocket();
                toClose.Close();
            } catch (...) {

            }
        } std::vector<SocketThread*>().swap(sockThrHolder);
        std::cout<<"Closing client from server"<<std::endl;
        terminate = true;
    }

    virtual long ThreadMain() {
        while (true) {
            try {
                // Wait for a client socket
                Socket* newConnection = new Socket(server.Accept());
                ThreadSem serverBlock(1);

                // Pass a reference to this pointer
                Socket& socketReference = *newConnection;
                sockThrHolder.push_back(new SocketThread(socketReference, terminate, std::ref(sockThrHolder)));
            } catch (std::string error) {
                std::cout << "ERROR: " << error << std::endl;
                return 1;
            }

            catch (TerminationException terminationException) {
                std::cout << "Server has shut down!" << std::endl;
                return terminationException;
            }
        }
        return 0;
    }
};

int main(void)
{
    std::cout << "I am a server." << std::endl;
    std::cout << "Press enter to terminate server: "<<std::endl;
    std::cout.flush();

    // Create server on port 3000
    SocketServer server(3000);

    // Thread to perform server operations
    ServerThread serverThread(server);

    // Wait for input to shutdown the server
    FlexWait cinWaiter(1, stdin);
    cinWaiter.Wait();
    std::cin.get();

    // Shut down and clean up the server
    server.Shutdown();
}
