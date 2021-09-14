#include<Windows.h>
#include<stdio.h>

#include<d3d11.h>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"USER32.lib")
#pragma comment(lib,"GDI32.lib")
#pragma comment(lib,"KERNEL32.lib")

#define WIN_WIDTH 600
#define WIN_HEIGHT 800

LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);

FILE *gpFile = NULL;
char gszLogFileName[] = "Log.txt";

HWND ghwnd = NULL;

DWORD dwStyle;
WINDOWPLACEMENT wpPrev = {sizeof(WINDOWPLACEMENT)};

bool gbActiveWindow = false;
bool gbEscapeKeyIsPressed = false;
bool gbFullscreen = false;

ID3D11DeviceContext *gpID3D11DeviceContext = NULL;
IDXGISwapChain *gpIDXGISwapChain = NULL;
ID3D11Device *gpID3D11Device = NULL;
ID3D11RenderTargetView *gpID3D11RenderTargetView = NULL;

float gClearColor[4];

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdLine,int iCmdSHow)
{
    HRESULT initialize();
    void display();
    void uninitialize();

    WNDCLASSEX wndclassex;
    HWND hwnd;
    TCHAR szClassName[] = TEXT("MyD3D11 Class");
    bool bDone = false;
    MSG msg;

    if(fopen_s(&gpFile,gszLogFileName,"w")!=0)
    {
        MessageBox(NULL,TEXT("Log File Can Not Be Created...Exiting"),TEXT("ERROR"),MB_ICONERROR);
        exit(0);
    }
    else
    {
        fprintf_s(gpFile,"Log File Is Successfully Opened.\n");
        fclose(gpFile);
    }

    wndclassex.cbSize = sizeof(WNDCLASSEX);
    wndclassex.cbClsExtra = 0;
    wndclassex.cbWndExtra = 0;
    wndclassex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndclassex.hCursor = LoadCursor(hInstance,IDC_ARROW);
    wndclassex.hIcon = LoadIcon(hInstance,IDI_APPLICATION);
    wndclassex.hIconSm = LoadIcon(hInstance,IDI_APPLICATION);
    wndclassex.hInstance = hInstance;
    wndclassex.lpfnWndProc = WndProc;
    wndclassex.lpszClassName = szClassName;
    wndclassex.lpszMenuName = NULL;
    wndclassex.style = CS_VREDRAW | CS_HREDRAW;

    if(!RegisterClassEx(&wndclassex))
    {
        MessageBox(NULL,TEXT("Could Not Register Class...Exiting!!!"),TEXT("Error"),MB_ICONERROR);
        return 0;
    }

    hwnd = CreateWindow(szClassName,TEXT("Plain Window"),WS_OVERLAPPEDWINDOW,100,100,WIN_WIDTH,WIN_HEIGHT,NULL,NULL,hInstance,NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL,TEXT("Could Not Create Window...Exiting!!!"),TEXT("Error"),MB_ICONERROR);
        return 0;
    }

    ghwnd = hwnd;

    ShowWindow(hwnd,iCmdSHow);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    HRESULT hr;
    hr = initialize();
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"initialize() Failed...Exiting!!!.\n");
        fclose(gpFile);
        DestroyWindow(hwnd);
        hwnd = NULL;
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"initialize() Succeeded.\n");
        fclose(gpFile);
    }

    while(bDone == false)
    {
        if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
                bDone = true;
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            display();
            if(gbActiveWindow == true)
            {
                if(gbEscapeKeyIsPressed == true)
                {
                    bDone = true;
                }
            }
        }
    }

    uninitialize();

    return ((int)msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT iMsg,WPARAM wParam,LPARAM lParam)
{
    HRESULT resize(int,int);
    void ToggleFullscreen();
    void uninitialize();

    HRESULT hr;

    switch(iMsg)
    {
        case WM_ACTIVATE:
            if(HIWORD(wParam)==0)
                gbActiveWindow = true;
            else
                gbActiveWindow = false;
            break;
        case WM_ERASEBKGND:
            return(0);
        case WM_SIZE:
            if(gpID3D11DeviceContext)
            {
                hr = resize(HIWORD(lParam),LOWORD(lParam));
                if(FAILED(hr))
                {
                    fopen_s(&gpFile,gszLogFileName,"a+");
                    fprintf_s(gpFile,"resize() Failed...Exiting!!!.\n");
                    fclose(gpFile);
                    return(hr);
                }
                else
                {
                    fopen_s(&gpFile,gszLogFileName,"a+");
                    fprintf_s(gpFile,"resize() Succeeded.\n");
                    fclose(gpFile);
                }
            }
            break;
        case WM_KEYDOWN:
            switch(wParam)
            {
                case VK_ESCAPE:
                        if(gbEscapeKeyIsPressed == false)
                        {
                            gbEscapeKeyIsPressed = true;
                        }
                    break;
                case 0x46:
                        if(gbFullscreen == false)
                        {
                            ToggleFullscreen();
                            gbFullscreen = true;
                        }
                        else
                        {
                            ToggleFullscreen();
                            gbFullscreen = false;
                        }
                    break;
                default:
                    break;
            }
            break;
        case WM_LBUTTONDOWN:
            break;
        case WM_CLOSE:
            uninitialize();
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            break;
    }
    return (DefWindowProc(hwnd,iMsg,wParam,lParam));
}

