#include "server.h"
#include <Windows.h>
#include "../../resource/tool.h"
#include <VersionHelpers.h>
using namespace std;

#define ThisInst (GetModuleHandle(NULL))

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


vector<wstring> auth_tokens;


// wWinMain function: The application will start from here.
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// TODO: Place code here.
	UNREFERENCED_PARAMETER(0);

	if (!IsWindowsVistaOrGreater()) {
		return ERROR_NOT_SUPPORTED; // Exit
	}

	CmdLineW cl(GetCommandLineW());

#if 0
	// Example command line:
	EBWebServer --webroot=./webResource/webroot/ --listen=127.0.0.1:8888 \
		--api-handler=./webResource/webapi.dll \
		--auth-type=(none|authkey|token|bearer) \
		--tokens=aaaaa|bbbbb|ccccc --bearer-tokens=user:password|aaa:bbb \
		--start-signal=1234 --stop-signal=5678 \
		--ssl=cert.cer|cert.pem
	;
#endif

	if (cl.getopt(L"help")) {
		wfstream fp(GetProgramDirW() + L".help", ios::out);
		fp <<
			L"EBWebServer Help Document\n"
			L"=========================\n"

			L"\nUsage:\n"
			L"\tEBWebServer \
		--webroot=<WebRootDir>\
		<--listen=address:port|--listen6=addressv6:port> \
		--api-handler=<API Handler File> \
		--auth-type=(none|authkey|token|bearer) \
		<(--tokens=token1|token2|...|tokenN)|(--bearer-tokens=user1:password1|...|userN:passwordN)> \
		[--start-signal=<decimal_integer>] [--stop-signal=<decimal_integer>] \
		[--ssl=<PublicCert>|<PrivatePEM>]"
			
			L"\nExample:\n"
			L"\tEBWebServer --webroot=./webResource/webroot/ --listen=127.0.0.1:8888 \
		--api-handler=./webResource/webapi.dll \
		--auth-type=(none|authkey|token|bearer) \
		--tokens=aaaaa|bbbbb|ccccc --bearer-tokens=user:password|aaa:bbb \
		--start-signal=1234 --stop-signal=5678 \
		--ssl=cert.cer|cert.pem"
			<< endl;

		Process.StartOnly(L"notepad \"" + GetProgramDirW() + L".help\"");
		return 0;
	}

	wstring cfgWebRoot, cfgListenOn, cfgApiHandler, cfgAuthType,
		cfgAuthTokens, cfgSigStart, cfgSigStop, cfgSSL;
	cl.getopt(L"webroot", cfgWebRoot);
	if (!cl.getopt(L"listen6", cfgListenOn))
		cl.getopt(L"listen", cfgListenOn);
	cl.getopt(L"api-handler", cfgApiHandler);
	cl.getopt(L"auth-type", cfgAuthType);
	cl.getopt(L"start-signal", cfgSigStart);
	cl.getopt(L"stop-signal", cfgSigStop);
	cl.getopt(L"ssl", cfgSSL);

	// 参数校验
	if (cfgWebRoot.empty() || cfgListenOn.empty() || cfgAuthType.empty()) {
		return ERROR_INVALID_PARAMETER;
	}
	if (cfgAuthType != L"none") {
		if (cfgAuthType == L"authkey") cl.getopt(L"authkey", cfgAuthTokens);
		else if (cfgAuthType == L"token") cl.getopt(L"tokens", cfgAuthTokens);
		else if (cfgAuthType == L"bearer") cl.getopt(L"bearer-tokens", cfgAuthTokens);
		else return ERROR_INVALID_PARAMETER;
		if (cfgAuthType == L"authkey") auth_tokens.push_back(cfgAuthTokens);
		else str_split(cfgAuthTokens, L"|", auth_tokens);
	}

	string s_webroot = ConvertUTF16ToUTF8(cfgWebRoot);
	string listen_addr; unsigned short listen_port;
	{
		vector<wstring>cfgListenOn2;
		str_split((cfgListenOn), L":", cfgListenOn2);
		if (cfgListenOn2.size() < 2) return ERROR_INVALID_PARAMETER;
		for (size_t i = 0, l = cfgListenOn2.size() - 1; i < l; ++i) {
			if (i) listen_addr.append(":");
			listen_addr.append(ConvertUTF16ToUTF8(cfgListenOn2[i]));
		}
		listen_port = (decltype(listen_port))atoi(ConvertUTF16ToUTF8(
			cfgListenOn2[cfgListenOn2.size() - 1]).c_str());
		if (listen_addr.empty() || listen_port == 0) return ERROR_INVALID_PARAMETER;
	}

	// register app
	std::shared_ptr<server::FileServer> srv(new server::FileServer);
	auto& app = drogon::app();
	app.setLogPath("./")
		.setLogLevel(trantor::Logger::kWarn)
		.setDocumentRoot(s_webroot)
		.addALocation("/")
		//.setUploadPath(s_upload)
		.setThreadNum(8)
		//.setClientMaxBodySize(2147483648)
		//.setClientMaxMemoryBodySize(67108864)
		.registerController(srv);
	bool useSSL = false;
	if (!cfgSSL.empty()) {
		useSSL = true;
		vector<string> raw;
		str_split(ConvertUTF16ToUTF8(cfgSSL), "|", raw);
		if (raw.size() < 2) return ERROR_INVALID_PARAMETER;
		string sslKey = raw[0], sslCert = raw[1];
		if (sslKey.empty()) sslKey = sslCert;
		app.setSSLFiles(sslCert, sslKey);
	}
	app.addListener(listen_addr, listen_port, useSSL);

	if (!cfgSigStop.empty()) {
		HANDLE signal = (HANDLE)(LONG_PTR)atoll(ws2s(cfgSigStop).c_str());
		if (signal) {
			HANDLE hThread = CreateThread(0, 0, [](PVOID signal)->DWORD {
				HANDLE sign = (HANDLE)signal;
				WaitForSingleObject(sign, INFINITE);
				drogon::app().quit();
				return 0;
				}, signal, 0, 0);
			if (hThread) CloseHandle(hThread);
		}
	}
	if (!cfgSigStart.empty()) {
		HANDLE signal = (HANDLE)(LONG_PTR)atoll(ws2s(cfgSigStart).c_str());
		if (signal) {
			SetEvent(signal);
		}
	}

	app.run();
	return 0;


	return ERROR_INVALID_PARAMETER;
	return 0;
}


