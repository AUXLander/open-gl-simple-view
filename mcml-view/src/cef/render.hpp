#pragma once

#include <cef_cmake/disable_warnings.h>
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/wrapper/cef_resource_manager.h>
#include <cef_cmake/reenable_warnings.h>
#include "../utils/directory.hpp"
#include <jsbind.hpp>
#include <iostream>
#include <map>

jsbind::persistent jsOnReceiveData;

void setReceiveData(jsbind::local func)
{
	jsOnReceiveData.reset(func);
}

void receiveData(jsbind::local v)
{
	auto command = v["command"].as<std::string>();

	auto msg = CefProcessMessage::Create(command);
	auto arg = msg->GetArgumentList();

	if (!command.compare("onString"))
	{
		auto content = v["content"].as<std::string>();

		arg->SetString(0, content.data());
		arg->SetSize(content.size());

		CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
	}
	else if (!command.compare("onBinary"))
	{
		auto content = jsbind::vecFromJSArray<uint8_t>(v["content"]);

		auto size = content.size();

		if (!content.empty())
		{
			CefRefPtr<CefBinaryValue> binary(CefBinaryValue::Create(content.data(), size));

			msg->GetArgumentList()->SetBinary(0, binary);
			msg->GetArgumentList()->SetSize(size);

			CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
		}
	}
	else if (!command.compare("onSetUpperBound") || !command.compare("onSetLowerBound"))
	{
		auto content = v["content"].as<double>();

		arg->SetDouble(0, content);

		CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
	}
	else if (!command.compare("onGetGist"))
	{
		CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
	}	 
	else if (!command.compare("onSetZ") || !command.compare("onSetL"))
	{
		auto content = v["content"].as<int>();

		arg->SetInt(0, content);

		CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, msg);
	}
}

JSBIND_BINDINGS(App)
{
	jsbind::function("sendData", receiveData);
	jsbind::function("setReceiveData", setReceiveData);
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

struct RendererApp : public CefApp, public CefRenderProcessHandler
{
	using callback = std::function<void(CefRefPtr<CefV8Value>& data, CefRefPtr<CefListValue>)>;
private:

	std::multimap<std::string, callback> __cbstorage;

public:
	RendererApp() = default;

	CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
	{
		return this;
	}

	void register_callback(const char* name, callback&& f)
	{
		__cbstorage.emplace(name, std::move(f));
	}

	void OnContextCreated(CefRefPtr<CefBrowser> /*browser*/, CefRefPtr<CefFrame> /*frame*/, CefRefPtr<CefV8Context> /*context*/) override
	{
		jsbind::initialize();
	}

	void OnContextReleased(CefRefPtr<CefBrowser> /*browser*/, CefRefPtr<CefFrame> /*frame*/, CefRefPtr<CefV8Context> /*context*/) override
	{
		jsbind::enter_context();
		
		jsOnReceiveData.reset();

		jsbind::exit_context();
		jsbind::deinitialize();
	}

	bool OnProcessMessageReceived(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefProcessId /* source proccess */, CefRefPtr<CefProcessMessage> message) override
	{
		auto name = message->GetName();
		auto args = message->GetArgumentList();
		auto sent = false;

		auto it = __cbstorage.find(name);

		while (it != __cbstorage.end() && it->first == name.ToString())
		{
			jsbind::enter_context();

			auto package = CefV8Value::CreateObject(NULL, NULL);

			(*it).second(package, args);

			auto content = package->GetValue("content");

			if (!content->IsUndefined())
			{
				package->SetValue("command", CefV8Value::CreateString(name.ToString()), CefV8Value::PropertyAttribute::V8_PROPERTY_ATTRIBUTE_NONE);

				jsOnReceiveData.to_local()(jsbind::local(package));

				sent = true;
			}

			jsbind::exit_context();

			++it;
		}

		return sent;
	}
private:
	IMPLEMENT_REFCOUNTING(RendererApp);
	DISALLOW_COPY_AND_ASSIGN(RendererApp);
};