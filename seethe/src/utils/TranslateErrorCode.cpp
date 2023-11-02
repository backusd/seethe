#include "TranslateErrorCode.h"
#include "utils/String.h"

namespace seethe
{
std::string TranslateErrorCode(HRESULT hr) noexcept
{
	std::wstring errorString;

	wchar_t* pMsgBuf = nullptr;
	// windows will allocate memory for err string and make our pointer point to it
	DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&pMsgBuf),
		0,
		nullptr
	);

	// 0 string length returned indicates a failure
	if (nMsgLen == 0)
	{
		errorString = L"Unidentified error code";
	}
	else
	{
		// copy error string from windows-allocated buffer to std::string
		errorString = pMsgBuf;
		// free windows buffer
		LocalFree(pMsgBuf);
	}

	return ws2s(errorString);
}


}