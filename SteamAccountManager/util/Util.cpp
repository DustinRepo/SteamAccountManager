#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "Util.h"
#include <shobjidl.h> // File dialog

#include <locale> //convert wstr to str
#include <codecvt> //convert wstr to str

std::string Util::xor_string(std::string input, std::string key) {
	int input_length = strlen(input.c_str());
	int key_length = strlen(key.c_str());

	//Extend the key until it matches the input length if it's shorter
	//Just loops each char in the key and places a new one at the end, copying the string until it matches the length
	int i = 0;
	while (input_length > key_length) {
		if (i > key_length)
			i = 0;
		key += key[i];
		i++;
		key_length++;
	}
	std::string output{};
	//Xor the string with the key
	for (int i = 0; i < input_length; i++) {
		output += input[i] ^ key[i];
	}
	return output;
}

std::string ws2s(const std::wstring& wstr) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

std::string Util::open_file_dialog() {
    //Create instance
    IFileDialog* file_dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&file_dialog));
    if (!SUCCEEDED(hr))
        return std::string("");
    //Set filter
    COMDLG_FILTERSPEC filter_spec[1] = { {L"JSON files", L"*.json"} };
    file_dialog->SetFileTypes(1, filter_spec);
    //Allow it to create a file if it doesn't exist
    FILEOPENDIALOGOPTIONS dialog_options;
    file_dialog->GetOptions(&dialog_options);
    dialog_options &= FOS_PATHMUSTEXIST;
    dialog_options &= FOS_FILEMUSTEXIST;
    file_dialog->SetOptions(dialog_options);
    //Show window
    IFileDialogEvents* file_dialog_events = nullptr;
    hr = file_dialog->Show(NULL);
    if (!SUCCEEDED(hr))
        return std::string("");
    //Get file name
    IShellItem* shell_item = nullptr;
    hr = file_dialog->GetResult(&shell_item);
    if (!SUCCEEDED(hr))
        return std::string("");
    PWSTR file_path = (PWSTR)L"";
    shell_item->GetDisplayName(SIGDN_FILESYSPATH, &file_path);

    return ws2s(file_path);
}