void ToggleFullscreen()
{
    MONITORINFO mi;

    if(gbFullscreen == false)
    {
        dwStyle = GetWindowLong(ghwnd,GWL_STYLE);
        if(dwStyle & WS_OVERLAPPEDWINDOW)
        {
            mi = { sizeof(MONITORINFO) };
            if(GetWindowPlacement(ghwnd,&wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd,MONITORINFOF_PRIMARY),&mi))
            {
                SetWindowLong(ghwnd,GWL_STYLE,dwStyle & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(ghwnd,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_NOZORDER | SWP_FRAMECHANGED);
            }
        }
        ShowCursor(FALSE);
    }
    else
    {
        SetWindowLong(ghwnd,GWL_STYLE,dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(ghwnd,&wpPrev);
        SetWindowPos(ghwnd,HWND_TOP,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
        ShowCursor(TRUE);
    }
}

HRESULT initialize()
{
    void uninitialize();
    HRESULT resize(int,int);

    HRESULT hr;

    D3D_DRIVER_TYPE d3dDriverType;
    D3D_DRIVER_TYPE d3dDriverTypes[] = { D3D_DRIVER_TYPE_HARDWARE , D3D_DRIVER_TYPE_WARP , D3D_DRIVER_TYPE_REFERENCE };

    D3D_FEATURE_LEVEL d3dFeatureLevel_required = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL d3dFeatureLevel_acquired = D3D_FEATURE_LEVEL_10_0;

    UINT createDeviceFlags = 0;
    UINT numDriverTypes = 0;
    UINT numFeatureLevels = 1;

    numDriverTypes = sizeof(d3dDriverTypes) / sizeof(d3dDriverTypes[0]);

    DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
    ZeroMemory((void *)&dxgiSwapChainDesc,sizeof(DXGI_SWAP_CHAIN_DESC));
    dxgiSwapChainDesc.BufferCount = 1;
    dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgiSwapChainDesc.BufferDesc.Height = WIN_HEIGHT;
    dxgiSwapChainDesc.BufferDesc.Width = WIN_WIDTH;
    dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgiSwapChainDesc.OutputWindow = ghwnd;
    dxgiSwapChainDesc.SampleDesc.Count = 1;
    dxgiSwapChainDesc.SampleDesc.Quality = 0;
    dxgiSwapChainDesc.Windowed = TRUE;

    for(UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        d3dDriverType = d3dDriverTypes[driverTypeIndex];

        hr = D3D11CreateDeviceAndSwapChain(NULL,d3dDriverType,NULL,createDeviceFlags,&d3dFeatureLevel_required,numFeatureLevels,D3D11_SDK_VERSION,&dxgiSwapChainDesc,&gpIDXGISwapChain,&gpID3D11Device,&d3dFeatureLevel_acquired,&gpID3D11DeviceContext);

        if(SUCCEEDED(hr))
            break;
    }

    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"D3D11CreateDeviceAndSwapChain() Failed...Exiting!!!.\n");
        fclose(gpFile);
        return(hr);
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"D3D11CreateDeviceAndSwapChain() Succeeded.\n");

        fprintf_s(gpFile,"The Chosen Driver is Of : ");
        if(d3dDriverType == D3D_DRIVER_TYPE_HARDWARE)
        {
            fprintf_s(gpFile,"Hardware Type.\n");
        }
        else if(d3dDriverType == D3D_DRIVER_TYPE_WARP)
        {
            fprintf_s(gpFile,"Warp Type.\n");
        }
        else if(d3dDriverType == D3D_DRIVER_TYPE_REFERENCE)
        {
            fprintf_s(gpFile,"Reference Type.\n");
        }

        fprintf_s(gpFile,"The Supported Highest Level is : ");
        if(d3dFeatureLevel_acquired == D3D_FEATURE_LEVEL_11_0)
        {
            fprintf_s(gpFile,"11.0\n");
        }
        else if(d3dFeatureLevel_acquired == D3D_FEATURE_LEVEL_10_1)
        {
            fprintf_s(gpFile,"10.1\n");
        }
        else if(d3dFeatureLevel_acquired == D3D_FEATURE_LEVEL_10_0)
        {
            fprintf_s(gpFile,"10.0\n");
        }
        else
        {
            fprintf_s(gpFile,"Unknown\n");
        }
        fclose(gpFile);
    }
    gClearColor[0] = 0.0f;
    gClearColor[1] = 0.0f;
    gClearColor[2] = 1.0f;
    gClearColor[3] = 1.0f;

    hr = resize(WIN_WIDTH,WIN_HEIGHT);

    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"Resize() Failed...Exiting!!!.\n");
        fclose(gpFile);
        return(hr);
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"Resize() Succeeded.\n");
        fclose(gpFile);
    }

    return (S_OK);
}

