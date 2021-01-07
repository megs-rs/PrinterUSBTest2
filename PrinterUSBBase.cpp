#include <Windows.h>
#include <SetupAPI.h>

#define SS_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
static const GUID name \
= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

SS_DEFINE_GUID(GUID_DEVINTERFACE_USBPRINT, 0x28d78fad, 0x5a12, 0x11D1, 0xae, 0x5b, 0x00, 0x00, 0xf8, 0x03, 0xa8, 0xc2);

int OpenUSB(HANDLE *usbHandle)
{
    HDEVINFO devs;
    DWORD devcount;
    SP_DEVINFO_DATA devinfo;
    SP_DEVICE_INTERFACE_DATA devinterface;
    DWORD size;
    GUID intfce;
    PSP_DEVICE_INTERFACE_DETAIL_DATA interface_detail;
    DWORD dataType;

    *usbHandle = INVALID_HANDLE_VALUE;

    intfce = GUID_DEVINTERFACE_USBPRINT;
    devs = SetupDiGetClassDevs(&intfce, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devs == INVALID_HANDLE_VALUE) {
        // printf("Nenhuma classe USBPRINT detectada.\n");
        return -1;
    }
    devcount = 0;
    devinterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while (SetupDiEnumDeviceInterfaces(devs, 0, &intfce, devcount, &devinterface)) {
        /* The following buffers would normally be malloced to he correct size
         * but here we just declare them as large stack variables
         * to make the code more readable
         */
        WCHAR driverkey[2048];
        WCHAR interfacename[2048];
        WCHAR location[2048];

        /* If this is not the device we want, we would normally continue onto the next one
         * so something like if (!required_device) continue; would be added here
         */
        devcount++;
        size = 0;
        /* See how large a buffer we require for the device interface details */
        SetupDiGetDeviceInterfaceDetail(devs, &devinterface, 0, 0, &size, 0);
        devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
        interface_detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) calloc(1, size);
        if (interface_detail) {
            interface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
            if (!SetupDiGetDeviceInterfaceDetail(devs, &devinterface, interface_detail, size, 0, &devinfo)) {
                free(interface_detail);
                SetupDiDestroyDeviceInfoList(devs);
                //printf("Erro ao pegar informações do dispositivo!\n");
                return -2;
            }
            /* Make a copy of the device path for later use */
            wcscpy_s(interfacename, interface_detail->DevicePath);
            free(interface_detail);
            /* And now fetch some useful registry entries */
            size = sizeof(driverkey);
            driverkey[0] = 0;
            if (!SetupDiGetDeviceRegistryProperty(devs, &devinfo, SPDRP_DRIVER, &dataType, (LPBYTE)driverkey, size, 0)) {
                SetupDiDestroyDeviceInfoList(devs);
                return -3;
            }
            size = sizeof(location);
            location[0] = 0;
            if (!SetupDiGetDeviceRegistryProperty(devs, &devinfo, SPDRP_LOCATION_INFORMATION, &dataType, (LPBYTE)location, size, 0)) {
                SetupDiDestroyDeviceInfoList(devs);
                return -4;
            }
            *usbHandle = CreateFile(interfacename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
        }
        else
        {
            //printf("Memória insuficiente!\n");
            return -5;
        }
    }

    SetupDiDestroyDeviceInfoList(devs);

    if (!devcount)
    {
        //printf("Nenhuma impressora detectada!\n");
        return -99;
    }

    return !(*usbHandle != INVALID_HANDLE_VALUE);
}

int CloseUSB(HANDLE* handle)
{
    if (*handle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    if (!CloseHandle(*handle))
    {
        return -2;
    }

    *handle = INVALID_HANDLE_VALUE;

    return 0;
}

int WriteUSB(HANDLE handle, CHAR * buffer, DWORD size)
{
    if (handle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    DWORD writted;
    BOOL result;
    OVERLAPPED o = { 0 };

    o.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!(result = WriteFile(handle, buffer, size, NULL, &o)))
    {
        if (GetLastError() != ERROR_IO_PENDING) return -2;
    }

    if (!result && !GetOverlappedResult(handle, &o, &writted, TRUE))
    {
        return -3;
    }

    return !(size == writted);
}

void DoEvents()
{
    MSG msg;
    BOOL result;

    while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
    {
        result = ::GetMessage(&msg, NULL, 0, 0);
        if (result == 0) // WM_QUIT
        {
            ::PostQuitMessage((int) msg.wParam);
            break;
        }
        else if (result == -1)
        {
            // Handle errors/exit application, etc.
        }
        else
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}

int ReadUSB(HANDLE handle, CHAR * buffer, DWORD * size, DWORD timeout)
{
    if (handle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    DWORD readed, agora = GetTickCount(), lasterror;
    BOOL result;
    OVERLAPPED o = { 0 };
    
    o.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!(result = ReadFile(handle, buffer, *size, &readed, &o)))
    {
        if (GetLastError() != ERROR_IO_PENDING) return -2;
    }

    while ((agora + timeout * 1000 > GetTickCount()) && !result)
    {
        if (!(result = GetOverlappedResult(handle, &o, &readed, FALSE))) 
        {
            lasterror = GetLastError();
            if (lasterror != ERROR_IO_INCOMPLETE) return -3;
        }
        DoEvents();
    }

    if (!result)
    {
        CancelIo(handle);
    }

    *size = readed;

    return !(size > 0);
}