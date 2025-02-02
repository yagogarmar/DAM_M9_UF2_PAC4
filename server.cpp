
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <iomanip>

#pragma comment(lib, "Ws2_32.lib")

const int PORT = 9111; // HE CAMBIADO EL PUERTO POR QUE NO ME FUNCIONABA EL DE LA PRACTICA 


std::mutex fileMutex;

std::atomic<int> orderCounter(0);


void handleClient(SOCKET clientSocket, std::string clientAddress) {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE] = {0};

    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRead <= 0) {
        std::cerr << "Error al leer de " << clientAddress << std::endl;
        closesocket(clientSocket);
        return;
    }

    std::string orderText(buffer);
    std::cout << ">> Conexión aceptada del cliente " << clientAddress << std::endl;
    std::cout << ">> Orden recibida: \"" << orderText << "\"" << std::endl;

    int orderNumber = ++orderCounter;

    std::ostringstream oss;
    oss << "ORD-" << std::setw(4) << std::setfill('0') << orderNumber;
    std::string orderId = oss.str();
    std::cout << ">> Identificador asignado: " << orderId << std::endl;

    {

        std::lock_guard<std::mutex> lock(fileMutex);
        std::ofstream outfile("comandas.txt", std::ios::app);
        if (!outfile.is_open()) {
            std::cerr << "No se pudo abrir el archivo para escribir." << std::endl;
        } else {
            outfile << orderId << ": \"" << orderText << "\"" << std::endl;
            outfile.close();
            std::cout << ">> Orden guardada en el archivo." << std::endl;
        }
    }


    send(clientSocket, orderId.c_str(), orderId.size(), 0);
    closesocket(clientSocket);
    std::cout << ">> Conexión con " << clientAddress << " cerrada." << std::endl;
}

int main() {

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup falló: " << iResult << std::endl;
        return 1;
    }


    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error al crear el socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }


    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; 
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind falló: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }


    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen falló: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << ">> Servicio iniciado. Escuchando en el puerto " << PORT << "..." << std::endl;

    std::vector<std::thread> threads;


    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error al aceptar conexión: " << WSAGetLastError() << std::endl;
            continue;
        }


        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        threads.emplace_back(std::thread(handleClient, clientSocket, std::string(clientIP)));
        threads.back().detach();

        std::cout << ">> Esperando nuevas conexiones..." << std::endl;
    }


    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
