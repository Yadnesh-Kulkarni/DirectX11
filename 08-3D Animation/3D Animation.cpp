#include<Windows.h>
#include<stdio.h>

#include<d3d11.h>
#include<d3dcompiler.h>

#pragma warning( disable: 4838)
#include "XNAMath\xnamath.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"USER32.lib")
#pragma comment(lib,"GDI32.lib")
#pragma comment(lib,"KERNEL32.lib")
#pragma comment(lib,"D3dcompiler.lib")

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

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

ID3D11VertexShader *gpID3D11VertexShader        = NULL;
ID3D11PixelShader  *gpID3D11PixelShader         = NULL;
ID3D11Buffer       *gpID3D11Buffer_VertexBuffer_Pyramid_Position = NULL;
ID3D11Buffer       *gpID3D11Buffer_VertexBuffer_Pyramid_Color = NULL;
ID3D11Buffer       *gpID3D11Buffer_VertexBuffer_Cube_Position = NULL;
ID3D11Buffer       *gpID3D11Buffer_VertexBuffer_Cube_Color = NULL;
ID3D11InputLayout  *gpID3D11InputLayout         = NULL;
ID3D11Buffer       *gpID3D11Buffer_ConstantBuffer = NULL;
ID3D11RasterizerState *gpID3D11RasterizerState = NULL;
ID3D11DepthStencilView *gpID3D11DepthStencilView = NULL;

float pyramid_rotation = 0.0f;
float cube_rotation = 0.0f;

struct CBUFFER
{
    XMMATRIX WorldViewProjectionMatrix;
};

XMMATRIX gPerspectiveProjectionMatrix;

