#include "server.h"
#include <string>
#include "../../resource/tool.h"
using namespace drogon;

using namespace std;


struct TAuthTokenList {
	vector <string> normal;
	HWND hwnd{};
	UINT uMsgCheck{};
} AuthTokenList;

/*
VerifyAuthToken 中的数据结构的代码表示
(不完全一致)
struct TAuthInfo {
	size_t nLength;
	WCHAR str;
};
*/


void InitAuthTokenVerify(CmdLineW& cl) {
	wstring tok;
	if (!cl.getopt(L"token", tok)) return;

	if (tok.starts_with(L"Window;")) {
		/*
		--token=Window;123;456
			123: HWND 的数字形式
			456: 用于验证的自定义Message
		*/
		wstring p1, p2;
		tok.erase(0, 7);
		p1 = tok.substr(0, tok.find_first_of(L';'));
		p2 = tok.substr(tok.find_first_of(L';') + 1);
		
		// p1 是HWND    p2 是用于验证的自定义消息
		AuthTokenList.hwnd = (HWND)(LONG_PTR)(PVOID)atoll(ws2c(p1));
		AuthTokenList.uMsgCheck = (UINT)(int)atoi(ws2c(p2));

	}
	else if (tok.starts_with(L"File;")) {
		// File;C:\Path\To\File.txt
		tok.erase(0, 7);
		fstream fp(tok);
		if (fp) {
			char* buffer = new char[4096];
			std::string sbuffer;
			constexpr const char* whiteSpaces = "\r\n\u0020\t";
			const auto trimLeft = [](const std::string& s, const char* w) {
				auto startpos = s.find_first_not_of(w);
				return (startpos == s.npos) ? string() : s.substr(startpos);
			};
			const auto trimRight = [](const std::string& s, const char* w) {
				auto endpos = s.find_last_not_of(w);
				return (endpos == s.npos) ? string() : s.substr(0, endpos + 1);
			};
			const auto trim = [&trimLeft, &trimRight](const std::string& s, const char* w) {
				return trimRight(trimLeft(s, w), w);
			};
			while ((fp.getline(buffer, 4096))) {
				sbuffer = trim(buffer, whiteSpaces);
				if (sbuffer.empty()) continue;
				AuthTokenList.normal.push_back(sbuffer);
			}
		}
	}
	else {
		// token 参数就是一条字符串token
		AuthTokenList.normal.push_back(ws2s(tok));
	}

}





void server::AuthFilter::doFilter(const HttpRequestPtr& req, FilterCallback&& fcb, FilterChainCallback&& fccb)
{
	string authToken = req->getParameter("token");
	if (req->method() == Options || true) {
		return fccb();
	}
	HttpResponsePtr resp = HttpResponse::newHttpResponse();
	resp->addHeader("access-control-allow-origin", "*");
	resp->addHeader("access-control-allow-methods", req->getHeader("access-control-request-method") + ",OPTIONS");
	resp->addHeader("access-control-allow-headers", req->getHeader("access-control-request-headers"));
	resp->setStatusCode(k401Unauthorized);
	resp->setBody("");
	fcb(resp);
}






void server::FileServer::auth(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const
{
	HttpResponsePtr resp = HttpResponse::newHttpResponse();
	resp->addHeader("access-control-allow-origin", req->getHeader("origin"));
	resp->addHeader("access-control-allow-methods", "POST,OPTIONS");
	resp->addHeader("access-control-allow-headers", req->getHeader("access-control-request-headers"));
	resp->addHeader("access-control-max-age", "300");
	resp->setContentTypeCode(CT_APPLICATION_JSON);
	resp->setBody("{success:true}");
	callback(resp);
}

void server::FileServer::apis(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, std::string&& apiName, std::string&& apiParams) const
{
	MessageBoxW(NULL, ConvertUTF8ToUTF16(apiName).c_str(), ConvertUTF8ToUTF16(apiParams).c_str(), 0);
}









