#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <wchar.h>

#define SERVER_PORT 8080
#define STARTUP_TIMEOUT_MS 10000
#define RETRY_INTERVAL_MS 100

static int service_available(void) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return 0;
    }

    struct sockaddr_in address;
    ZeroMemory(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(SERVER_PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int connected = connect(sock, (struct sockaddr *)&address, sizeof(address)) == 0;
    closesocket(sock);
    return connected;
}

static int get_portable_root(wchar_t *root, size_t root_len) {
    DWORD copied = GetModuleFileNameW(NULL, root, (DWORD)root_len);
    if (copied == 0 || copied >= root_len) {
        return 0;
    }

    wchar_t *slash = wcsrchr(root, L'\\');
    if (!slash) {
        return 0;
    }

    *slash = L'\0';
    return 1;
}

static int start_server(const wchar_t *root) {
    wchar_t executable[MAX_PATH];
    wchar_t command_line[MAX_PATH + 4];

    if (swprintf(executable, MAX_PATH, L"%ls\\backend\\server.exe", root) < 0) {
        return 0;
    }
    if (GetFileAttributesW(executable) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(NULL, L"Missing backend\\server.exe.", L"Stationery System", MB_OK | MB_ICONERROR);
        return 0;
    }
    if (swprintf(command_line, MAX_PATH + 4, L"\"%ls\"", executable) < 0) {
        return 0;
    }

    STARTUPINFOW startup;
    PROCESS_INFORMATION process;
    ZeroMemory(&startup, sizeof(startup));
    ZeroMemory(&process, sizeof(process));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;

    if (!CreateProcessW(
            executable,
            command_line,
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW,
            NULL,
            root,
            &startup,
            &process)) {
        MessageBoxW(NULL, L"Could not start backend\\server.exe.", L"Stationery System", MB_OK | MB_ICONERROR);
        return 0;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return 1;
}

static int wait_for_server(void) {
    for (int elapsed = 0; elapsed < STARTUP_TIMEOUT_MS; elapsed += RETRY_INTERVAL_MS) {
        if (service_available()) {
            return 1;
        }
        Sleep(RETRY_INTERVAL_MS);
    }
    return service_available();
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command) {
    (void)instance;
    (void)previous;
    (void)command_line;
    (void)show_command;

    wchar_t root[MAX_PATH];
    if (!get_portable_root(root, MAX_PATH) || !SetCurrentDirectoryW(root)) {
        MessageBoxW(NULL, L"Could not locate the portable folder.", L"Stationery System", MB_OK | MB_ICONERROR);
        return 1;
    }

    WSADATA winsock;
    if (WSAStartup(MAKEWORD(2, 2), &winsock) != 0) {
        MessageBoxW(NULL, L"Could not initialize Windows networking.", L"Stationery System", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!service_available() && !start_server(root)) {
        WSACleanup();
        return 1;
    }

    if (!wait_for_server()) {
        WSACleanup();
        MessageBoxW(NULL, L"The backend did not start within 10 seconds.", L"Stationery System", MB_OK | MB_ICONERROR);
        return 1;
    }

    WSACleanup();

    wchar_t skip_browser[8];
    if (GetEnvironmentVariableW(L"STATIONERY_SKIP_BROWSER", skip_browser, 8) == 0) {
        ShellExecuteW(NULL, L"open", L"http://localhost:8080", NULL, NULL, SW_SHOWNORMAL);
    }

    return 0;
}
