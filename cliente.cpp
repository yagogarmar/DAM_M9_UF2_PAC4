// cliente.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

// Enlazar con la librería Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

const int PORT = 9111;

int main() {
    // Inicializar Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup falló: " << iResult << std::endl;
        return 1;
    }

    // Crear el socket del cliente
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error al crear el socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Configurar la dirección del servidor (en este ejemplo se conecta a localhost)
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Dirección no válida o el servidor no está disponible." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Conectar con el servidor
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error al conectar: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << ">> Conexión establecida con el servicio." << std::endl;
    std::cout << ">> Introduce tu orden: ";
    std::string command;
    std::getline(std::cin, command);

    // Enviar la orden al servidor
    if (send(sock, command.c_str(), command.size(), 0) == SOCKET_ERROR) {
        std::cerr << "Error al enviar la orden: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    std::cout << ">> Orden enviada al servicio." << std::endl;

    // Recibir la respuesta (el identificador asignado)
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE] = {0};
    int bytesReceived = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Error al recibir la respuesta: " << WSAGetLastError() << std::endl;
    } else {
        std::string orderId(buffer);
        std::cout << ">> Identificador recibido: " << orderId << std::endl;
    }

    // Cerrar el socket y limpiar Winsock
    closesocket(sock);
    WSACleanup();
    return 0;
}
