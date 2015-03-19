/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		November 2014
 RCS:		$Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibasemaptbmgr.h"

#include "uibasemapiomgr.h"
#include "uibasemapscalebar.h"
#include "uibasemapwin.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsview.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiioobjseldlg.h"
#include "uisellinest.h"
#include "uitoolbar.h"

#include "basemaptr.h"
#include "ioman.h"
#include "menuhandler.h"
#include "survinfo.h"


uiBaseMapTBMgr::uiBaseMapTBMgr( uiMainWin& mw, uiBasemapView& bmv )
    : mainwin_(mw)
    , basemapview_(bmv)
    , pickmode_(false)
    , iomgr_(new uiBasemapIOMgr(&mw))
    , isstored_(false)
{
    createMenuBar();
    createCommonActions();

    createviewTB();
    createitemTB();

    createFileMenu();

    updateViewMode();
}


uiBaseMapTBMgr::~uiBaseMapTBMgr()
{
    delete iomgr_;

    delete open_;
    delete save_;
    delete saveas_;
}


void uiBaseMapTBMgr::createMenuBar()
{
    filemnu_ = mainwin_.menuBar()->addMenu( new uiMenu(uiStrings::sFile()) );
    processingmnu_ = mainwin_.menuBar()->addMenu( new uiMenu("Processing") );
    syncmnu_ = mainwin_.menuBar()->addMenu( new uiMenu("Synchronization") );
    helpmnu_ = mainwin_.menuBar()->addMenu( new uiMenu("Help") );
}


void uiBaseMapTBMgr::createCommonActions()
{
    open_ = new MenuItem( uiStrings::sOpen(false), "open",
			  "Open Stored Basemap",
			  mCB(this,uiBaseMapTBMgr,readCB) );
    save_ = new MenuItem( uiStrings::sSave(true), "save",
			  "Store Basemap",
			  mCB(this,uiBaseMapTBMgr,saveCB) );
    saveas_ = new MenuItem( uiStrings::sSaveAs(false), "saveas",
			    "Save as...",
			    mCB(this,uiBaseMapTBMgr,saveAsCB) );
}


void uiBaseMapTBMgr::createFileMenu()
{
    if ( !open_ || !save_ || !saveas_ ) return;

    filemnu_->insertAction( *open_ );
    filemnu_->insertAction( *save_ );
    filemnu_->insertAction( *saveas_ );
}


void uiBaseMapTBMgr::createitemTB()
{
    itemtoolbar_ = new uiToolBar( &mainwin_, "Basemap Items" );
    CallBack cb = mCB(this,uiBaseMapTBMgr,iconClickCB);
    const ObjectSet<uiBasemapItem>& itms = BMM().items();

    addid_ = itemtoolbar_->addButton( "basemap-add", tr("Add Basemap Items") );
    uiMenu* itemmnu = new uiMenu( tr("Item Menu") );

    for ( int idx=0; idx<itms.size(); idx++ )
    {
	const uiBasemapItem* itm = itms[idx];

	uiString str( "Add " ); str.append( itm->factoryDisplayName() );
	uiAction* action = new uiAction( str, cb, itm->iconName() );
	itemmnu->insertAction( action, itm->ID() );
    }
    itemtoolbar_->setButtonMenu( addid_, itemmnu, uiToolButton::InstantPopup );

    removeid_ = itemtoolbar_->addButton( "trashcan",
					  tr("Remove selected items"),
					  mCB(this,uiBaseMapTBMgr,removeCB),
					  false );
}


void uiBaseMapTBMgr::createviewTB()
{
    vwtoolbar_ = new uiToolBar( &mainwin_, "Viewer Tools", uiToolBar::Left );

    viewid_ = vwtoolbar_->addButton( "altview", tr("Switch to pick mode"),
				mCB(this,uiBaseMapTBMgr,viewCB), false );

    openid_ = vwtoolbar_->addButton( *open_ );
    saveid_ = vwtoolbar_->addButton( *save_ );
    saveasid_ = vwtoolbar_->addButton( *saveas_ );

    vwtoolbar_->addObject(
		basemapview_.view().getSaveImageButton(vwtoolbar_) );
    vwtoolbar_->addObject(
		basemapview_.view().getPrintImageButton(vwtoolbar_) );

    vworientationid_ = vwtoolbar_->addButton( "northarrow",
				tr("Display Orientation"),
				mCB(this,uiBaseMapTBMgr,vworientationCB), true);
    vwtoolbar_->turnOn( vworientationid_, true );

    vwmapscaleid_ = vwtoolbar_->addButton( "scale",
				tr("Display Map Scale"),
				mCB(this,uiBaseMapTBMgr,vwmapscaleCB), true );

    uiMenu* scalemnu = new uiMenu( tr("Map Scale Menu") );
    uiAction* item = new uiAction( uiStrings::sSettings(false),
				mCB(this,uiBaseMapTBMgr,barSettingsCB),
				"disppars" );
    scalemnu->insertItem( item, vwmapscaleid_ );
    vwtoolbar_->setButtonMenu( vwmapscaleid_, scalemnu );
    vwtoolbar_->turnOn( vwmapscaleid_, true );
}


