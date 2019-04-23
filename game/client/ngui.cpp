//The following include files are necessary to allow your NPanel.cpp to compile.
#include "cbase.h"
#include "INPanel.h"
using namespace vgui;
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include "VAwesomium.h"
#include "ngui_handler.h"
#include "tier0/icommandline.h"

const char *COM_GetModDirectory_NGUI()
{
	static char modDir[MAX_PATH];
	if ( Q_strlen( modDir ) == 0 )
	{
		const char *gamedir = CommandLine()->ParmValue("-game", CommandLine()->ParmValue( "-defaultgamedir", "hl2" ) );
		Q_strncpy( modDir, gamedir, sizeof(modDir) );
		if ( strchr( modDir, '/' ) || strchr( modDir, '\\' ) )
		{
			Q_StripLastDir( modDir, sizeof(modDir) );
			int dirlen = Q_strlen( modDir );
			Q_strncpy( modDir, gamedir + dirlen, sizeof(modDir) - dirlen );
		}
	}

	return modDir;
}


//CNPanel class: Tutorial example class
class CNPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CNPanel, vgui::Frame); 
	//CNPanel : This Class / vgui::Frame : BaseClass

	CNPanel(vgui::VPANEL parent); 	// Constructor
	~CNPanel(){};				// Destructor
 
protected:
	//VGUI overrides:
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);

private:
	//Other used VGUI control Elements:
	NGUI_Browser *m_Browser;

};

CNPanel::CNPanel(vgui::VPANEL parent)
: BaseClass(NULL, "NPanel")
{
	SetParent( parent );
	
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
	
	SetProportional( false );
	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetVisible( true );
	SetSize( ScreenWidth(), ScreenHeight() );
	
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	LoadControlSettings("resource/UI/NPanel.res");

	char fname[100]; 

	GetCurrentDirectory(100, fname);
	
	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	m_Browser = new NGUI_Browser(this, "VAwesomium");
    m_Browser->SetSize(ScreenWidth(), ScreenHeight());
	char path1[256];
	Q_snprintf( path1, sizeof(path1), "file:///%s/%s/ngui/menu.html", fname, COM_GetModDirectory_NGUI());
    m_Browser->OpenURL(path1);
    //m_Browser->OpenURL("file:///T:/gluon/game/ep3/ngui/menu.html");
	DevMsg("NGUI has been constructed\n");
	DevMsg("NGUI path = %s\n", path1);
}

//Class: CNPanelInterface Class. Used for construction.
class CNPanelInterface : public INPanel
{
private:
	CNPanel *NPanel;
public:
	CNPanelInterface()
	{
		NPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		NPanel = new CNPanel(parent);
	}
	void Destroy()
	{
		if (NPanel)
		{
			NPanel->SetParent( (vgui::Panel *)NULL);
			delete NPanel;
		}
	}
};
static CNPanelInterface g_NPanel;
INPanel* NPanel = (INPanel*)&g_NPanel;

void CNPanel::OnTick()
{
	BaseClass::OnTick();
}

void CNPanel::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);
}