#include <cef_cmake/disable_warnings.h>
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/wrapper/cef_resource_manager.h>
#include <cef_cmake/reenable_warnings.h>

#include "utils/directory.hpp"

#include <jsbind.hpp>

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <variant>

#include <fstream>

volatile uint8_t* global_buffer{0};

#define URI_ROOT "http://localhost:8000"
//const char* const URL = URI_ROOT "/cef-echo.html";
const char* const URL = "file:///F:/UserData/Projects/open-gl-simple-view/simple-view/html/cef-echo.html";

void setupResourceManagerDirectoryProvider(CefRefPtr<CefResourceManager> resource_manager, std::string uri, std::string dir)
{
	if (!CefCurrentlyOn(TID_IO)) {
		// Execute on the browser IO thread.
		CefPostTask(TID_IO, base::Bind(&setupResourceManagerDirectoryProvider, resource_manager, uri, dir));
		return;
	}

	resource_manager->AddDirectoryProvider(uri, dir, 1, dir);
}

// this is only needed so we have a way to break the message loop
class MinimalClient : public CefClient, public CefLifeSpanHandler, public CefRequestHandler, public CefResourceRequestHandler
{
	std::unique_ptr<std::function<void(CefRefPtr<CefFrame>, std::string&&)>> callback_string;
	std::unique_ptr<std::function<void(CefRefPtr<CefFrame>, std::unique_ptr<uint8_t[]>&&, size_t)>> callback_binary;

public:
	MinimalClient() : 
		m_resourceManager(new CefResourceManager)
	{
		auto exePath = DirUtil::getCurrentExecutablePath();
		auto assetPath = DirUtil::getAssetPath(exePath, "html");

		setupResourceManagerDirectoryProvider(m_resourceManager, URI_ROOT, assetPath);
	}

	CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
	CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }

	void OnBeforeClose(CefRefPtr<CefBrowser> /*browser*/) override
	{
		CefQuitMessageLoop();
	}

	CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
		CefRefPtr<CefBrowser> /*browser*/,
		CefRefPtr<CefFrame> /*frame*/,
		CefRefPtr<CefRequest> /*request*/,
		bool /*is_navigation*/,
		bool /*is_download*/,
		const CefString& /*request_initiator*/,
		bool& /*disable_default_handling*/) override { return this; }

	cef_return_value_t OnBeforeResourceLoad(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request,
		CefRefPtr<CefRequestCallback> callback) override
	{
		return m_resourceManager->OnBeforeResourceLoad(browser, frame, request, callback);
	}

	CefRefPtr<CefResourceHandler> GetResourceHandler(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request) override
	{
		return m_resourceManager->GetResourceHandler(browser, frame, request);
	}

	void set_callback_string(std::function<void(CefRefPtr<CefFrame>, std::string&&)>&& f)
	{
		callback_string.reset(new std::function<void(CefRefPtr<CefFrame>, std::string&&)>(f));
	}

	void set_callback_binary(std::function<void(CefRefPtr<CefFrame>, std::unique_ptr<uint8_t[]>&&, size_t)>&& f)
	{
		callback_binary.reset(new std::function<void(CefRefPtr<CefFrame>, std::unique_ptr<uint8_t[]>&&, size_t)>(f));
	}

	void send_string(CefRefPtr<CefFrame> frame, std::string&& text)
	{
		auto msg = CefProcessMessage::Create("onString");

		msg->GetArgumentList()->SetString(0, text);

		frame->SendProcessMessage(PID_RENDERER, msg);
	}

	void send_binary(CefRefPtr<CefFrame> frame, std::unique_ptr<uint8_t[]>&& data, size_t size)
	{
		auto msg = CefProcessMessage::Create("onBinary");

		CefRefPtr<CefBinaryValue> binary(CefBinaryValue::Create(data.get(), size));

		std::cout << "ptr: " << (size_t)data.get() << "len: " << size;

		msg->GetArgumentList()->SetSize(size);
		msg->GetArgumentList()->SetBinary(0, binary);

		frame->SendProcessMessage(PID_RENDERER, msg);
	}

	bool OnProcessMessageReceived(CefRefPtr<CefBrowser> /*browser*/, CefRefPtr<CefFrame> frame, CefProcessId /*source_process*/, CefRefPtr<CefProcessMessage> message) override
	{
		auto name = message->GetName();
		auto args = message->GetArgumentList();

		if (callback_string && name == "onString")
		{
			auto text = args->GetString(0).ToString();

			(*callback_string)(frame, std::move(text));

			return true;
		}
		else if (callback_binary && name == "onBinary")
		{
			auto binary = args->GetBinary(0);
			auto size = args->GetSize();

			std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);

			binary->GetData(&buffer[0], size, 0);

			(*callback_binary)(frame, std::move(buffer), size);

			return true;
		}

		return false;
	}

private:
	CefRefPtr<CefResourceManager> m_resourceManager;

	IMPLEMENT_REFCOUNTING(MinimalClient);
	DISALLOW_COPY_AND_ASSIGN(MinimalClient);
};

jsbind::persistent jsOnReceiveStringData;
jsbind::persistent jsOnReceiveBinaryData;

void setReceiveStringData(jsbind::local func)
{
	jsOnReceiveStringData.reset(func);
}

void setReceiveBinaryData(jsbind::local func)
{
	jsOnReceiveBinaryData.reset(func);
}

void receiveBinary(jsbind::local v)
{
	auto msg = CefProcessMessage::Create("onBinary");

	auto jsvec = jsbind::vecFromJSArray<uint8_t>(v);

	auto data = jsvec.data();
	auto size = jsvec.size();

	if (data)
	{
		CefRefPtr<CefBinaryValue> binary(CefBinaryValue::Create(data, size));

		msg->GetArgumentList()->SetBinary(0, binary);
		msg->GetArgumentList()->SetSize(size);

		CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
	}
}

