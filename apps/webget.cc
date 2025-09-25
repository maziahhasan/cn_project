#include "../util/socket.hh"   // local include
#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

void get_URL (const string &host, const string &path)
{
    TCPSocket socket;
    Address address(host, "http");
    socket.connect(address);

    socket.write("GET " + path + " HTTP/1.1\r\n");
    socket.write("Host: " + host + "\r\n");
    socket.write("Connection: close\r\n");
    socket.write("\r\n");

    while (!socket.eof()) {
        string response;
        socket.read(response);
        cout << response;
    }

    socket.close();
}

int main( int argc, char* argv[] )
{
    try {
        if ( argc != 3 ) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        const string host { argv[1] };
        const string path { argv[2] };

        get_URL(host, path);
    } catch ( const exception& e ) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
