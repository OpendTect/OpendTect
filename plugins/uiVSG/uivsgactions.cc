/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________


-*/

#include "uivsgmod.h"

#include "ptrman.h"
#include "stringview.h"

#include "uimsg.h"
#include "uivsgcolorbardlg.h"
#include "uivsgcoltabdlg.h"
#include "uivsgscenesettingsdlg.h"
#include "uivsgscreenshotdlg.h"
#include "uivsgviewer.h"

#include "visvsgsurvscene.h"


static void showScreenshotDialog( uiParent* parent, uiVSGViewer& viewer )
{
    uiVSGScreenshotDlg dlg( parent, viewer.width(), viewer.height() );
    if ( !dlg.go() )
	return;

    const uiRetVal uirv = viewer.takeScreenshot( dlg.width(), dlg.height(),
						 dlg.filename().buf(),
						 dlg.dpi() );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}


static void showSettingsDialog( uiParent* parent, uiVSGViewer& viewer )
{
    uiVSGSceneSettingsDlg dlg( parent, viewer );
    dlg.go();
}


static void showColorBarDialog( uiParent* parent, uiVSGViewer& viewer )
{
    uiVSGColorBarDlg dlg( parent, viewer );
    dlg.go();
}


static void showColorTableDialog( uiParent* parent, uiVSGViewer& viewer )
{
    ODVsg::Scene* scene = viewer.scene();
    if ( !scene || !viewer.textureChannels() )
    {
	uiMSG().error( toUiString("The VSG scene has no test plane") );
	return;
    }

    mDefineStaticLocalObject( PtrMan<uiVSGColTabDlg>, coltabdlg, );
    coltabdlg = new uiVSGColTabDlg( parent, *scene );
    coltabdlg->show();
}


mExternC(uiVSG) bool showVSGDialog(const char*,uiParent*,const SceneID&);

mExternC(uiVSG) bool showVSGDialog( const char* action, uiParent* parent,
				    const SceneID& sceneid )
{
    uiVSGViewer* viewer = uiVSGViewer::find( sceneid );
    if ( !viewer )
    {
	uiMSG().error( toUiString("Open a VSG scene first") );
	return false;
    }

    const StringView actionstr( action );
    if ( actionstr == "Screenshot" )
	showScreenshotDialog( parent, *viewer );
    else if ( actionstr == "Settings" )
	showSettingsDialog( parent, *viewer );
    else if ( actionstr == "ColorBar" )
	showColorBarDialog( parent, *viewer );
    else if ( actionstr == "ColorTable" )
	showColorTableDialog( parent, *viewer );
    else
	return false;

    return true;
}


mExternC(uiVSG) void showVSGColorBar(const SceneID&,bool);

mExternC(uiVSG) void showVSGColorBar( const SceneID& sceneid, bool yn )
{
    uiVSGViewer* viewer = uiVSGViewer::find( sceneid );
    if ( viewer )
	viewer->showColorBar( yn );
}


mExternC(uiVSG) void setVSGViewMode(const SceneID&,bool);

mExternC(uiVSG) void setVSGViewMode( const SceneID& sceneid, bool yn )
{
    uiVSGViewer* viewer = uiVSGViewer::find( sceneid );
    if ( viewer )
	viewer->setViewMode( yn );
}


mExternC(uiVSG) NotifierAccess* getVSGViewModeNotifier(const SceneID&);

mExternC(uiVSG) NotifierAccess* getVSGViewModeNotifier(
						const SceneID& sceneid )
{
    uiVSGViewer* viewer = uiVSGViewer::find( sceneid );
    return viewer ? &viewer->viewmodechanged : nullptr;
}


mExternC(uiVSG) void doVSGViewAction(const char*,const SceneID&);

mExternC(uiVSG) void doVSGViewAction( const char* action,
				      const SceneID& sceneid )
{
    uiVSGViewer* viewer = uiVSGViewer::find( sceneid );
    if ( !viewer )
	return;

    const StringView actionstr( action );
    if ( actionstr == "ToHome" )
	viewer->toHomePos();
    else if ( actionstr == "SaveHome" )
	viewer->saveHomePos();
    else if ( actionstr == "ResetHome" )
	viewer->resetHomePos();
    else if ( actionstr == "ViewAll" )
	viewer->viewAll();
}


mExternC(uiVSG) void showVSGRotAxis(const SceneID&,bool);

mExternC(uiVSG) void showVSGRotAxis( const SceneID& sceneid, bool yn )
{
    uiVSGViewer* viewer = uiVSGViewer::find( sceneid );
    if ( !viewer )
	return;

    if ( yn && !viewer->isAxisShown() )
	viewer->setModernAxes( !viewer->isModernAxes() );

    viewer->showRotAxis( yn );
}
