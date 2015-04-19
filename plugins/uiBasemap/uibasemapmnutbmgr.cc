/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		November 2014
 RCS:		$Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uibasemapmnutbmgr.h"

#include "uibasemapcoltabed.h"
#include "uibasemapiomgr.h"
#include "uibasemapscalebar.h"
#include "uibasemapsettings.h"
#include "uibasemapwin.h"
#include "uichangesurfacedlg.h"
#include "uicolortable.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsview.h"
#include "uihorinterpol.h"
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


uiBaseMapMnuTBMgr::uiBaseMapMnuTBMgr( uiMainWin& mw, uiBasemapView& bmv )
    : mainwin_(mw)
    , basemapview_(bmv)
    , pickmode_(false)
    , iomgr_(new uiBasemapIOMgr(&mw))
    , isstored_(false)
{
    createMenuBar();
    createCommonActions();

    createViewTB();
    createItemTB();
    createColTabTB();

    createFileMenu();
    createProcessingMenu();

    updateViewMode();
}


uiBaseMapMnuTBMgr::~uiBaseMapMnuTBMgr()
{
    delete iomgr_;

    delete openitm_;
    delete saveitm_;
    delete saveasitm_;
}


void uiBaseMapMnuTBMgr::createMenuBar()
{
    uiMenuBar* mb = mainwin_.menuBar();
    if ( !mb )
	return;

    filemnu_ = mb->addMenu( new uiMenu(tr("Basemap")) );
    processingmnu_ = mb->addMenu( new uiMenu(uiStrings::sProcessing()) );
    syncmnu_ = mb->addMenu( new uiMenu("Synchronization") );
    helpmnu_ = mb->addMenu( new uiMenu(uiStrings::sHelp()) );
}


void uiBaseMapMnuTBMgr::createCommonActions()
{
    openitm_ = new MenuItem( uiStrings::sOpen(false), "open",
			     "Open Stored Basemap",
			     mCB(this,uiBaseMapMnuTBMgr,readCB) );
    saveitm_ = new MenuItem( uiStrings::sSave(true), "save",
			     "Store Basemap",
			     mCB(this,uiBaseMapMnuTBMgr,saveCB) );
    saveasitm_ = new MenuItem( uiStrings::sSaveAs(false), "saveas",
			       "Save as ...",
			       mCB(this,uiBaseMapMnuTBMgr,saveAsCB) );
}


void uiBaseMapMnuTBMgr::createFileMenu()
{
    filemnu_->insertAction( *openitm_ );
    filemnu_->insertAction( *saveitm_ );
    filemnu_->insertAction( *saveasitm_ );

    const MenuItem settings( uiStrings::sSettings(false),
			     mCB(this,uiBaseMapMnuTBMgr,settingsCB) );
    filemnu_->insertAction( settings );

    const MenuItem closeitm( uiStrings::sClose(),
			     mCB(this,uiBaseMapMnuTBMgr,closeCB) );
    filemnu_->insertAction( closeitm );
}


void uiBaseMapMnuTBMgr::createProcessingMenu()
{
    const MenuItem griditm( tr("Gridding"),
			    mCB(this,uiBaseMapMnuTBMgr,gridCB));
    processingmnu_->insertAction( griditm );

    const MenuItem filtitm( tr("Filtering"),
			    mCB(this,uiBaseMapMnuTBMgr,filtCB));
    processingmnu_->insertAction( filtitm );
}


void uiBaseMapMnuTBMgr::createItemTB()
{
    itemtoolbar_ = new uiToolBar( &mainwin_, "Basemap Items" );

/* For now disabled
    CallBack cb = mCB(this,uiBaseMapMnuTBMgr,iconClickCB);
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
*/

    removeid_ = itemtoolbar_->addButton( "trashcan",
					  tr("Remove selected items"),
					  mCB(this,uiBaseMapMnuTBMgr,removeCB),
					  false );
}