void receiveString(jsbind::local v)
{
	auto msg = CefProcessMessage::Create("onString");

	auto jsstr = v.as<std::string>();

	auto data = jsstr.data();
	auto size = jsstr.size();

	msg->GetArgumentList()->SetString(0, jsstr);
	msg->GetArgumentList()->SetSize(size);

	CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
}

JSBIND_BINDINGS(App)
{
	jsbind::function("sendString", receiveString);
	jsbind::function("sendBinary", receiveBinary);
	jsbind::function("setReceiveStringData", setReceiveStringData);
	jsbind::function("setReceiveBinaryData", setReceiveBinaryData);
}

class ReleaseCallback : public CefV8ArrayBufferReleaseCallback {
public:
	void ReleaseBuffer(void* buffer) override {
		std::free(buffer);
	}
	IMPLEMENT_REFCOUNTING(ReleaseCallback);
};

class RendererApp : public CefApp, public CefRenderProcessHandler
{
public:
	RendererApp() = default;

	CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
	{
		return this;
	}

	void OnContextCreated(CefRefPtr<CefBrowser> /*browser*/, CefRefPtr<CefFrame> /*frame*/, CefRefPtr<CefV8Context> /*context*/) override
	{
		jsbind::initialize();
	}

	void OnContextReleased(CefRefPtr<CefBrowser> /*browser*/, CefRefPtr<CefFrame> /*frame*/, CefRefPtr<CefV8Context> /*context*/) override
	{
		jsbind::enter_context();
		jsOnReceiveStringData.reset();
		jsOnReceiveBinaryData.reset();
		jsbind::exit_context();
		jsbind::deinitialize();
	}

	bool OnProcessMessageReceived(CefRefPtr<CefBrowser> /*browser*/, CefRefPtr<CefFrame> /*frame*/, CefProcessId /*source_process*/, CefRefPtr<CefProcessMessage> message) override
	{
		auto name = message->GetName();
		auto args = message->GetArgumentList();

		if (name == "onString")
		{
			auto data = args->GetString(0).ToString();

			jsbind::enter_context();
			jsOnReceiveStringData.to_local()(data);
			jsbind::exit_context();

			return true;
		}
		else if (name == "onBinary")
		{
			auto binary = args->GetBinary(0);
			auto size = args->GetSize();

			uint8_t* buffer = new uint8_t[size];

			//std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);

			auto data = binary->GetData(buffer, size, 0);

			jsbind::enter_context();

			auto jsarr = jsbind::local(CefV8Value::CreateArrayBuffer(buffer, size, new ReleaseCallback()));
			
			jsOnReceiveBinaryData.to_local()(jsarr);

			jsbind::exit_context();

			return true;
		}

		return false;
	}
private:
	IMPLEMENT_REFCOUNTING(RendererApp);
	DISALLOW_COPY_AND_ASSIGN(RendererApp);
};

int main(int argc, char* argv[])
{
	CefRefPtr<CefCommandLine> commandLine = CefCommandLine::CreateCommandLine();
#if defined(_WIN32)
	CefEnableHighDPISupport();
	CefMainArgs args(GetModuleHandle(NULL));
	commandLine->InitFromString(GetCommandLineW());
#else
	CefMainArgs args(argc, argv);
	commandLine->InitFromArgv(argc, argv);
#endif

	void* windowsSandboxInfo = NULL;

#if defined(CEF_USE_SANDBOX) && defined(_WIN32)
	// Manage the life span of the sandbox information object. This is necessary
	// for sandbox support on Windows. See cef_sandbox_win.h for complete details.
	CefScopedSandboxInfo scopedSandbox;
	windowsSandboxInfo = scopedSandbox.sandbox_info();
#endif

	CefRefPtr<CefApp> app = nullptr;
	std::string appType = commandLine->GetSwitchValue("type");
	if (appType == "renderer" || appType == "zygote")
	{
		app = new RendererApp;
		// use nullptr for other process types
	}

	int result = CefExecuteProcess(args, app, windowsSandboxInfo);
	if (result >= 0)
	{
		// child process completed
		return result;
	}

	CefSettings settings;
	settings.remote_debugging_port = 1234;
#if !defined(CEF_USE_SANDBOX)
	settings.no_sandbox = true;
#endif

	CefInitialize(args, settings, nullptr, windowsSandboxInfo);

	CefWindowInfo windowInfo;

#if defined(_WIN32)
	// On Windows we need to specify certain flags that will be passed to CreateWindowEx().
	windowInfo.SetAsPopup(NULL, "simple");
#endif
	CefBrowserSettings browserSettings;

	CefRefPtr<MinimalClient> client(new MinimalClient);

	client->set_callback_binary(
		[&](auto frame, auto data, size_t size) 
		{
			std::ifstream file("D:\\file.png", std::ios_base::binary );

			if (file.is_open())
			{
				size_t size = file.tellg();

				file.seekg(0, std::ios::end);

				size = (size_t)file.tellg() - size;

				file.seekg(0, std::ios::beg);

				data.reset(new uint8_t[size]{0});

				file.read((char*)data.get(), size);

				client->send_binary(frame, std::move(data), size);
			}
		}
	);

	client->set_callback_string(
		[&](auto frame, auto text)
		{
			client->send_string(frame, std::move(text));
		}
	);

	CefBrowserHost::CreateBrowser(windowInfo, client, URL, browserSettings, nullptr, nullptr);

	CefRunMessageLoop();

	CefShutdown();

	return 0;
}
