#include "VAwesomium.h"

#define EXIT_COMMAND "ExitNGUI_Browser"

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

class NGUI_Browser : public VAwesomium
{
    DECLARE_CLASS_SIMPLE(NGUI_Browser, VAwesomium);
public:
    NGUI_Browser(vgui::Panel *parent, const char *panelName) : VAwesomium(parent, panelName){};

    virtual void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url);

    virtual void OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args);
};