void uiBaseMapMnuTBMgr::createViewTB()
{
    vwtoolbar_ = new uiToolBar( &mainwin_, "Viewer Tools", uiToolBar::Left );

    viewid_ = vwtoolbar_->addButton( "altview", tr("Switch to pick mode"),
				mCB(this,uiBaseMapMnuTBMgr,viewCB), false );

    openid_ = vwtoolbar_->addButton( *openitm_ );
    saveid_ = vwtoolbar_->addButton( *saveitm_ );
    saveasid_ = vwtoolbar_->addButton( *saveasitm_ );

    vwtoolbar_->addObject(
		basemapview_.view().getSaveImageButton(vwtoolbar_) );
    vwtoolbar_->addObject(
		basemapview_.view().getPrintImageButton(vwtoolbar_) );

    vworientationid_ = vwtoolbar_->addButton( "northarrow",
			tr("Display Orientation"),
			mCB(this,uiBaseMapMnuTBMgr,vworientationCB), true);
    vwtoolbar_->turnOn( vworientationid_, true );

    vwmapscaleid_ = vwtoolbar_->addButton( "scale",
			tr("Display Map Scale"),
			mCB(this,uiBaseMapMnuTBMgr,vwmapscaleCB), true );

    uiMenu* scalemnu = new uiMenu( tr("Map Scale Menu") );
    uiAction* item = new uiAction( uiStrings::sSettings(false),
				mCB(this,uiBaseMapMnuTBMgr,barSettingsCB),
				"disppars" );
    scalemnu->insertItem( item, vwmapscaleid_ );
    vwtoolbar_->setButtonMenu( vwmapscaleid_, scalemnu );
    vwtoolbar_->turnOn( vwmapscaleid_, true );
}


void uiBaseMapMnuTBMgr::createColTabTB()
{
    ctabtoolbar_ = new uiColorTableToolBar( &mainwin_ );
    ctabed_ = new uiBasemapColTabEd( *ctabtoolbar_ );
    BMM().setColTabEd( ctabed_ );
}


void uiBaseMapMnuTBMgr::updateViewMode()
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



void uiBaseMapMnuTBMgr::barSettingsCB( CallBacker* )
{
    uiBarSettingsDialog dlg( &mainwin_, *basemapview_.getScaleBar() );
    if( !dlg.go() ) return;
}


void uiBaseMapMnuTBMgr::iconClickCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,action,cb)
    if ( !action ) return;

    const int id = action->getID();
    BMM().add( id );
}


void uiBaseMapMnuTBMgr::removeCB( CallBacker* )
{
    BMM().removeSelectedItems();
}


void uiBaseMapMnuTBMgr::viewCB( CallBacker* )
{
    pickmode_ = !pickmode_;
    updateViewMode();
}


void uiBaseMapMnuTBMgr::vwmapscaleCB( CallBacker* )
{
    const bool ison = vwtoolbar_->isOn( vwmapscaleid_ );
    basemapview_.getScaleBar()->show( ison );
}


void uiBaseMapMnuTBMgr::vworientationCB( CallBacker* )
{
    const bool ison = vwtoolbar_->isOn( vworientationid_ );
    basemapview_.getNorthArrow()->setVisible( ison );
}


void uiBaseMapMnuTBMgr::saveCB( CallBacker* )
{ save( !isstored_ ); }


void uiBaseMapMnuTBMgr::saveAsCB( CallBacker* )
{ save( true ); }


void uiBaseMapMnuTBMgr::save( bool saveas )
{
    if ( !iomgr_->save(saveas) ) return;

    basemapview_.resetChangeFlag();
    isstored_ = true;
}


void uiBaseMapMnuTBMgr::settingsCB( CallBacker* )
{
    uiBasemapSettingsDlg dlg( &mainwin_, basemapview_ );
    if ( !dlg.go() ) return;
}


void uiBaseMapMnuTBMgr::closeCB( CallBacker* )
{
    save( !isstored_ );
    mainwin_.close();
}


void uiBaseMapMnuTBMgr::gridCB( CallBacker* )
{
    uiHorizonInterpolDlg dlg( &mainwin_, 0, false );
    dlg.go();
}


void uiBaseMapMnuTBMgr::filtCB( CallBacker* )
{
    uiFilterHorizonDlg dlg( &mainwin_, 0 );
    dlg.go();
}


void uiBaseMapMnuTBMgr::readCB( CallBacker* )
{
    if ( iomgr_->read(basemapview_.hasChanged()) )
	isstored_ = true;
}
