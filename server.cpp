// servidor.cpp
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

// Enlazar con la librería Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

const int PORT = 9111;

// Mutex para proteger el acceso al archivo y al contador de órdenes
std::mutex fileMutex;
// Contador de órdenes (variable atómica para seguridad en multihilo)
std::atomic<int> orderCounter(0);

// Función para gestionar la conexión de un cliente
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

    // Incrementar el contador de órdenes de forma segura
    int orderNumber = ++orderCounter;

    // Formatear el identificador de la orden: ORD-000X
    std::ostringstream oss;
    oss << "ORD-" << std::setw(4) << std::setfill('0') << orderNumber;
    std::string orderId = oss.str();
    std::cout << ">> Identificador asignado: " << orderId << std::endl;

    {
        // Escribir la orden en el archivo de forma segura
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

    // Enviar el identificador al cliente
    send(clientSocket, orderId.c_str(), orderId.size(), 0);
    closesocket(clientSocket);
    std::cout << ">> Conexión con " << clientAddress << " cerrada." << std::endl;
}

int main() {
    // Inicializar Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup falló: " << iResult << std::endl;
        return 1;
    }

    // Crear el socket del servidor
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error al crear el socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Configurar la dirección del servidor
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Escucha en todas las interfaces
    serverAddr.sin_port = htons(PORT);

    // Asignar la dirección al socket (bind)
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind falló: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Poner el socket en modo escucha
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen falló: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << ">> Servicio iniciado. Escuchando en el puerto " << PORT << "..." << std::endl;

    std::vector<std::thread> threads;

    // Bucle para aceptar conexiones de clientes
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error al aceptar conexión: " << WSAGetLastError() << std::endl;
            continue;
        }

        // Obtener la dirección IP del cliente
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        // Crear un hilo para gestionar la conexión del cliente
        threads.emplace_back(std::thread(handleClient, clientSocket, std::string(clientIP)));
        threads.back().detach();

        std::cout << ">> Esperando nuevas conexiones..." << std::endl;
    }

    // Cerrar el socket del servidor y limpiar Winsock (nunca se llega aquí en este ejemplo)
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
