/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________


-*/

#include "uivsgmod.h"

#include "visvsgdata.h"
#include "visvsgdataman.h"
#include "visvsgevent.h"
#include "visvsgfontcreator.h"
#include "visvsgsurvscene.h"
#include "keyenum.h"
#include "od_iostream.h"
#include "odplugin.h"
#include "thread.h"

#include "uiaction.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uifont.h"
#include "uiodscenemgr.h"
#include "uivsgcoltabdlg.h"
#include "uivsgscenesettingsdlg.h"
#include "uivsgscreenshotdlg.h"
#include "uivsgviewer.h"

#include <QGuiApplication>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPointer>


class uiVSGPIMgr : public uiPluginInitMgr
{ mODTextTranslationClass(uiVSGPIMgr);
public:
    uiVSGPIMgr();
    ~uiVSGPIMgr();

private:
    void	dTectMenuChanged() override;
    void	cleanup() override;
    void	showCB( CallBacker* );
    void	fontCB( CallBacker* );
    void	font3DChangedCB( CallBacker* );
    void	screenshotCB( CallBacker* );
    void	sceneSettingsCB( CallBacker* );
    void	colTabCB( CallBacker* );

    QPointer<uiVSGViewer>	viewer_;
    PtrMan<uiVSGColTabDlg>	coltabdlg_;
    FontData			chosenfont_;
    bool			haschosenfont_ = false;
};


uiVSGPIMgr::uiVSGPIMgr()
    : uiPluginInitMgr()
{
    init();
    mAttachCB( FontList().get(FontData::Graphics3D).changed,
	       uiVSGPIMgr::font3DChangedCB );
}


uiVSGPIMgr::~uiVSGPIMgr()
{
    detachAllNotifiers();
}


void uiVSGPIMgr::font3DChangedCB( CallBacker* )
{
    if ( haschosenfont_ || !viewer_ )
	return;

    viewer_->setAnnotationFont(
			FontList().get(FontData::Graphics3D).fontData() );
}


void uiVSGPIMgr::dTectMenuChanged()
{
    uiMenu* scenemnu = appl().menuMgr().sceneMnu();
    if ( !scenemnu )
	return;

    scenemnu->insertAction( new uiAction( toUiString("VSG Viewer..."),
	mCB(this,uiVSGPIMgr,showCB) ) );
    scenemnu->insertAction( new uiAction( toUiString("VSG Font..."),
	mCB(this,uiVSGPIMgr,fontCB) ) );
    scenemnu->insertAction( new uiAction( toUiString("Save VSG Screenshot..."),
	mCB(this,uiVSGPIMgr,screenshotCB) ) );
    scenemnu->insertAction( new uiAction( toUiString("VSG Scene Settings..."),
	mCB(this,uiVSGPIMgr,sceneSettingsCB) ) );
    scenemnu->insertAction( new uiAction( toUiString("VSG Color Table..."),
	mCB(this,uiVSGPIMgr,colTabCB) ) );
}


void uiVSGPIMgr::colTabCB( CallBacker* )
{
    dGBVis::TextureChannels* channels = viewer_ ? viewer_->textureChannels()
						: nullptr;
    if ( !channels )
    {
	uiMSG().error( tr("Open the VSG Viewer with the test plane first") );
	return;
    }

    coltabdlg_ = new uiVSGColTabDlg( &appl(), *viewer_->scene() );
    coltabdlg_->show();
}


void uiVSGPIMgr::sceneSettingsCB( CallBacker* )
{
    if ( !viewer_ )
    {
	uiMSG().error( tr("Open the VSG Viewer first") );
	return;
    }

    uiVSGSceneSettingsDlg dlg( &appl(), *viewer_.data() );
    dlg.go();
}


void uiVSGPIMgr::cleanup()
{
    coltabdlg_ = nullptr;
    if ( viewer_ )
    {
	auto* subwin = qobject_cast<QMdiSubWindow*>( viewer_->parentWidget() );
	if ( subwin )
	    delete subwin;
	else
	    delete viewer_.data();
    }

    dGBVis::FontCreator::instance().clear();
}


void uiVSGPIMgr::showCB( CallBacker* )
{
    if ( viewer_ )
    {
	auto* subwin = qobject_cast<QMdiSubWindow*>( viewer_->parentWidget() );
	if ( subwin && subwin->mdiArea() )
	    subwin->mdiArea()->setActiveSubWindow( subwin );
	else
	    viewer_->activateWindow();

	return;
    }

    appl().sceneMgr().cleanUp( false );

    auto* viewer = new uiVSGViewer();
    viewer_ = viewer;
    if ( haschosenfont_ )
	viewer->setAnnotationFont( chosenfont_ );

    QWidget* mainwin = appl().qWidget();
    QMdiArea* mdiarea = mainwin ? mainwin->findChild<QMdiArea*>() : nullptr;
    if ( mdiarea )
    {
	QMdiSubWindow* subwin = mdiarea->addSubWindow( viewer );
	subwin->setWindowTitle( "VSG Viewer" );
	subwin->resize( 800, 600 );
	subwin->showMaximized();
	mdiarea->setActiveSubWindow( subwin );
	return;
    }

    viewer->showMaximized();
    viewer->raise();
    viewer->activateWindow();
}


void uiVSGPIMgr::fontCB( CallBacker* )
{
    if ( !viewer_ )
    {
	uiMSG().error( tr("Open the VSG Viewer first") );
	return;
    }

    FontData fd = haschosenfont_ ? chosenfont_ : viewer_->annotationFont();
    if ( !selectFont(fd,&appl(),tr("VSG Font"),true) )
	return;

    chosenfont_ = fd;
    haschosenfont_ = true;
    viewer_->setAnnotationFont( fd );
}


void uiVSGPIMgr::screenshotCB( CallBacker* )
{
    if ( !viewer_ )
    {
	uiMSG().error( tr("Open the VSG Viewer first") );
	return;
    }

    uiVSGScreenshotDlg dlg( &appl(), viewer_->width(), viewer_->height() );
    if ( !dlg.go() )
	return;

    const uiRetVal uirv = viewer_->takeScreenshot( dlg.width(), dlg.height(),
						  dlg.filename().buf(),
						  dlg.dpi() );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}


static OD::ButtonState keyboardModifiers()
{
    const Qt::KeyboardModifiers mods =
				QGuiApplication::queryKeyboardModifiers();
    int state = 0;
    if ( mods & Qt::ShiftModifier )
	state |= int(OD::ShiftButton);
    if ( mods & Qt::ControlModifier )
	state |= int(OD::ControlButton);
    if ( mods & Qt::AltModifier )
	state |= int(OD::AltButton);

    return OD::ButtonState( state );
}


mDefODPluginInfo(uiVSG)
{
    static PluginInfo retpi(
	"VSG Viewer",
	"Experimental Vulkan (VSG) 3D viewer." );
    return &retpi;
}


mDefODInitPlugin(uiVSG)
{
    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();
    dGBVisSurvey::initStdClasses();
    dGBVis::EventCatcher::setModifierQuery( keyboardModifiers );
    od_cerr() << "VSG Viewer: this is the od plugin, not the dgb one"
	      << od_endl;

    static PtrMan<uiVSGPIMgr> pimgr = new uiVSGPIMgr();
    if ( !pimgr )
	return "Cannot instantiate the VSG Viewer plugin";

    return nullptr;
}
