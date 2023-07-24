#include <iostream>
#define _WINSOCKAPI_
#include <Windows.h>
#include <mmsystem.h>
#include <thread>
#include <vector>
#include <windows.h>
#include <WinSock2.h>
#include <cmath>
#include <Ws2tcpip.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 4096; // �o�b�t�@�T�C�Y
const int PORT = 12345; // �|�[�g�ԍ�

#define M_PI 3.14159265358979323846
const int SAMPLE_RATE = 44100; // �T���v�����O���g��
const int AMPLITUDE = 10000; // �U��
const double FREQUENCY = 440.0; // ���g�� (440 Hz��A��)

void generateSinWave(short* buffer, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        double t = static_cast<double>(i) / SAMPLE_RATE;
        double value = AMPLITUDE * sin(2.0 * M_PI * FREQUENCY * t);
        buffer[i] = static_cast<short>(value);
    }
}

void recordAndSend(SOCKET clientSocket) {
    HWAVEIN hWaveIn;
    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1; // ���m����
    waveFormat.nSamplesPerSec = 44100; // �T���v�����O���g��
    waveFormat.wBitsPerSample = 16; // �T���v��������̃r�b�g��
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, WAVE_FORMAT_DIRECT);
    waveInStart(hWaveIn);

    char buffer[BUFFER_SIZE];

    while (true) {
        waveInAddBuffer(hWaveIn, reinterpret_cast<WAVEHDR*>(buffer), sizeof(WAVEHDR));
        waveInUnprepareHeader(hWaveIn, reinterpret_cast<WAVEHDR*>(buffer), sizeof(WAVEHDR));
        waveInPrepareHeader(hWaveIn, reinterpret_cast<WAVEHDR*>(buffer), sizeof(WAVEHDR));
        waveInStart(hWaveIn);

        send(clientSocket, buffer, BUFFER_SIZE, 0);
    }

    waveInClose(hWaveIn);
}

void playAndReceive(SOCKET clientSocket) {
    HWAVEOUT hWaveOut;
    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1; // ���m����
    waveFormat.nSamplesPerSec = 44100; // �T���v�����O���g��
    waveFormat.wBitsPerSample = 16; // �T���v��������̃r�b�g��
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, WAVE_FORMAT_DIRECT);
    waveOutSetVolume(hWaveOut, 0xFFFFFFFF); // ���ʂ��ő�ɐݒ�

    char buffer[BUFFER_SIZE];

    while (true) {
        recv(clientSocket, buffer, BUFFER_SIZE, 0);
        waveOutWrite(hWaveOut, reinterpret_cast<WAVEHDR*>(buffer), sizeof(WAVEHDR));
    }

    waveOutClose(hWaveOut);
}

int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    SOCKET serverSocket;
    struct sockaddr_in serverAddr;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
    serverAddr.sin_port = htons(PORT);

    result = bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        std::cerr << "Binding failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    result = listen(serverSocket, 1);
    if (result == SOCKET_ERROR) {
        std::cerr << "Listening failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "�T�[�o�[���N�����܂����B�N���C�A���g����̐ڑ���ҋ@��..." << std::endl;

    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accepting connection failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "�N���C�A���g���ڑ����܂����B" << std::endl;

    //std::thread recordThread(recordAndSend, clientSocket);
    std::thread playThread(playAndReceive, clientSocket);

    //recordThread.join();
    playThread.join();

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}