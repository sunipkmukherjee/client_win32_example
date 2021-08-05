// Dear ImGui: standalone example application for DirectX 9
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs
#define DEBUG_CONSOLE
#include "imgui/imgui.h"
#include "backend/imgui_impl_dx9.h"
#include "backend/imgui_impl_win32.h"
#define WIN32_LEAN_AND_MEAN
#include <d3d9.h>
#include <windows.h>
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

// Data
static LPDIRECT3D9 g_pD3D = NULL;
static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool done = false;
static bool conn_rdy = false;
static char recv_buf[1024];
DWORD WINAPI RecvFcn(LPVOID _in)
{
    ZeroMemory(recv_buf, sizeof(recv_buf));
    while (!done)
    {
        if (conn_rdy)
        {
            SOCKET sock = *(SOCKET *)_in;
            char msg_sz = 0;
            int sz = recv(sock, &msg_sz, 1, 0);
            if (sz > 0)
                sz = recv(sock, recv_buf, msg_sz, MSG_WAITALL);
        }
        Sleep(16); // 16 ms sleep
    }
    return NULL;
}

void ReceiveWindow(bool *active)
{
    ImGui::Begin("Receiver Window", active);
    if (conn_rdy)
    {
        ImGui::Text("Received: %s", recv_buf);
    }
    else
    {
        ImGui::Text("Connection not opened, please open socket connection...");
    }
    ImGui::End();
}

// Main code
#ifdef DEBUG_CONSOLE
int main(int, char **)
#else
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
#endif
{
    static WSADATA wsaData;
    static SOCKET ConnectSocket = INVALID_SOCKET;
    static struct addrinfo *result = NULL,
                           *ptr = NULL,
                           hints;

    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 0;
    }
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Client Example"), NULL};
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX9 + WinSock Client Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    DWORD threadId;
    HANDLE RecvThread = CreateThread(NULL, 0, RecvFcn, &ConnectSocket, 0, &threadId);
    if (RecvThread == NULL)
    {
        printf("Could not create thread\n");
    }

    // Main loop
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static bool show_readout_win = true;
        if (show_readout_win)
            ReceiveWindow(&show_readout_win);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static int port = 12376;
            static char port_str[10];
            _snprintf(port_str, sizeof(port_str), "%d", port);
            static char ipaddr[16] = "127.0.0.1";
            auto flag = ImGuiInputTextFlags_ReadOnly;
            if (!conn_rdy)
                flag = (ImGuiInputTextFlags_)0;

            ImGui::Begin("Connection Manager"); // Create a window called "Hello, world!" and append into it.

            ImGui::InputText("IP Address", ipaddr, sizeof(ipaddr), flag);
            ImGui::InputInt("Port", &port, 0, 0, flag);

            if (!conn_rdy || (ConnectSocket == INVALID_SOCKET))
            {
                if (ImGui::Button("Connect"))
                {
                    if ((iResult = getaddrinfo(ipaddr, port_str, &hints, &result)))
                    {
                        printf("getaddrinfo failed with %d\n", iResult);
                        result = NULL;
                    }
                    if (result)
                    {
                        ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
                        if (ConnectSocket == INVALID_SOCKET)
                        {
                            printf("socket failed with error: %ld\n", WSAGetLastError());
                        }
                    }
                    if ((iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen)) == SOCKET_ERROR)
                    {
                        closesocket(ConnectSocket);
                        ConnectSocket = INVALID_SOCKET;
                    }
                    else
                        conn_rdy = true;
                }
            }
            else
            {
                if (ImGui::Button("Disconnect"))
                {
                    iResult = shutdown(ConnectSocket, SD_SEND);
                    if (iResult == SOCKET_ERROR)
                    {
                        printf("shutdown failed with error %d\n", WSAGetLastError());
                    }
                    closesocket(ConnectSocket);
                    ConnectSocket = INVALID_SOCKET;
                    conn_rdy = false;
                }
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        if (conn_rdy && (ConnectSocket != INVALID_SOCKET))
        {
            static char msg[1024];
            static int ctr = 0;
            int sz = _snprintf(msg, 1024, "Hello from client: %d!\n", ++ctr);
            iResult = send(ConnectSocket, msg, sz, 0);
            if (iResult == SOCKET_ERROR)
            {
                printf("Send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                ConnectSocket = INVALID_SOCKET;
                conn_rdy = false;
            }
            else
                printf("Sent: %d bytes\n", iResult);
        }

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }
    WaitForMultipleObjects(1, &RecvThread, true, 1000); // wait a second
    CloseHandle(RecvThread);
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
    if (g_pD3D)
    {
        g_pD3D->Release();
        g_pD3D = NULL;
    }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}