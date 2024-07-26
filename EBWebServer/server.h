#pragma once
#include <drogon/drogon.h>
#include <Windows.h>


#define CORSadd(req, resp) {\
	resp->addHeader("access-control-allow-origin", req->getHeader("origin"));\
	resp->addHeader("access-control-allow-methods", "GET,HEAD,POST,PUT,PATCH,DELETE,OPTIONS");\
	resp->addHeader("access-control-allow-headers", req->getHeader("access-control-request-headers"));\
	resp->addHeader("access-control-allow-credentials", "true");\
	resp->addHeader("access-control-max-age", "300");\
}


namespace server {
	using namespace drogon;

	class AuthFilter :public drogon::HttpFilter<AuthFilter>
	{
	public:
		virtual void doFilter(const HttpRequestPtr& req,
			FilterCallback&& fcb,
			FilterChainCallback&& fccb) override;
	};

	class FileServer : public drogon::HttpController<FileServer>
	{
	public:
		METHOD_LIST_BEGIN
			ADD_METHOD_TO(server::FileServer::auth, "/auth", Post, Options, "server::AuthFilter");

			ADD_METHOD_TO(server::FileServer::apis, "/api/{}?{}", Options, Get, "server::AuthFilter");
		METHOD_LIST_END



		void auth(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
		void apis(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback, std::string&& apiName, std::string&& apiParams) const;

	public:
		FileServer() {

		}

		static const bool isAutoCreation = false;
	};
}