float gClearColor[4];

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdLine,int iCmdSHow)
{
    HRESULT initialize();
    void display();
    void uninitialize();
    void update();

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
            update();
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
    dxgiSwapChainDesc.SampleDesc.Count = 4;
    dxgiSwapChainDesc.SampleDesc.Quality = 1;
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

    //=============================================================VERTEX SHADER==========================================================

    const char *vertexShaderSourceCode = 
    "cbuffer ConstantBuffer"\
    "{"\
    "float4x4 worldViewProjectionMatrix;"\
    "}"\
    "struct vertex_output"\
    "{"\
    "float4 position : SV_POSITION;"\
    "float4 color : COLOR;"\
    "};"\
    "vertex_output main(float4 pos : POSITION,float4 col : COLOR)"\
    "{"\
    "vertex_output vout;"\
    "vout.position = mul(worldViewProjectionMatrix,pos);"\
    "vout.color = col;"\
    "return(vout);"\
    "}";

    ID3DBlob *pID3DBlob_VertexShaderCode = NULL;
    ID3DBlob *pID3DBlob_Error = NULL;

    hr = D3DCompile(vertexShaderSourceCode,
    lstrlenA(vertexShaderSourceCode)+1,
    "VS",
    NULL,
    D3D_COMPILE_STANDARD_FILE_INCLUDE,
    "main",
    "vs_5_0",
    0,
    0,
    &pID3DBlob_VertexShaderCode,
    &pID3DBlob_Error
    );

    if(FAILED(hr))
    {
        if(pID3DBlob_Error != NULL)
        {
            fopen_s(&gpFile,gszLogFileName,"a+");
            fprintf_s(gpFile,"D3DCompile() Failed For Vertex Shader : %s\n",(char *)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
            return hr;
        }
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"D3DCompile Succeded for Vertex Shader\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreateVertexShader(pID3DBlob_VertexShaderCode->GetBufferPointer(),pID3DBlob_VertexShaderCode->GetBufferSize(),NULL,&gpID3D11VertexShader);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateVertexShader() Failed \n");
        fclose(gpFile);
        pID3DBlob_VertexShaderCode->Release();
        pID3DBlob_VertexShaderCode = NULL;
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateVertexShader Succeded\n");
        fclose(gpFile);
    }

    gpID3D11DeviceContext->VSSetShader(gpID3D11VertexShader,0,0);

    //=============================================================PIXEL SHADER===================================================

    const char *pixelShaderSourceCode = 
    "float4 main(float4 pos : SV_POSITION,float4 color : COLOR): SV_TARGET"\
    "{"\
    "return(color);"\
    "}";

    ID3DBlob *pID3DBlob_PixelShaderCode = NULL;
    pID3DBlob_Error = NULL;

    hr = D3DCompile(pixelShaderSourceCode,
    lstrlenA(pixelShaderSourceCode)+1,
    "PS",
    NULL,
    D3D_COMPILE_STANDARD_FILE_INCLUDE,
    "main",
    "ps_5_0",
    0,
    0,
    &pID3DBlob_PixelShaderCode,
    &pID3DBlob_Error);
    
    if(FAILED(hr))
    {
        if(pID3DBlob_Error != NULL)
        {
            fopen_s(&gpFile,gszLogFileName,"a+");
            fprintf_s(gpFile,"D3DCompile() Failed For Pixel Shader : %s\n",(char *)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
            return hr;
        }
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"D3DCompile Succeded for Pixel Shader\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreatePixelShader(pID3DBlob_PixelShaderCode->GetBufferPointer(),pID3DBlob_PixelShaderCode->GetBufferSize(),NULL,&gpID3D11PixelShader);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreatePixelShader() Failed \n");
        fclose(gpFile);
        pID3DBlob_PixelShaderCode->Release();
        pID3DBlob_PixelShaderCode = NULL;
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreatePixelShader Succeded\n");
        fclose(gpFile);
    }

    gpID3D11DeviceContext->PSSetShader(gpID3D11PixelShader,0,0);

    pID3DBlob_PixelShaderCode->Release();
    pID3DBlob_PixelShaderCode = NULL;

    //===========================================================INITIALIZATION===============================================

    //======================TRIANGLE=====================
    float vertices_pyramid[] =
	{
		// triangle of front side
		// front-top
		0.0f, 1.0f, 0.0f,
		// front-right
		1.0f, -1.0f, -1.0f,
		// front-left
		-1.0f, -1.0f, -1.0f,

		// triangle of right side
		// right-top
		0.0f, 1.0f, 0.0f,
		// right-right
		1.0f, -1.0f, 1.0f,
		// right-left
		1.0f, -1.0f, -1.0f,

		// triangle of back side
		// back-top
		0.0f, 1.0f, 0.0f,
		// back-right
		-1.0f, -1.0f, 1.0f,
		// back-left
		1.0f, -1.0f, 1.0f,

		// triangle of left side
		// left-top
		0.0f, 1.0f, 0.0f,
		// left-right
		-1.0f, -1.0f, -1.0f,
		// left-left
		-1.0f, -1.0f, 1.0f,
	};

    float color_pyramid[] =
    {
        1.0f, 0.0f, 0.0f,
		// B
		0.0f, 0.0f, 1.0f,
		// G
		0.0f, 1.0f, 0.0f,

		// triangle of right side
		// R ( Top )
		1.0f, 0.0f, 0.0f,
		// G
		0.0f, 1.0f, 0.0f,
		// B
		0.0f, 0.0f, 1.0f,

		// triangle of back side
		// R ( Top )
		1.0f, 0.0f, 0.0f,
		// B
		0.0f, 0.0f, 1.0f,
		// G
		0.0f, 1.0f, 0.0f,

		// triangle of left side
		// R ( Top )
		1.0f, 0.0f, 0.0f,
		// G
		0.0f, 1.0f, 0.0f,
		// B
		0.0f, 0.0f, 1.0f,    
    };
    
    //Triangle Position
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc,sizeof(D3D11_BUFFER_DESC));
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(float)*ARRAYSIZE(vertices_pyramid);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = gpID3D11Device->CreateBuffer(&bufferDesc,NULL,&gpID3D11Buffer_VertexBuffer_Pyramid_Position);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateBuffer() Failed \n");
        fclose(gpFile);
        return (hr);
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateBuffer Succeded\n");
        fclose(gpFile);
    }

    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    ZeroMemory(&mappedSubResource,sizeof(D3D11_MAPPED_SUBRESOURCE));
    gpID3D11DeviceContext->Map(gpID3D11Buffer_VertexBuffer_Pyramid_Position,NULL,D3D11_MAP_WRITE_DISCARD,NULL,&mappedSubResource);
    memcpy(mappedSubResource.pData,vertices_pyramid,sizeof(vertices_pyramid));
    gpID3D11DeviceContext->Unmap(gpID3D11Buffer_VertexBuffer_Pyramid_Position,NULL);

    //======================Triangle Color============
    ZeroMemory(&bufferDesc,sizeof(D3D11_BUFFER_DESC));
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(float) * sizeof(color_pyramid);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = gpID3D11Device->CreateBuffer(&bufferDesc,NULL,&gpID3D11Buffer_VertexBuffer_Pyramid_Color);
    if(FAILED(hr))
        {
            fopen_s(&gpFile,gszLogFileName,"a+");
            fprintf_s(gpFile,"CreateBuffer() Failed \n");
            fclose(gpFile);
            return (hr);
        }
        else
        {
            fopen_s(&gpFile,gszLogFileName,"a+");
            fprintf_s(gpFile,"CreateBuffer Succeded\n");
            fclose(gpFile);
        }

    ZeroMemory(&mappedSubResource,sizeof(D3D11_MAPPED_SUBRESOURCE));
    gpID3D11DeviceContext->Map(gpID3D11Buffer_VertexBuffer_Pyramid_Color,0,D3D11_MAP_WRITE_DISCARD,0,&mappedSubResource);
    memcpy(mappedSubResource.pData,color_pyramid,sizeof(color_pyramid));
    gpID3D11DeviceContext->Unmap(gpID3D11Buffer_VertexBuffer_Pyramid_Color,0);

    //==================QUAD==================

    float vertices_cube [] =
    {
        // SIDE 1 ( TOP )
    // triangle 1
    -1.0f, +1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    -1.0f, +1.0f, -1.0f,
    // triangle 2
    -1.0f, +1.0f, -1.0f,
    +1.0f, +1.0f, +1.0f,
    +1.0f, +1.0f, -1.0f,
    
    // SIDE 2 ( BOTTOM )
    // triangle 1
    +1.0f, -1.0f, -1.0f,
    +1.0f, -1.0f, +1.0f,
    -1.0f, -1.0f, -1.0f,
    // triangle 2
    -1.0f, -1.0f, -1.0f,
    +1.0f, -1.0f, +1.0f,
    -1.0f, -1.0f, +1.0f,
    
    // SIDE 3 ( FRONT )
    // triangle 1
    -1.0f, +1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    // triangle 2
    -1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    +1.0f, -1.0f, -1.0f,
    
    // SIDE 4 ( BACK )
    // triangle 1
    +1.0f, -1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    -1.0f, -1.0f, +1.0f,
    // triangle 2
    -1.0f, -1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    -1.0f, +1.0f, +1.0f,
    
    // SIDE 5 ( LEFT )
    // triangle 1
    -1.0f, +1.0f, +1.0f,
    -1.0f, +1.0f, -1.0f,
    -1.0f, -1.0f, +1.0f,
    // triangle 2
    -1.0f, -1.0f, +1.0f,
    -1.0f, +1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    
    // SIDE 6 ( RIGHT )
    // triangle 1
    +1.0f, -1.0f, -1.0f,
    +1.0f, +1.0f, -1.0f,
    +1.0f, -1.0f, +1.0f,
    // triangle 2
    +1.0f, -1.0f, +1.0f,
    +1.0f, +1.0f, -1.0f,
    +1.0f, +1.0f, +1.0f,
    };

    float color_cube[] = 
    {
        // SIDE 1 ( TOP ) : RED
    // triangle 1 of side 1
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    // triangle 2 of side 1
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    
    // SIDE 2 ( BOTTOM ) : GREEN
    // triangle 1 of side 2
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    // triangle 2 of side 2
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    
    // SIDE 3 ( FRONT ) : BLUE
    // triangle 1 of side 3
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    // triangle 2 of side 3
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    
    // SIDE 4 ( BACK ) : CYAN
    // triangle 1 of side 4
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    // triangle 2 of side 4
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    
    // SIDE 5 ( LEFT ) : MAGENTA
    // triangle 1 of side 5
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    // triangle 2 of side 5
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    
    // SIDE 6 ( RIGHT ) : YELLOW
    // triangle 1 of side 6
    1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    // triangle 2 of side 6
    1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    };

    //Quad Position
    ZeroMemory(&bufferDesc,sizeof(D3D11_BUFFER_DESC));
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(float) * sizeof(vertices_cube);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = gpID3D11Device->CreateBuffer(&bufferDesc,NULL,&gpID3D11Buffer_VertexBuffer_Cube_Position);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateBuffer() Failed \n");
        fclose(gpFile);
        return (hr);
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateBuffer Succeded\n");
        fclose(gpFile);
    }

    ZeroMemory(&mappedSubResource,sizeof(D3D11_MAPPED_SUBRESOURCE));
    gpID3D11DeviceContext->Map(gpID3D11Buffer_VertexBuffer_Cube_Position,0,D3D11_MAP_WRITE_DISCARD,0,&mappedSubResource);
    memcpy(mappedSubResource.pData,vertices_cube,sizeof(vertices_cube));
    gpID3D11DeviceContext->Unmap(gpID3D11Buffer_VertexBuffer_Cube_Position,0);

    //Cube Color
    ZeroMemory(&bufferDesc,sizeof(D3D11_BUFFER_DESC));
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(float) * sizeof(color_cube);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = gpID3D11Device->CreateBuffer(&bufferDesc,NULL,&gpID3D11Buffer_VertexBuffer_Cube_Color);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateBuffer() Failed \n");
        fclose(gpFile);
        return (hr);
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateBuffer Succeded\n");
        fclose(gpFile);
    }

    ZeroMemory(&mappedSubResource,sizeof(D3D11_MAPPED_SUBRESOURCE));
    gpID3D11DeviceContext->Map(gpID3D11Buffer_VertexBuffer_Cube_Color,0,D3D11_MAP_WRITE_DISCARD,0,&mappedSubResource);
    memcpy(mappedSubResource.pData,color_cube,sizeof(color_cube));
    gpID3D11DeviceContext->Unmap(gpID3D11Buffer_VertexBuffer_Cube_Color,0);
    //=========================================Input Layout=================================
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[2];
    ZeroMemory(&inputElementDesc,sizeof(D3D11_INPUT_ELEMENT_DESC));
    inputElementDesc[0].SemanticName = "POSITION";
    inputElementDesc[0].SemanticIndex = 0;
    inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDesc[0].InputSlot = 0;
    inputElementDesc[0].AlignedByteOffset = 0;
    inputElementDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    inputElementDesc[0].InstanceDataStepRate = 0;

    inputElementDesc[1].SemanticName = "COLOR";
    inputElementDesc[1].SemanticIndex = 0;
    inputElementDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDesc[1].InputSlot = 1;
    inputElementDesc[1].AlignedByteOffset = 0;
    inputElementDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    inputElementDesc[1].InstanceDataStepRate = 0;

    hr = gpID3D11Device->CreateInputLayout(inputElementDesc,2,pID3DBlob_VertexShaderCode->GetBufferPointer(),pID3DBlob_VertexShaderCode->GetBufferSize(),&gpID3D11InputLayout);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateInputLayout() Failed \n");
        fclose(gpFile);
        pID3DBlob_PixelShaderCode->Release();
        pID3DBlob_PixelShaderCode = NULL;
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateInputLayout Succeded\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->IASetInputLayout(gpID3D11InputLayout);
    pID3DBlob_VertexShaderCode->Release();
    pID3DBlob_VertexShaderCode = NULL;

    D3D11_BUFFER_DESC bufferDesc_ConstantBuffer;
    ZeroMemory(&bufferDesc_ConstantBuffer,sizeof(D3D11_BUFFER_DESC));
    bufferDesc_ConstantBuffer.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc_ConstantBuffer.ByteWidth = sizeof(CBUFFER);
    bufferDesc_ConstantBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = gpID3D11Device->CreateBuffer(&bufferDesc_ConstantBuffer,nullptr,&gpID3D11Buffer_ConstantBuffer);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateBuffer() Failed for Constant Buffer\n");
        fclose(gpFile);
        return hr;
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"CreateBuffer() Succeded for Constant Buffer\n");
        fclose(gpFile);
    }

    gpID3D11DeviceContext->VSSetConstantBuffers(0,1,&gpID3D11Buffer_ConstantBuffer);

    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc,sizeof(D3D11_RASTERIZER_DESC));
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.DepthBias = 0;
    rasterizerDesc.DepthBiasClamp = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias = 0.0f;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.ScissorEnable = FALSE;

    gpID3D11Device->CreateRasterizerState(&rasterizerDesc,&gpID3D11RasterizerState);
    gpID3D11DeviceContext->RSSetState(gpID3D11RasterizerState);

    gClearColor[0] = 0.0f;
    gClearColor[1] = 0.0f;
    gClearColor[2] = 0.0f;
    gClearColor[3] = 0.0f;

    gPerspectiveProjectionMatrix = XMMatrixIdentity();

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

    if(gpID3D11DepthStencilView)
    {
        gpID3D11DepthStencilView->Release();
        gpID3D11DepthStencilView = NULL;
    }

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

    D3D11_TEXTURE2D_DESC depthDesc;
    ZeroMemory(&depthDesc,sizeof(D3D11_TEXTURE2D_DESC));
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.ArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.SampleDesc.Count = 4;
    depthDesc.SampleDesc.Quality = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;

    ID3D11Texture2D *pID3D11Texture2D_Depth = NULL;
    hr = gpID3D11Device->CreateTexture2D(&depthDesc,NULL,&pID3D11Texture2D_Depth);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"ID3D11Device::CreateTexture2D() Failed...Exiting!!!.\n");
        fclose(gpFile);
        return(hr);
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"ID3D11Device::CreateTexture2D() Succeeded.\n");
        fclose(gpFile);
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    ZeroMemory(&dsvDesc,sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

    hr = gpID3D11Device->CreateDepthStencilView(pID3D11Texture2D_Depth,&dsvDesc,&gpID3D11DepthStencilView);
    if(FAILED(hr))
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"ID3D11Device::CreateDepthStencilView() Failed...Exiting!!!.\n");
        fclose(gpFile);
        return(hr);
    }
    else
    {
        fopen_s(&gpFile,gszLogFileName,"a+");
        fprintf_s(gpFile,"ID3D11Device::CreateDepthStencilView() Succeeded.\n");
        fclose(gpFile);
    }
    pID3D11Texture2D_Depth->Release();
    pID3D11Texture2D_Depth = NULL;

    gpID3D11DeviceContext->OMSetRenderTargets(1,&gpID3D11RenderTargetView,gpID3D11DepthStencilView);

    D3D11_VIEWPORT d3d11ViewPort;
    d3d11ViewPort.TopLeftX = 0;
    d3d11ViewPort.TopLeftY = 0;
    d3d11ViewPort.Width    = (float)width;
    d3d11ViewPort.Height   = (float)height;
    d3d11ViewPort.MinDepth = 0.0f;
    d3d11ViewPort.MaxDepth = 1.0f;

    gpID3D11DeviceContext->RSSetViewports(1,&d3d11ViewPort);

    gPerspectiveProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f),(float)width/(float)height,0.1f,100.0f);
    return hr;
}

