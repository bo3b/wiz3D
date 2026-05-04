/* wiz3D DX10 Output Test
 *
 * Minimal DX10 app that creates a DXGI factory and D3D10 device + swap chain.
 * Used to verify the dxgi.dll proxy loads S3DWrapperD3D10.dll correctly.
 *
 * Expected flow:
 *   1. CreateDXGIFactory  -> proxy -> wrapper -> hooks CreateSwapChain vtable
 *   2. D3D10CreateDevice  -> creates real device
 *   3. CreateSwapChain    -> wrapper intercepts (stereo rendering begins)
 *   4. Render loop with ClearRenderTargetView + Present
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d10.h>
#include <dxgi.h>
#include <stdio.h>
#include <math.h>
#include <conio.h>

#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "dxgi.lib")

static const wchar_t* CLASS_NAME = L"wiz3D_DX10Test";
static const wchar_t* WINDOW_TITLE = L"wiz3D DX10 Output Test";
static bool g_running = true;

static IDXGIFactory*        g_pFactory   = NULL;
static ID3D10Device*        g_pDevice    = NULL;
static IDXGISwapChain*      g_pSwapChain = NULL;
static ID3D10RenderTargetView* g_pRTV    = NULL;

static void Log(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            g_running = false;
        break;
    case WM_CLOSE:
        g_running = false;
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static BOOL InitD3D10(HWND hWnd)
{
    HRESULT hr;

    // Step 1: Create DXGI Factory — this is where our proxy intercepts
    Log("Calling CreateDXGIFactory...\n");
    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&g_pFactory);
    if (FAILED(hr))
    {
        Log("FAIL: CreateDXGIFactory returned 0x%08lX\n", hr);
        return FALSE;
    }
    Log("OK: IDXGIFactory created at %p\n", g_pFactory);

    // Enumerate adapters
    IDXGIAdapter* pAdapter = NULL;
    hr = g_pFactory->EnumAdapters(0, &pAdapter);
    if (FAILED(hr))
    {
        Log("FAIL: EnumAdapters returned 0x%08lX\n", hr);
        return FALSE;
    }

    DXGI_ADAPTER_DESC adapterDesc;
    pAdapter->GetDesc(&adapterDesc);
    Log("Adapter: %ls\n", adapterDesc.Description);

    // Step 2: Create D3D10 device
    Log("Creating D3D10 device...\n");
    hr = D3D10CreateDevice(pAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
                           D3D10_SDK_VERSION, &g_pDevice);
    pAdapter->Release();
    if (FAILED(hr))
    {
        Log("FAIL: D3D10CreateDevice returned 0x%08lX\n", hr);
        return FALSE;
    }
    Log("OK: ID3D10Device created at %p\n", g_pDevice);

    // Step 3: Create swap chain — wrapper hooks this via vtable[10]
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = 800;
    scd.BufferDesc.Height = 600;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    Log("Calling IDXGIFactory::CreateSwapChain...\n");
    hr = g_pFactory->CreateSwapChain(g_pDevice, &scd, &g_pSwapChain);
    if (FAILED(hr))
    {
        Log("FAIL: CreateSwapChain returned 0x%08lX\n", hr);
        return FALSE;
    }
    Log("OK: IDXGISwapChain created at %p\n", g_pSwapChain);

    // Create render target view
    ID3D10Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr))
    {
        Log("FAIL: GetBuffer returned 0x%08lX\n", hr);
        return FALSE;
    }

    hr = g_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRTV);
    pBackBuffer->Release();
    if (FAILED(hr))
    {
        Log("FAIL: CreateRenderTargetView returned 0x%08lX\n", hr);
        return FALSE;
    }

    g_pDevice->OMSetRenderTargets(1, &g_pRTV, NULL);

    D3D10_VIEWPORT vp = {};
    vp.Width = 800;
    vp.Height = 600;
    vp.MaxDepth = 1.0f;
    g_pDevice->RSSetViewports(1, &vp);

    Log("OK: D3D10 initialization complete\n");
    return TRUE;
}

static void Render(void)
{
    // Cycle clear color over time
    static float t = 0.0f;
    t += 0.01f;
    float color[4] = {
        0.5f + 0.5f * sinf(t),
        0.5f + 0.5f * sinf(t + 2.0f),
        0.5f + 0.5f * sinf(t + 4.0f),
        1.0f
    };
    g_pDevice->ClearRenderTargetView(g_pRTV, color);
    g_pSwapChain->Present(1, 0);
}

static void Cleanup(void)
{
    Log("Cleaning up...\n");
    if (g_pRTV)       { g_pRTV->Release();       g_pRTV = NULL; }
    if (g_pSwapChain) { g_pSwapChain->Release();  g_pSwapChain = NULL; }
    if (g_pDevice)    { g_pDevice->Release();      g_pDevice = NULL; }
    if (g_pFactory)   { g_pFactory->Release();     g_pFactory = NULL; }
    Log("Cleanup complete\n");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Allocate console for log output
    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    Log("=== wiz3D DX10 Output Test ===\n");

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowExW(0, CLASS_NAME, WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 816, 639,
        NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        Log("FAIL: CreateWindow failed\n");
        return 1;
    }

    if (!InitD3D10(hWnd))
    {
        Log("FAIL: D3D10 init failed — press any key to exit\n");
        Cleanup();
        _getch();
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);

    Log("Entering render loop (press ESC or close window to exit)...\n");

    MSG msg = {};
    while (g_running)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
                g_running = false;
        }
        if (g_running)
            Render();
    }

    Cleanup();
    Log("=== DX10 test complete ===\n");
    return 0;
}
