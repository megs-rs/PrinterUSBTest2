#ifndef PrinterUSBBase_h
#define PrinterUSBBase_h

int OpenUSB(HANDLE* usbHandle);
int CloseUSB(HANDLE* handle);
int WriteUSB(HANDLE handle, CHAR* buffer, DWORD size);
int ReadUSB(HANDLE handle, CHAR* buffer, DWORD* size, DWORD timeout);

#endif