void display()
{
    gpID3D11DeviceContext->ClearRenderTargetView(gpID3D11RenderTargetView,gClearColor);
    gpID3D11DeviceContext->ClearDepthStencilView(gpID3D11DepthStencilView,D3D11_CLEAR_DEPTH,1.0f,0);
    //Triangle
    UINT stride = sizeof(float) * 3;
    UINT offset = 0;

    gpID3D11DeviceContext->IASetVertexBuffers(0,1,&gpID3D11Buffer_VertexBuffer_Pyramid_Position,&stride,&offset);
    gpID3D11DeviceContext->IASetVertexBuffers(1,1,&gpID3D11Buffer_VertexBuffer_Pyramid_Color,&stride,&offset);
    gpID3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    XMMATRIX worldMatrix = XMMatrixIdentity();
    XMMATRIX viewMatrix = XMMatrixIdentity();
    XMMATRIX rotationMatrix = XMMatrixIdentity();

    rotationMatrix = XMMatrixRotationY(pyramid_rotation);
    worldMatrix = XMMatrixTranslation(-1.5f,0.0f,12.0f);

    worldMatrix = rotationMatrix * worldMatrix;

    XMMATRIX wvpMatrix = worldMatrix * viewMatrix * gPerspectiveProjectionMatrix;

    CBUFFER constantBuffer;
    constantBuffer.WorldViewProjectionMatrix = wvpMatrix;
    gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer,0,NULL,&constantBuffer,0,0);
    gpID3D11DeviceContext->Draw(3,0);
    gpID3D11DeviceContext->Draw(3,3);
    gpID3D11DeviceContext->Draw(3,6);
    gpID3D11DeviceContext->Draw(3,9);

    //Quad
    gpID3D11DeviceContext->IASetVertexBuffers(0,1,&gpID3D11Buffer_VertexBuffer_Cube_Position,&stride,&offset);
    gpID3D11DeviceContext->IASetVertexBuffers(1,1,&gpID3D11Buffer_VertexBuffer_Cube_Color,&stride,&offset);
    gpID3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    worldMatrix = XMMatrixIdentity();
    viewMatrix = XMMatrixIdentity();
    rotationMatrix = XMMatrixIdentity();

    XMMATRIX rotationMatrixX = XMMatrixIdentity();
    XMMATRIX rotationMatrixY = XMMatrixIdentity();
    XMMATRIX rotationMatrixZ = XMMatrixIdentity();


    rotationMatrixX = XMMatrixRotationX(cube_rotation);
    rotationMatrixY = XMMatrixRotationY(cube_rotation);
    rotationMatrixZ = XMMatrixRotationZ(cube_rotation);

    rotationMatrix = rotationMatrixX * rotationMatrixY * rotationMatrixZ;

    XMMATRIX scaleMatrix = XMMatrixScaling(0.75f,0.75f,0.75f);

    worldMatrix = XMMatrixTranslation(1.5f,0.0f,12.0f);

    worldMatrix = scaleMatrix * rotationMatrix * worldMatrix;

    wvpMatrix = worldMatrix * viewMatrix * gPerspectiveProjectionMatrix;

    constantBuffer.WorldViewProjectionMatrix = wvpMatrix;
    gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer,0,NULL,&constantBuffer,0,0);
    gpID3D11DeviceContext->Draw(6,0);
    gpID3D11DeviceContext->Draw(6,6);
    gpID3D11DeviceContext->Draw(6,12);
    gpID3D11DeviceContext->Draw(6,18);
    gpID3D11DeviceContext->Draw(6,24);
    gpID3D11DeviceContext->Draw(6,30);


    gpIDXGISwapChain->Present(0,0);
}

