#pragma once

#include <cef_cmake/disable_warnings.h>
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/wrapper/cef_resource_manager.h>
#include <cef_cmake/reenable_warnings.h>
#include "../utils/directory.hpp"
#include <jsbind.hpp>
#include <iostream>

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

class ReleaseCallback : public CefV8ArrayBufferReleaseCallback
{
public:
	void ReleaseBuffer(void* buffer) override
	{
		std::free(buffer);

		std::cerr << "free";
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

	bool OnProcessMessageReceived(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefProcessId /* source proccess */, CefRefPtr<CefProcessMessage> message) override
	{
		auto name = message->GetName();
		auto args = message->GetArgumentList();

		jsbind::enter_context();

		std::unique_ptr<jsbind::local> data{ nullptr };

		if (name == "onString")
		{
			data.reset(new jsbind::local(args->GetString(0).ToString()));
		}
		else if (name == "onBinary")
		{
			auto binary = args->GetBinary(0);
			auto size = args->GetSize();

			if (size)
			{
				uint8_t* buffer = new uint8_t[size];

				binary->GetData(buffer, size, 0);

				data.reset(new jsbind::local(CefV8Value::CreateArrayBuffer(buffer, size, new ReleaseCallback())));

				//jsOnReceiveBinaryData.to_local()(jsbind::local(CefV8Value::CreateArrayBuffer(buffer, size, new ReleaseCallback())));
			}
		}

		if (data)
		{
			data->set("command", name.ToString());

			jsOnReceiveBinaryData.to_local()(*data);
		}

		jsbind::exit_context();

		return (bool)data;
	}
private:
	IMPLEMENT_REFCOUNTING(RendererApp);
	DISALLOW_COPY_AND_ASSIGN(RendererApp);
};