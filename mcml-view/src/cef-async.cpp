#include "cef/client.hpp"
#include "cef/render.hpp"

#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


#include "gl_draw.h"

#include <string>
#include <vector>
#include <cassert>
#include <iostream>


struct OpenGLClient : public MinimalClient
{
	GLFWwindow* window;

	render_xy renderer;

	explorer ex;

	static CefRefPtr<OpenGLClient> CreateBrowserSync(const CefWindowInfo& windowInfo, const CefString& url, const CefBrowserSettings& settings, CefRefPtr<CefDictionaryValue> extra_info, CefRefPtr<CefRequestContext> request_context)
	{
		CefRefPtr<OpenGLClient> client(new OpenGLClient);

		client->__browser = CefBrowserHost::CreateBrowserSync(windowInfo, client, url, settings, extra_info, request_context);

		//client->__main_tread.reset(new std::thread(client->main()));

		//client->__main_tread->detach();

		client->main();

		return client;
	}

	int main() override
	{
		/* Initialize the library */
		if (!glfwInit())
		{
			return -1;
		}

		window = glfwCreateWindow(800, 800, "MCML Draw", NULL, NULL);

		SetWindowPos(glfwGetWin32Window(window), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
		//SetWindowPos(glfwGetWin32Window(window), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		glfwMakeContextCurrent(window);

		if (!gladLoadGL())
		{
			return -2;
		}

		std::cout << "Render device: " << glGetString(GL_RENDERER) << '\n';
		std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << '\n';

		ex.init(800, 800);
	}

	void draw(const model& m)
	{
		glfwMakeContextCurrent(window);

		glfwPollEvents();

		ex.invalidate_frame();

		ex.draw_texture(renderer, m);

		glfwSwapBuffers(window);
	}

	void set_lower_bound(float v)
	{
		renderer.set_lower_bound(v);
	}

	void set_upper_bound(float v)
	{
		renderer.set_upper_bound(v);
	}

	void set_z_index(size_t index)
	{
		//ex.set_z_index(index);
	}

	void set_l_index(size_t index)
	{
		//ex.set_l_index(index);
	}
};


int vmain(int argc, char* argv[])
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
		auto __renderer = new RendererApp;

		__renderer->register_callback(
			"onString",
			[](auto& package, auto args) {

				auto string = CefV8Value::CreateString(args->GetString(0).ToString());

				package->SetValue("content", string, cef_v8_propertyattribute_t::V8_PROPERTY_ATTRIBUTE_NONE);
			}
		);

		__renderer->register_callback(
			"onBinary",
			[](auto& package, auto args) {
				auto binary = args->GetBinary(0);
				auto size = args->GetSize();

				if (size)
				{
					uint8_t* buffer = new uint8_t[size]{0};

					binary->GetData(buffer, size, 0);

					auto bin = CefV8Value::CreateArrayBuffer(buffer, size, new ReleaseCallback());

					package->SetValue("content", bin, cef_v8_propertyattribute_t::V8_PROPERTY_ATTRIBUTE_NONE);
				}
			}
		);

		__renderer->register_callback(
			"onGetGist",
			[](auto& package, auto args) {

				auto string = CefV8Value::CreateString(args->GetString(0).ToString());

				package->SetValue("content", string, cef_v8_propertyattribute_t::V8_PROPERTY_ATTRIBUTE_NONE);
			}
		);

		// use nullptr for other process types
		app = __renderer;
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
	windowInfo.SetAsPopup(NULL, "Monte Carlo Visualizer");
#endif

	windowInfo.width = 1400;
	windowInfo.height = 1100;

	CefBrowserSettings browserSettings;

	auto client = OpenGLClient::CreateBrowserSync(windowInfo, URL, browserSettings, nullptr, nullptr);

	filemodel fm("D:\\snapshot-2022_10_15-15_12_51.bin");