HRESULT resize(int width,int height)
{
    HRESULT hr = S_OK;

    if(gpID3D11RenderTargetView)
    {
        gpID3D11RenderTargetView->Release();
        gpID3D11RenderTargetView = NULL;
    }

    gpIDXGISwapChain->ResizeBuffers(1,width,height,DXGI_FORMAT_R8G8B8A8_UNORM,0);

    ID3D11Texture2D *pID3D11Texture2D_BackBuffer;
    gpIDXGISwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),(LPVOID *)&pID3D11Texture2D_BackBuffer);

    hr = gpID3D11Device->CreateRenderTargetView(pID3D11Texture2D_BackBuffer,NULL,&gpID3D11RenderTargetView);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"ID3D11Device::CreateRenderTargetView() Failed...Exiting!!!.\n");
        fclose(gpFile);
        return(hr);
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"ID3D11Device::CreateRenderTargetView() Succeeded.\n");
        fclose(gpFile);
    }

    pID3D11Texture2D_BackBuffer->Release();
    pID3D11Texture2D_BackBuffer = NULL;

    gpID3D11DeviceContext->OMSetRenderTargets(1,&gpID3D11RenderTargetView,NULL);

    D3D11_VIEWPORT d3d11ViewPort;
    d3d11ViewPort.TopLeftX = 0;
    d3d11ViewPort.TopLeftY = 0;
    d3d11ViewPort.Width    = (float)width;
    d3d11ViewPort.Height   = (float)height;
    d3d11ViewPort.MinDepth = 0.0f;
    d3d11ViewPort.MaxDepth = 1.0f;

    gpID3D11DeviceContext->RSSetViewports(1,&d3d11ViewPort);
    return hr;
}

void display()
{
    gpID3D11DeviceContext->ClearRenderTargetView(gpID3D11RenderTargetView,gClearColor);

    gpIDXGISwapChain->Present(0,0);
}

void uninitialize()
{
    if(gpID3D11RenderTargetView)
    {
        gpID3D11RenderTargetView->Release();
        gpID3D11RenderTargetView = NULL;
    }

    if(gpIDXGISwapChain)
    {
        gpIDXGISwapChain->Release();
        gpIDXGISwapChain = NULL;
    }

    if(gpID3D11DeviceContext)
    {
        gpID3D11DeviceContext->Release();
        gpID3D11DeviceContext = NULL;
    }

    if(gpID3D11Device)
    {
        gpID3D11Device->Release();
        gpID3D11Device = NULL;
    }

    if(gpFile)
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"Uninitialize() Succeeded!!!.\n");
        fprintf_s(gpFile,"Log File is Successfully Closed.\n");
        fclose(gpFile);
    }
}