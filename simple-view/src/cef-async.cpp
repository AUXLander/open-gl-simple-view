#include "cef/client.hpp"
#include "cef/render.hpp"

#include <string>
#include <vector>
#include <cassert>
#include <iostream>

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

	auto client = MinimalClient::CreateBrowserSync(windowInfo, URL, browserSettings, nullptr, nullptr);

	client->register_callback<std::string>(
		// name of method to bind
		"onString",
		
		// function to process strings
		[&client](CefRefPtr<CefProcessMessage> msg, std::string &&text) -> void {

			msg->GetArgumentList()->SetString(0, std::move(text));

			client->send(msg);
		}, 

		// function to resolve argument lists
		[](auto arglist) -> std::string {
			return arglist->GetString(0).ToString();
		}
	);

	client->register_callback<buffer<uint8_t>>(
		// name of method to bind
		"onBinary",

		// function to process buffer
		[&client](CefRefPtr<CefProcessMessage> msg, buffer<uint8_t> &&buffer) -> void {

			auto binary = CefBinaryValue::Create(buffer.data.get(), buffer.size);

			msg->GetArgumentList()->SetSize(buffer.size);
			msg->GetArgumentList()->SetBinary(0, binary);

			client->send(msg);
		},

		// function to resolve argument lists
		[](auto arglist) -> buffer<uint8_t> {

			auto binary = arglist->GetBinary(0);
			auto size = arglist->GetSize();

			buffer<uint8_t> buf(size);

			binary->GetData(buf.data.get(), size, 0);

			std::ifstream file("D:\\file.bmp", std::ios_base::binary);

			if (file.is_open())
			{
				size_t size = file.tellg();

				file.seekg(0, std::ios::end);

				size = (size_t)file.tellg() - size;

				file.seekg(0, std::ios::beg);

				buffer<uint8_t> buffer(size);

				file.read((char*)buffer.data.get(), size);
				
				file.close();
				
				return buffer;
			}

			return buf;
		}
	);

	//std::thread th([&]() {

	//	std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(10000));

	//	std::ifstream file("D:\\file.png", std::ios_base::binary);

	//	if (file.is_open())
	//	{
	//		size_t size = file.tellg();

	//		file.seekg(0, std::ios::end);

	//		size = (size_t)file.tellg() - size;

	//		file.seekg(0, std::ios::beg);

	//		std::unique_ptr<uint8_t[]> data(new uint8_t[size]{ 0 });

	//		file.read((char*)data.get(), size);

	//		client->send_binary(std::move(data), size);
	//	}
	// 
	//  file.close();
	//});

	//th.detach();

	CefRunMessageLoop();

	CefShutdown();

	return 0;
}
