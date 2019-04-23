#include "cbase.h"
#include "ngui_handler.h"
using namespace Awesomium;

#ifdef PostMessage
#undef PostMessage
#endif

char *buff2 =  (char *)malloc( sizeof( char ) * 256 );

void NGUI_Browser::OnDocumentReady(WebView* caller, const WebURL& url)
{
    JSValue result = caller->CreateGlobalJavascriptObject(WSLit("MyObject"));

    JSObject &myObject = result.ToObject();
    myObject.SetCustomMethod(WSLit("ReturnToGame"), false);
    myObject.SetCustomMethod(WSLit("Cmd"), false);

    caller->ExecuteJavascript(WSLit("AwesomiumInitialized()"), WSLit(""));
}

void NGUI_Browser::OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args)
{
    if (method_name == WSLit("ReturnToGame"))
    {
        BaseClass::GetParent()->PostMessage(BaseClass::GetParent(), new KeyValues("command", "command", EXIT_COMMAND));
    }
	if (method_name == WSLit("Cmd"))
    {
		WebString buffer = args[0].ToString();
		if(buffer.length() < 1) return;
		
		int n = buffer.length(); 
  
		// declaring character array 
  
		// copying the contents of the 
		// string to char array 
		strcpy(buff2, ToString(buffer).c_str());
	
		engine->ClientCmd( buff2 );
    }
}