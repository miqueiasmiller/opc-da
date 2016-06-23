#include <string>

using namespace std;

namespace opc
{
	wchar_t * convertMBSToWCS(char const * value)
	{
		size_t newSize = strlen(value) + 1;
		size_t convertedChars = 0;

		wchar_t * converted = new wchar_t[newSize];

		mbstowcs_s(&convertedChars, converted, newSize, value, _TRUNCATE);

		return converted;
	}

	wstring convertMBSToWCS(string const * value)
	{
		wchar_t * converted = convertMBSToWCS(value->c_str());

		return wstring(converted);
	}
}