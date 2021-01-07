#include <Windows.h>
#include <iostream>
#include <bitset>

#include "PrinterUSBBase.h"

/*! ASCII code */
#define ASCII_EOT            0x04   /*!< EOT */
#define ASCII_ENQ            0x05   /*!< ENQ */
#define ASCII_HT             0x09   /*!< HT */
#define ASCII_LF             0x0A   /*!< LF */
#define ASCII_DLE            0x10   /*!< DLE */
#define ASCII_ESC            0x1B   /*!< ESC */
#define ASCII_GS             0x1D   /*!< GS */
#define ASCII_SP             0x20   /*!< SP */
#define ASCII_CAN            0x18   /*!< CAN */
#define ASCII_FF             0x0C   /*!< FF */
#define ASCII_DC4            0x14   /*!< DC4 */
#define ASCII_SUB            0x1A   /*!< DC4 */
#define ASCII_ACK            0x06   /*!< ACK */
#define ASCII_NACK           0x15   /*!< NACK */


int GetPrinterVersion(HANDLE handle, CHAR* version, DWORD max)
{
	char cmd[] = { ASCII_GS, 'I', 3 };
	DWORD size;

	if (WriteUSB(handle, cmd, sizeof(cmd)))
	{
		return -1;
	}

	size = max - 1;
	if (ReadUSB(handle, version, &size, 5))
	{
		return -2;
	}
	version[size] = 0;

	return 0;
}

int GetPrinterStatus(HANDLE handle, char* status)
{
	char cmd[] = { ASCII_DLE, ASCII_EOT, 0x01 };
	char resp;
	DWORD size;

	if (WriteUSB(handle, cmd, sizeof(cmd)))
	{
		return -1;
	}

	size = sizeof(resp);
	if (ReadUSB(handle, &resp, &size, 5))
	{
		return -2;
	}

	if (size != 1)
	{
		return -3;
	}

	*status = resp;

	return 0;
}


int GetPaperStatus(HANDLE handle, char* status)
{
	char cmd[] = { ASCII_DLE, ASCII_EOT, 0x04 };
	char resp;
	DWORD size;

	if (WriteUSB(handle, cmd, sizeof(cmd)))
	{
		return -1;
	}

	size = sizeof(resp);
	if (ReadUSB(handle, &resp, &size, 5))
	{
		return -2;
	}
	
	if (size != 1)
	{
		return -3;
	}

	*status = resp;

	return 0;
}

int main(int argc, char* argv[])
{
	HANDLE handle;
	std::string nome = argv[0];

	setlocale(LC_ALL, "");

	std::cout << nome.substr(nome.find_last_of("\\") + 1) + " - SOB Software - " + __DATE__ + " " + __TIME__ + "\n\n";

	if (OpenUSB(&handle))
	{
		std::cout << "Erro ao abrir a impressora!" << std::endl;
		return -1;
	}

	char version[64];
	if (GetPrinterVersion(handle, version, sizeof(version)) == 0)
	{
		std::cout << "Versão do firmware: " + std::string(version) << std::endl;
	}
	else
		std::cout << "Erro ao ler a versão!";

	char status;
	if (GetPaperStatus(handle, &status) == 0)
	{
		std::bitset <8> x(status);
		std::cout << "Estado do papel: " << x << std::endl;
		std::cout << "\t0\n\tSempre 1\n\tPouco Papel *\n\tPouco Papel *\n\tSempre 1\n\tSem papel\n\tSem papel\n\t0\n";
	}
	else
		std::cout << "Erro ao ler o estado do papel!";

	if (GetPrinterStatus(handle, &status) == 0)
	{
		std::bitset <8> x(status);
		std::cout << "Estado da impressora: " << x << std::endl;
		std::cout << "\t0\n\tSempre 1\n\tGaveta Fechada *\n\tErro ou Aviso\n\tSempre 1\n\t0\n\t0\n\tImprimindo\n";
	}
	else
		std::cout << "Erro ao ler o estado do papel!";



	if (CloseUSB(&handle))
	{
		std::cout << "Erro ao fechar a impressora!";
		return -2;
	}

	std::cout << "\n\n";

	return 0;
}