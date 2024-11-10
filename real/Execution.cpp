#include <iostream>
#include "Methods.h" 

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, int nCmdShow)
{
    AddToStartup();
    NotSus();
    ShowGUI();

    return 0;
}