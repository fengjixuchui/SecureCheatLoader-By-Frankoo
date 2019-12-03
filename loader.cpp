#include <iostream>
#include <string>
#include <windows.h>
#include "wininet.h"
#include <fstream>
#include <string_view>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <sstream>
#include <chrono>
#include <thread>
#pragma comment(lib, "wininet.lib")


#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/crc.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/erase.hpp>

// ref : https://stackoverflow.com/questions/28887001/lnk2038-mismatch-detected-for-runtimelibrary-value-mt-staticrelease-doesn
// ref : https://stackoverflow.com/questions/34871579/how-to-encrypt-plaintext-with-aes-256-cbc-in-php-using-openssl/34871988
// ref : https://stackoverflow.com/questions/19130498/problems-with-cryptopp-c-aes-256base64


std::string GetHwUID()
{
	HW_PROFILE_INFO hwProfileInfo;
	std::string szHwProfileGuid = "";
	if (GetCurrentHwProfileA(&hwProfileInfo) != NULL)
		szHwProfileGuid = hwProfileInfo.szHwProfileGuid;
	return szHwProfileGuid;
}

std::string GetCompUserName(bool User)
{
	std::string CompUserName = "";
	char szCompName[MAX_COMPUTERNAME_LENGTH + 1];
	char szUserName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwCompSize = sizeof(szCompName);
	DWORD dwUserSize = sizeof(szUserName);
	if (GetComputerNameA(szCompName, &dwCompSize))
	{
		CompUserName = szCompName;
		if (User && GetUserNameA(szUserName, &dwUserSize))
		{
			CompUserName = szUserName;
		}
	}
	return CompUserName;
}


#define CRYPT_KEY "jnwpwadqeesyoutxcxehfbjfdpxfiova" 


std::string random_string()
{
	std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

	std::random_device rd;
	std::mt19937 generator(rd());

	std::shuffle(str.begin(), str.end(), generator);

	return str.substr(0, 16);    // assumes 16 < number of characters in str         
}

std::string Decrypt(std::string str_in, std::string iv)
{
	const std::string key(CRYPT_KEY);

	std::string str_out;
	CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption((CryptoPP::byte*)key.c_str(), key.length(), (CryptoPP::byte*)iv.c_str());

	CryptoPP::StringSource decryptor(str_in, true,
		new CryptoPP::Base64Decoder(
			new CryptoPP::StreamTransformationFilter(decryption,
				new CryptoPP::StringSink(str_out)
			)
		)
	);

	return str_out;
}

std::string Encrypt(std::string str_in) {
	const std::string key(CRYPT_KEY);
	const std::string iv = random_string();

	std::string str_out;

	CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption((CryptoPP::byte*)key.c_str(), key.length(), (CryptoPP::byte*)iv.c_str());

	CryptoPP::StringSource encryptor(str_in, true,
		new CryptoPP::StreamTransformationFilter(encryption,
			new CryptoPP::Base64Encoder(
				new CryptoPP::StringSink(str_out),
				false
			)
		)
	);


	// do it here..
	boost::replace_all(str_out, "+", "-");
	boost::replace_all(str_out, "/", "_");
	boost::replace_all(str_out, "=", "");

	return iv + str_out;
}


std::string auth(std::string SubKey)
{
	std::string request_data = "";

	HINTERNET hIntSession = InternetOpenA("", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hIntSession == NULL)
	{
		return request_data;
	}

	HINTERNET hHttpSession = InternetConnectA(hIntSession, "127.0.0.1", INTERNET_DEFAULT_HTTP_PORT, 0, 0, INTERNET_SERVICE_HTTP, 0, NULL);
	if (hHttpSession == NULL)
	{
		return request_data;
	}

	HINTERNET hHttpRequest = HttpOpenRequestA(hHttpSession, "POST", 0
		, 0, 0, 0, INTERNET_FLAG_RELOAD, 0);
	if (hHttpRequest == NULL)
	{
		return request_data;
	}

	std::string szHeaders = ("Content-Type: application/x-www-form-urlencoded");
	LPCSTR szHeadersL = szHeaders.c_str();


	std::string Hwidstr = GetHwUID();
	std::size_t HWID_Hash = std::hash<std::string>{}(Hwidstr);
	std::string HWID_hashNumber = std::to_string(HWID_Hash);
	std::string PCName = GetCompUserName(false);

	// encrypt shit here

	
	std::string EncryptedPCName = Encrypt(PCName);
	std::string Encrypted_HWID_hashNumber = Encrypt(HWID_hashNumber);
	std::string EncryptedSubKey = Encrypt(SubKey);

	

	std::string request_auth = "";
	
	request_auth.append("pcname=" + EncryptedPCName + "&hwid=" + Encrypted_HWID_hashNumber + "&subkey=" + EncryptedSubKey);

	char szRequest[1024] = "";

	strcat_s(szRequest, request_auth.c_str());

	if (!HttpSendRequestA(hHttpRequest, szHeadersL, strlen(szHeadersL), szRequest, strlen(szRequest)))
	{
		return request_data;
	}

	CHAR szBuffer[1024] = { 0 };
	DWORD dwRead = 0;
	while (InternetReadFile(hHttpRequest, szBuffer, sizeof(szBuffer) - 1, &dwRead) && dwRead)
	{
		request_data.append(szBuffer, dwRead); 
	}

	InternetCloseHandle(hHttpRequest);
	InternetCloseHandle(hHttpSession);
	InternetCloseHandle(hIntSession);

	return request_data;
}

struct FilteredData {
	std::string first;
	std::string second;
};

// global object.
FilteredData flt;

void Filter_encryptedData(std::string Encrypted_data) {
	std::vector<std::string> strings;
	std::istringstream f(Encrypted_data);
	std::string s;
	while (getline(f, s, ':')) {
		//std::cout << s << std::endl;
		strings.push_back(s);
	}
	
	flt.first = strings[0];
	flt.second= strings[1];
}

std::string Decrypt_data(std::string decrypt_this) {
	Filter_encryptedData(decrypt_this);

	auto decrypt = Decrypt(flt.first, flt.second);
	return decrypt;
}


void Client_Responser(std::string response) {

	auto Decrypted_response = Decrypt_data(response);

	if (!Decrypted_response.compare("success") == 0) {
		std::cout << "[-]Authentication status : " << Decrypted_response.data() << std::endl;
		std::cin.get();
		system("exit");
	}
	else
	{
		std::cout << "[+]Authentication status : " << Decrypted_response.data() << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // 5 seconds i guess.
		std::system("cls"); // clears screen

		std::cout << "[~]perparing cheat..." <<std::endl;
		// load cheat done!
		std::cout << "loading cheat..." << std::endl;
		std::cin.get();
	}
}



int main() {
	SetConsoleTitleA("SecureCheatLoader-By-Frankoo");
	std::cout << "Enter your subscription key :";
	std::string key;
	std::cin >> key;
	
	auto response = auth(key);
	
	Client_Responser(response);


	std::cin.get();
}