void update()
{
    pyramid_rotation = pyramid_rotation + 0.005f;
    if(pyramid_rotation >= 360.0f)
        pyramid_rotation = 0.0f;

    cube_rotation = cube_rotation + 0.005f;
    if(cube_rotation >= 360.0f)
        cube_rotation = 0.0f;
}

void uninitialize()
{
    if(gpID3D11RasterizerState)
    {
        gpID3D11RasterizerState->Release();
        gpID3D11RasterizerState = NULL;
    }

    if(gpID3D11Buffer_ConstantBuffer)
    {
        gpID3D11Buffer_ConstantBuffer->Release();
        gpID3D11Buffer_ConstantBuffer = NULL;
    }

    if(gpID3D11InputLayout)
    {
        gpID3D11InputLayout->Release();
        gpID3D11InputLayout = NULL;
    }

    if(gpID3D11Buffer_VertexBuffer_Cube_Color)
    {
        gpID3D11Buffer_VertexBuffer_Cube_Position->Release();
        gpID3D11Buffer_VertexBuffer_Cube_Position = NULL;
    }

    if(gpID3D11Buffer_VertexBuffer_Cube_Position)
    {
        gpID3D11Buffer_VertexBuffer_Cube_Position->Release();
        gpID3D11Buffer_VertexBuffer_Cube_Position = NULL;
    }

    if(gpID3D11Buffer_VertexBuffer_Pyramid_Color)
    {
        gpID3D11Buffer_VertexBuffer_Pyramid_Color->Release();
        gpID3D11Buffer_VertexBuffer_Pyramid_Color = NULL;
    }

    if(gpID3D11Buffer_VertexBuffer_Pyramid_Position)
    {
        gpID3D11Buffer_VertexBuffer_Pyramid_Position->Release();
        gpID3D11Buffer_VertexBuffer_Pyramid_Position = NULL;
    }

    if(gpID3D11PixelShader)
    {
        gpID3D11PixelShader->Release();
        gpID3D11PixelShader = NULL;
    }

    if(gpID3D11VertexShader)
    {
        gpID3D11VertexShader->Release();
        gpID3D11VertexShader = NULL;
    }

    if(gpID3D11DepthStencilView)
    {
        gpID3D11DepthStencilView->Release();
        gpID3D11DepthStencilView = NULL;
    }

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