	client->register_callback<std::string>(
		// name of method to bind
		"onString",
		
		// function to process strings
		[&client](CefRefPtr<CefProcessMessage> msg, std::string &&text) -> void {

			auto message = CefString(std::move(text));

			msg->GetArgumentList()->SetString(0, message);

			client->send(msg);
		}, 

		// function to resolve argument lists
		[](auto arglist) -> std::string {
			return arglist->GetString(0).ToString();
		}
	);

	client->register_callback<bool>(
		// name of method to bind
		"onGetGist",

		// function to process strings
		[&client, &fm](CefRefPtr<CefProcessMessage> msg, bool) -> void {

			std::string data = fm.gist(1);



			msg->GetArgumentList()->SetString(0, data);
		
			client->send(msg);
		},

		// function to resolve argument lists
		[&client](auto arglist) -> bool { return true; }
	);

	client->register_callback<bool>(
		// name of method to bind
		"onSetLowerBound",

		// function to process strings
		[&client](CefRefPtr<CefProcessMessage> msg, bool) -> void {},

		// function to resolve argument lists
		[&client](auto arglist) -> bool {
			double value = arglist->GetDouble(0);

			client->set_lower_bound(value);

			return true;
		}
	);

	client->register_callback<bool>(
		// name of method to bind
		"onSetUpperBound",

		// function to process strings
		[&client](CefRefPtr<CefProcessMessage> msg, bool) -> void {},

		// function to resolve argument lists
		[&client](auto arglist) -> bool {

			double value = arglist->GetDouble(0);

			client->set_upper_bound(value);

			return true;
		}
	);


	client->register_callback<bool>(
		// name of method to bind
		"onSetZ",

		// function to process strings
		[&client](CefRefPtr<CefProcessMessage> msg, bool) -> void {},

		// function to resolve argument lists
		[&client](auto arglist) -> bool {

			double value = arglist->GetInt(0);

			client->set_z_index(value);

			return true;
		}
	);


	client->register_callback<bool>(
		// name of method to bind
		"onSetL",

		// function to process strings
		[&client](CefRefPtr<CefProcessMessage> msg, bool) -> void {},

		// function to resolve argument lists
		[&client](auto arglist) -> bool {

			double value = arglist->GetInt(0);

			client->set_l_index(value);

			return true;
		}
	);


	client->register_callback<std::pair<uint8_t*, size_t>>(
		// name of method to bind
		"onBinary",

		// function to process buffer
		[&client](CefRefPtr<CefProcessMessage> msg, std::pair<uint8_t*, size_t> buffer) -> void {

			auto [ptr, size] = buffer;

			//return;

			auto binary = CefBinaryValue::Create(ptr, size);

			msg->GetArgumentList()->SetBinary(0, binary);
			msg->GetArgumentList()->SetSize(size);

			client->send(msg);
		},

		// function to resolve argument lists
		[&client, &fm](auto arglist) -> std::pair<uint8_t*, size_t> {

			auto binary = arglist->GetBinary(0);
			auto size = arglist->GetSize();

			//binary->GetData(buf.data.get(), size, 0);

			client->draw(fm);

			uint8_t * p = client->ex.get_texture();
			size_t s = 54 + client->ex.get_texture_size();

			//return std::pair<uint8_t*, size_t>(nullptr, 0);
			
			
			return std::pair<uint8_t*, size_t>(p, s);
			
			
			//std::ifstream file("D:\\file.bmp", std::ios_base::binary);
			//
			//if (file.is_open())
			//{
			//	size_t size = file.tellg();

			//	file.seekg(0, std::ios::end);

			//	size = (size_t)file.tellg() - size;

			//	file.seekg(0, std::ios::beg);

			//	buffer<uint8_t> buffer(size);

			//	file.read((char*)buffer.data.get(), size);
			//	
			//	file.close();
			//	
			//	return buffer;
			//}

			//return std::pair<uint8_t*, size_t>(nullptr, 0);
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
