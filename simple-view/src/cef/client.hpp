#pragma once

#include <cef_cmake/disable_warnings.h>
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/wrapper/cef_resource_manager.h>
#include <cef_cmake/reenable_warnings.h>
#include "../utils/directory.hpp"
#include "types.hpp"
#include <jsbind.hpp>
#include <map>

#include <thread>
#include <chrono>
#include <optional>

#include <fstream>

//#define URI_ROOT "http://localhost:8000"
//const char* const URL = URI_ROOT "/cef-echo.html";

#define URI_ROOT "http://localhost:8000"
const char* const URL = "file:///F:/UserData/Projects/open-gl-simple-view/simple-view/html/cef-echo.html";

void setupResourceManagerDirectoryProvider(CefRefPtr<CefResourceManager> resource_manager, std::string uri, std::string dir)
{
	if (!CefCurrentlyOn(TID_IO))
	{
		// Execute on the browser IO thread.
		CefPostTask(TID_IO, base::Bind(&setupResourceManagerDirectoryProvider, resource_manager, uri, dir));
		return;
	}

	resource_manager->AddDirectoryProvider(uri, dir, 1, dir);
}


// this is only needed so we have a way to break the message loop
struct MinimalClient : public CefClient, public CefLifeSpanHandler, public CefRequestHandler, public CefResourceRequestHandler
{
	template<class ArgumentList>
	struct callback_base
	{
		using argslist = typename ArgumentList;

	protected:
		const char* __name;

		callback_base(const char* name) :
			__name{ name }
		{;}

	public:
		virtual void call() {};
		virtual void resolve_argument_list(ArgumentList args) {};
		virtual ~callback_base() {}

		CefRefPtr<CefProcessMessage> message() const
		{
			return CefProcessMessage::Create(__name);
		}

		void operator()()
		{
			return call();
		}

		const char* name() const
		{
			return __name;
		}
	};

	template<class T, class ArgumentList, class function, class resolver>
	struct callback : public callback_base<ArgumentList>
	{
		using argslist = typename ArgumentList;
		using argument = typename T;

	private:
		function __fnc;
		resolver __rsl;
		argument __arg;

	public:
		callback(const char* name, function f, resolver r) :
			callback_base<ArgumentList>(name),
			__fnc{ f }, __rsl{ r }
		{;}

		callback(callback<T, ArgumentList, function, resolver>&&) = default;
		callback(const callback<T, ArgumentList, function, resolver>&) = default;

		virtual void resolve_argument_list(argslist args) override
		{
			set_argument(std::move(__rsl(args)));
		}

		virtual void call() override
		{
			__fnc(message(), std::move(__arg));
		}

		void set_argument(argument&& arg)
		{
			__arg = std::move(arg);
		}
	};

	using callback_base_arguments = CefRefPtr<CefListValue>;

	template<class T, class function, class resolver>
	using callback_t = typename callback<T, callback_base_arguments, function, resolver>;

	using buffer_t = buffer<uint8_t>;
	using callback_base_t = callback_base<callback_base_arguments>;

	std::multimap<std::string, std::unique_ptr<callback_base_t>> __cbstorage;

private:

	CefRefPtr<CefBrowser> __browser;

	MinimalClient() :
		m_resourceManager(new CefResourceManager)
	{
		auto exePath = DirUtil::getCurrentExecutablePath();
		auto assetPath = DirUtil::getAssetPath(exePath, "html"); // folder

		setupResourceManagerDirectoryProvider(m_resourceManager, URI_ROOT, assetPath);
	}

public:

	static CefRefPtr<MinimalClient> CreateBrowserSync(const CefWindowInfo& windowInfo, const CefString& url, const CefBrowserSettings& settings, CefRefPtr<CefDictionaryValue> extra_info, CefRefPtr<CefRequestContext> request_context) 
	{
		CefRefPtr<MinimalClient> client(new MinimalClient);

		client->__browser = CefBrowserHost::CreateBrowserSync(windowInfo, client, url, settings, extra_info, request_context);

		return client;
	}

	template<class T, class function, class resolver>
	void register_callback(const char* name, function f, resolver r)
	{
		__cbstorage.emplace(name, new callback<T, callback_base_arguments, function, resolver>(name, f, r));
	}

	void send(CefRefPtr<CefProcessMessage> message) const
	{
		__browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
	}

private:

	CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
	CefRefPtr<CefRequestHandler> GetRequestHandler()   override { return this; }

	void OnBeforeClose(CefRefPtr<CefBrowser>) override
	{
		CefQuitMessageLoop();
	}

	CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefRequest>, bool is_navigation, bool is_download, const CefString& request_initiator, bool& disable_default_handling) override
	{
		return this;
	}

	cef_return_value_t OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback) override
	{
		return m_resourceManager->OnBeforeResourceLoad(browser, frame, request, callback);
	}

	CefRefPtr<CefResourceHandler> GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) override
	{
		return m_resourceManager->GetResourceHandler(browser, frame, request);
	}

	bool OnProcessMessageReceived(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefProcessId /*source_process*/, CefRefPtr<CefProcessMessage> message) override
	{
		auto name = message->GetName().ToString();
		auto args = message->GetArgumentList();

		auto it = __cbstorage.find(name);
		bool found = false;

		while (it != __cbstorage.end() && it->first == name)
		{
			auto &callback = (*it).second;

			callback->resolve_argument_list(args);
			callback->call();

			found = true;
			++it;
		}

		return found;
	}

private:
	CefRefPtr<CefResourceManager> m_resourceManager;

	IMPLEMENT_REFCOUNTING(MinimalClient);
	DISALLOW_COPY_AND_ASSIGN(MinimalClient);
};
