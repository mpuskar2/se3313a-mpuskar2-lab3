#include "socket.h"
#include <iostream>
#include "thread.h"

using namespace Sync;

class ClientThread: Thread {
private:
    // Connected socket
    Socket& socket;
    bool &active;
    // Data
    ByteArray data;
    std::string data_str;
public:
    ClientThread(Socket& socket, bool &active)
            : socket(socket), active(active)
    {}

    ~ClientThread() {}

    virtual long ThreadMain() {
        while (true) {
            try {
                std::cout << "Please input your data (done to exit): ";
                std::cout.flush();

                std::getline(std::cin, data_str);
                data = ByteArray(data_str);

                if(data_str == "done") { // If done is typed, terminate connection
                    active = false;
                    break;
                }

                // Write to server
                socket.Write(data);

                // Get response from server
                int connection = socket.Read(data);

                if(connection == 0) {
                    active = false;
                    break;
                }

                std::cout<<"Server Response: " << data.ToString() << std::endl;
            } catch (std::string err) {
                std::cout<<err<<std::endl;
            }
        }
        return 0;
    }
};

int main(void)
{
	std::cout << "I am a client" << std::endl;
    bool active = true;

    Socket socket("127.0.0.1", 3000); // Create the socket
    ClientThread clientThread(socket, active); // Create a new client thread with the specified socket
    socket.Open(); // Open the socket

    while (active) { // While thread is alive refresh every 1 ms, active gets updated in ClientThread-> threadmain
        sleep(1);
    }
    socket.Close();

    return 0;
}
