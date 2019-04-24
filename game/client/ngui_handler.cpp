#include "cbase.h"
#include "ngui_handler.h"
using namespace Awesomium;

#ifdef PostMessage
#undef PostMessage
#endif


const ConVar *hostip_ngui;
const ConVar *hostname_ngui;
const ConVar *name_ngui;
const ConVar *mapname_ngui;
ConVar *temporarycv;

char *buff2 =  (char *)malloc( sizeof( char ) * 256 );

void NGUI_Browser::OnDocumentReady(WebView* caller, const WebURL& url)
{
    JSValue result = caller->CreateGlobalJavascriptObject(WSLit("ngui"));

    JSObject &myObject = result.ToObject();
    myObject.SetCustomMethod(WSLit("ReturnToGame"), false);
    myObject.SetCustomMethod(WSLit("Cmd"), false);
    myObject.SetCustomMethod(WSLit("getUsername"), true);
    myObject.SetCustomMethod(WSLit("getHostname"), true);
    myObject.SetCustomMethod(WSLit("getHostip"), true);
    myObject.SetCustomMethod(WSLit("getMapname"), true);
    myObject.SetCustomMethod(WSLit("getConvar"), true);

    caller->ExecuteJavascript(WSLit("AwesomiumInitialized()"), WSLit(""));
}

void NGUI_Browser::OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args)
{
    if (method_name == WSLit("ReturnToGame"))
    {
		engine->ClientCmd_Unrestricted( "resume" );
        BaseClass::GetParent()->PostMessage(BaseClass::GetParent(), new KeyValues("command", "command", EXIT_COMMAND));
    }
	if (method_name == WSLit("Cmd"))
    {
		WebString buffer = args[0].ToString();
		if(buffer.length() < 1) return;
		
		// copying the contents of the 
		// string to char array 
		strcpy(buff2, ToString(buffer).c_str());
	
		engine->ClientCmd_Unrestricted( buff2 );
    }
}

JSValue NGUI_Browser::OnMethodCallWithReturnValue(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args)
{
	if(method_name == WSLit("getUsername")) {
		name_ngui = cvar->FindVar( "name" );
		return JSValue(WSLit(name_ngui->GetString()));
	}
	if(method_name == WSLit("getHostname")) {
		hostname_ngui = cvar->FindVar( "hostname" );
		return JSValue(WSLit(hostname_ngui->GetString()));
	}
	if(method_name == WSLit("getHostip")) {
		hostip_ngui = cvar->FindVar( "hostip" );
		return JSValue(WSLit(hostip_ngui->GetString()));
	}
	if(method_name == WSLit("getMapname")) {
		mapname_ngui = cvar->FindVar( "map_current" );
		if(strcmp ("", mapname_ngui->GetString()) == 0) {
			return JSValue(WSLit("nullpointer"));
		}else{
			return JSValue(WSLit(mapname_ngui->GetString()));
		}
	}
	if(method_name == WSLit("getConvar")) {
		WebString buffer = args[0].ToString();
		if(buffer.length() < 1) return JSValue(0);
		
		// copying the contents of the 
		// string to char array 
		strcpy(buff2, ToString(buffer).c_str());
		temporarycv = cvar->FindVar( buff2 );
		return JSValue(WSLit(temporarycv->GetString()));
	}
	return JSValue(0);
}