void uiBaseMapTBMgr::updateViewMode()
{
    vwtoolbar_->setIcon( viewid_, pickmode_ ? "altpick" : "altview" );
    vwtoolbar_->setToolTip( viewid_, pickmode_ ? "Switch to view mode"
				    : "Switch to pick mode" );
    basemapview_.view().setDragMode(
	pickmode_ ? uiGraphicsViewBase::NoDrag
		  : uiGraphicsViewBase::ScrollHandDrag);
}



class uiBarSettingsDialog : public uiDialog
{ mODTextTranslationClass(uiBarSettingsDialog)
public:
uiBarSettingsDialog( uiParent* p, uiMapScaleObject& ms )
    : uiDialog(p,Setup(tr("Scale Properties"),mNoDlgTitle,mTODOHelpKey))
    , scalelenfld_(0)
    , msobj_(ms)
{
    setCtrlStyle( CloseOnly );

    scalelen_ = msobj_.getScaleLen();

    const double mv = SI().maxCoord(false).x - SI().minCoord(false).x;
    const int imv = mCast(int,mv/100);
    maxscalelen_ = mCast(float,100*imv);

    const BufferString lbl( "Scale Size ", SI().getXYUnitString() );
    scalelenfld_ = new uiGenInput( this, lbl, FloatInpSpec(scalelen_) );
    scalelenfld_->valuechanged.notify(
		mCB(this,uiBarSettingsDialog,scalebarChg) );

    uiSelLineStyle::Setup stu; stu.drawstyle( false );
    const LineStyle lst( msobj_.getLineStyle() );
    scalestylefld_ = new uiSelLineStyle( this, lst, stu );
    scalestylefld_->changed.notify(
		mCB(this,uiBarSettingsDialog,scalebarChg) );
    scalestylefld_->attach( alignedBelow, scalelenfld_ );
}


void scalebarChg( CallBacker* )
{
    const float newlen = scalelenfld_->getfValue();
    if ( newlen<=0 || newlen>maxscalelen_ )
    {
	scalelenfld_->setValue( scalelen_ );
	const BufferString msg(
		"Please enter a value between 0 and ", maxscalelen_ );
	uiMSG().error( msg );
    }
    else
    {
	msobj_.setScaleLen( newlen );
	scalelen_ = newlen;
    }

    msobj_.setLineStyle( scalestylefld_->getStyle() );
}

    float		scalelen_;
    float		maxscalelen_;

    uiGenInput*		scalelenfld_;
    uiSelLineStyle*	scalestylefld_;
    uiMapScaleObject&	msobj_;
};



void uiBaseMapTBMgr::barSettingsCB( CallBacker* )
{
    uiBarSettingsDialog dlg( &mainwin_, *basemapview_.getScaleBar() );
    if( !dlg.go() ) return;
}


void uiBaseMapTBMgr::iconClickCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,action,cb)
    if ( !action ) return;

    const int id = action->getID();
    BMM().add( id );
}


void uiBaseMapTBMgr::removeCB( CallBacker* )
{
    BMM().removeSelectedItems();
}


void uiBaseMapTBMgr::viewCB( CallBacker* )
{
    pickmode_ = !pickmode_;
    updateViewMode();
}


void uiBaseMapTBMgr::vwmapscaleCB( CallBacker* )
{
    const bool ison = vwtoolbar_->isOn( vwmapscaleid_ );
    basemapview_.getScaleBar()->show( ison );
}


void uiBaseMapTBMgr::vworientationCB( CallBacker* )
{
    const bool ison = vwtoolbar_->isOn( vworientationid_ );
    basemapview_.getNorthArrow()->setVisible( ison );
}


void uiBaseMapTBMgr::saveCB( CallBacker* )
{ save( !isstored_ ); }


void uiBaseMapTBMgr::saveAsCB( CallBacker* )
{ save( true ); }


void uiBaseMapTBMgr::save( bool saveas )
{
    if ( !iomgr_->save(saveas) ) return;

    basemapview_.resetChangeFlag();
    isstored_ = true;
}


void uiBaseMapTBMgr::readCB( CallBacker* )
{
    if ( iomgr_->read(basemapview_.hasChanged()) )
	isstored_ = true;
}
