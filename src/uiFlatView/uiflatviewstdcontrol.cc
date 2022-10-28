/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewstdcontrol.h"
#include "uibitmapdisplay.h"

#include "uicolortable.h"
#include "uiflatviewcoltabed.h"
#include "uiflatviewer.h"
#include "uigraphicsscene.h"
#include "uigeninput.h"
#include "uimainwin.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uirgbarraycanvas.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "keyboardevent.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "survinfo.h"
#include "texttranslator.h"

#define sLocalHZIdx	0
#define sGlobalHZIdx	1
#define sManHZIdx	2

static const char* sKeyVW2DTrcsPerCM()	{ return "Viewer2D.TrcsPerCM"; }
static const char* sKeyVW2DZPerCM()	{ return "Viewer2D.ZSamplesPerCM"; }

uiFlatViewZoomLevelDlg::uiFlatViewZoomLevelDlg( uiParent* p,
			float x1start, float x2start,
			float x1pospercm, float x2pospercm, bool isvertical )
    : uiDialog(p,uiDialog::Setup(tr("Set Zoom Level"),mNoDlgTitle,mNoHelpKey)
				.applybutton(true))
    , x1pospercm_(x1pospercm)
    , x2pospercm_(x2pospercm)
    , isvertical_(isvertical)
{
    setCtrlStyle( CloseOnly );

    uiSeparator* sep = nullptr;
    if ( isvertical && !mIsUdf(x1start) && !mIsUdf(x2start) )
    {
	uiString x1lbl = tr("First %1").arg(
		    isvertical ? uiStrings::sTrace() : uiStrings::sInline() );
	x1startfld_ = new uiGenInput( this, x1lbl, IntInpSpec() );
	x1startfld_->setElemSzPol( uiObject::Medium );
	x1startfld_->setValue( mNINT32(x1start) );

	uiString x2lbl = tr("First Z (%1)").arg( SI().zDomain().uiUnitStr() );
	x2startfld_ = new uiGenInput( this, x2lbl, FloatInpSpec() );
	x2startfld_->setValue( x2start*SI().zDomain().userFactor() );
	x2startfld_->attach( alignedBelow, x1startfld_ );

	sep = new uiSeparator( this, "Hor Sep" );
	sep->attach( stretchedBelow, x2startfld_ );
    }

    const bool usesi = !SI().xyInFeet();
    unitflds_ = new uiGenInput( this, uiStrings::sUnit(),
			BoolInpSpec(usesi,tr("cm"),tr("inches")) );
    if ( x2startfld_ )
    {
	unitflds_->attach( alignedBelow, x2startfld_ );
	unitflds_->attach( ensureBelow, sep );
    }

    mAttachCB( unitflds_->valuechanged, uiFlatViewZoomLevelDlg::unitChgCB );

    x1fld_ = new uiGenInput( this, getFieldLabel(true,usesi), FloatInpSpec() );
    x1fld_->attach( alignedBelow, unitflds_ );

    if ( isvertical )
    {
	x2fld_ = new uiGenInput( this, getFieldLabel(false,usesi),
				 FloatInpSpec() );
	x2fld_->attach( alignedBelow, x1fld_ );
    }

    saveglobalfld_ = new uiCheckBox( this, tr("Use for all new viewers") );
    saveglobalfld_->attach( alignedBelow, isvertical ? x2fld_ : x1fld_ );

    mAttachCB( applyPushed, uiFlatViewZoomLevelDlg::applyCB );
    mAttachCB( postFinalize(), uiFlatViewZoomLevelDlg::finalizeDoneCB );
}


uiFlatViewZoomLevelDlg::~uiFlatViewZoomLevelDlg()
{
    detachAllNotifiers();
}


uiString uiFlatViewZoomLevelDlg::getFieldLabel(bool x1, bool incm) const
{
    uiString lbl = toUiString("%1 %2")
	.arg( x1 ? "Traces per" : "Z Samples per" )
	.arg( incm ? "cm" : "inch" );
    return lbl;
}


void uiFlatViewZoomLevelDlg::finalizeDoneCB( CallBacker* )
{
    x1fld_->setNrDecimals( 2 );
    if ( x2fld_ )
	x2fld_->setNrDecimals( 2 );

    if ( x2startfld_ )
	x2startfld_->setNrDecimals( SI().nrZDecimals() );

    unitChgCB( nullptr );
}


void uiFlatViewZoomLevelDlg::unitChgCB( CallBacker* )
{
    const bool incm = unitflds_->getBoolValue();
    const float fact = incm ? 1.f : 2.54f;

    x1fld_->setValue( x1pospercm_*fact );
    x1fld_->setTitleText( getFieldLabel(true,incm) );
    if ( x2fld_ )
    {
	x2fld_->setValue( x2pospercm_*fact );
	x2fld_->setTitleText( getFieldLabel(false,incm) );
    }
}


void uiFlatViewZoomLevelDlg::computeZoomValues()
{
    const bool incm = unitflds_->getBoolValue();
    const float fact = incm ? 1.f : 2.54f;
    x1pospercm_ = x1fld_->getFValue() / fact;
    x2pospercm_ = x2fld_ ? x2fld_->getFValue()/fact : x1pospercm_;
}


bool uiFlatViewZoomLevelDlg::acceptOK( CallBacker* )
{
    computeZoomValues();
    return true;
}


void uiFlatViewZoomLevelDlg::applyCB( CallBacker* )
{
    computeZoomValues();
    if ( saveglobalfld_->isChecked() )
	uiFlatViewStdControl::setGlobalZoomLevel( x1pospercm_, x2pospercm_,
						  isvertical_ );
}


void uiFlatViewZoomLevelDlg::getNrPosPerCm( float &x1, float &x2 ) const
{
    x1 = x1pospercm_;
    x2 = x2pospercm_;
}


void uiFlatViewZoomLevelDlg::getStartPos( float& x1, float& x2 ) const
{
    x1 = x1startfld_ ? x1startfld_->getFValue() : mUdf(float);

    float factor = 1;
    if ( isvertical_ )
	factor = SI().zDomain().userFactor();

    x2 = x2startfld_ ? x2startfld_->getFValue()/factor : mUdf(float);
}


#define cb(fnm) mCB(this,uiFlatViewStdControl,fnm)

#define mDefBut(fnm,cb,tt,istoggle) \
    tb->addButton(fnm,tt,cb,istoggle);

// uiFlatViewStdControl
uiFlatViewStdControl::uiFlatViewStdControl( uiFlatViewer& vwr,
					    const Setup& setup )
    : uiFlatViewControl(vwr,setup.parent_,setup.withrubber_)
    , vwr_(vwr)
    , setup_(setup)
    , mousepressed_(false)
    , menu_(*new uiMenuHandler(0,-1))
    , propertiesmnuitem_(m3Dots(tr("Properties")),100)
{
    if ( setup_.withfixedaspectratio_ )
	getGlobalZoomLevel( defx1pospercm_, defx2pospercm_, setup.isvertical_ );

    uiToolBar::ToolBarArea tba( setup.withcoltabed_ ? uiToolBar::Left
						    : uiToolBar::Top );
    if ( setup.tba_ > 0 )
	tba = (uiToolBar::ToolBarArea)setup.tba_;
    tb_ = new uiToolBar( mainwin(), tr("Flat Viewer Tools"), tba );

    uiToolBar* tb = tb_;
    if ( setup.withzoombut_ || setup.isvertical_ )
    {
	rubbandzoombut_ = mDefBut("rubbandzoom",cb(dragModeCB),
				    tr( "Rubberband zoom" ), true )
    }

    if ( setup.withzoombut_ )
    {
	zoominbut_ = mDefBut("zoomforward",cb(zoomCB),
			tr("Zoom in"),false)
	zoomoutbut_ = mDefBut("zoombackward",cb(zoomCB),
			tr("Zoom out"),false)
    }

    if ( setup.isvertical_ )
    {
	vertzoominbut_ = mDefBut("vertzoomin",cb(zoomCB),
			tr("Vertical zoom in"),false)
	vertzoomoutbut_ = mDefBut("vertzoomout",cb(zoomCB),
			tr("Vertical zoom out"),false)
    }

    if ( setup.withzoombut_ || setup.isvertical_ )
    {
	cancelzoombut_ = mDefBut("cancelzoom",cb(cancelZoomCB),
			    tr("Cancel zoom"),false)
	if ( setup.withfixedaspectratio_ )
	{
	    fittoscrnbut_ = mDefBut("exttofullsurv",cb(fitToScreenCB),
				tr("Fit to screen"),false)
	}
    }

    if ( setup.withhomebutton_ )
    {
	sethomezoombut_ = mDefBut("set_homezoom",cb(homeZoomOptSelCB),
					tr("Set home zoom"),false)
	const CallBack optcb = cb(homeZoomOptSelCB);
	auto* mnu = new uiMenu( tb_, tr("Zoom level options") );
	mnu->insertAction( new uiAction(tr("Set local home zoom"),
					   optcb,"set_homezoom"), sLocalHZIdx );
	mnu->insertAction( new uiAction(tr("Set global home zoom"),
					optcb,"set_ghomezoom"), sGlobalHZIdx );
	mnu->insertAction( new uiAction(tr("Manually set home zoom"),
					optcb,"man_homezoom"), sManHZIdx );
	tb_->setButtonMenu( sethomezoombut_, mnu, uiToolButton::InstantPopup );
	gotohomezoombut_ = mDefBut("homezoom",cb(gotoHomeZoomCB),
		tr("Go to home zoom"), false )
	tb_->setSensitive( gotohomezoombut_, !mIsUdf(defx1pospercm_) &&
					     !mIsUdf(defx2pospercm_) );
    }

    if ( setup.withflip_ )
    {
	mDefBut("flip_lr",cb(flipCB),uiStrings::sFlipLeftRight(),false)
    }

    if ( setup.withsnapshot_ )
    {
	vwr_.rgbCanvas().enableImageSave();
	tb_->addObject( vwr_.rgbCanvas().getSaveImageButton(tb_) );
	tb_->addObject( vwr_.rgbCanvas().getPrintImageButton(tb_) );
    }

    if ( setup.withscalebarbut_ )
    {
	scalebarbut_ = mDefBut("scale",cb(displayScaleBarCB),
			tr("Display Scale Bar"),true)
    }

#ifdef __debug__
    if ( setup.withcoltabinview_ )
    {
	coltabbut_ = mDefBut("colorbar",cb(displayColTabCB),
			tr("Display Color Bar"),true)
    }
#endif

    tb_->addSeparator();
    parsbut_ = mDefBut("2ddisppars",cb(parsCB),
			tr("Set Display Parameters"),false);

    if ( setup.withcoltabed_ )
    {
	uiColorTableToolBar* coltabtb = new uiColorTableToolBar( mainwin() );
	ctabed_ = new uiFlatViewColTabEd( *coltabtb );
	coltabtb->display( vwr.rgbCanvas().prefHNrPics()>=400 );
	if ( setup.managecoltab_ )
	{
	    mAttachCB( ctabed_->colTabChgd, uiFlatViewStdControl::coltabChg );
	    mAttachCB( vwr.dispParsChanged, uiFlatViewStdControl::dispChgCB );
	}
    }

    if ( !setup.helpkey_.isEmpty() )
    {
	mDefBut("contexthelp",cb(helpCB),uiStrings::sHelp(),false);
	helpkey_ = setup.helpkey_;
    }

    if ( setup.withedit_ )
    {
	edittb_ = new uiToolBar( mainwin(), tr("Edit Tools") );
	tb = edittb_;
	editbut_ = mDefBut("seedpickmode",cb(editModeCB),
			    tr("Edit mode"), true );
	tb_->setShortcut( editbut_, "space" );
    }

    menu_.ref();
    mAttachCB( menu_.createnotifier, uiFlatViewStdControl::createMenuCB );
    mAttachCB( menu_.handlenotifier, uiFlatViewStdControl::handleMenuCB );
    mAttachCB( zoomChanged, uiFlatViewStdControl::zoomChgCB );
    mAttachCB( rubberBandUsed, uiFlatViewStdControl::rubBandUsedCB );
}


uiFlatViewStdControl::~uiFlatViewStdControl()
{
    detachAllNotifiers();
    delete ctabed_;
    menu_.unRef();
    MouseCursorManager::restoreOverride();
}


void uiFlatViewStdControl::finalPrepare()
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	uiGraphicsView& view = vwrs_[idx]->rgbCanvas();
	if ( setup_.withfixedaspectratio_ )
	{
	    vwrs_[idx]->updateBitmapsOnResize( false );
	    mAttachCBIfNotAttached(
		    view.reSize, uiFlatViewStdControl::aspectRatioCB );
	}

	MouseEventHandler& mevh = view.getNavigationMouseEventHandler();
	mAttachCBIfNotAttached(
		mevh.wheelMove, uiFlatViewStdControl::wheelMoveCB );

	if ( setup_.withhanddrag_ )
	{
	    mAttachCBIfNotAttached(
		    mevh.buttonPressed, uiFlatViewStdControl::handDragStarted );
	    mAttachCBIfNotAttached(
		    mevh.buttonReleased, uiFlatViewStdControl::handDragged );
	    mAttachCBIfNotAttached(
		    mevh.movement, uiFlatViewStdControl::handDragging );
	}

	mAttachCBIfNotAttached( view.gestureEventHandler().pinchnotifier,
				uiFlatViewStdControl::pinchZoomCB );
    }

    updatePosButtonStates();
}


void uiFlatViewStdControl::clearToolBar()
{
    delete mainwin()->removeToolBar( tb_ );
    zoominbut_ = zoomoutbut_ = rubbandzoombut_ = parsbut_ = editbut_ = -1;
    vertzoominbut_ = vertzoomoutbut_ = cancelzoombut_ = -1;
    tb_ = nullptr;
}


void uiFlatViewStdControl::updatePosButtonStates()
{
    uiToolBar* tb = toolBar();
    if ( !tb || setup_.withfixedaspectratio_ )
	return;

    const bool yn = !zoommgr_.atStart();
    if ( tb->findAction( zoomoutbut_ ) )
	tb->setSensitive( zoomoutbut_, yn );
    if ( tb->findAction(vertzoomoutbut_) )
	tb->setSensitive( vertzoomoutbut_, yn );
    if ( tb->findAction(cancelzoombut_) )
	tb->setSensitive( cancelzoombut_, yn );
}


void uiFlatViewStdControl::dispChgCB( CallBacker* )
{
    if ( ctabed_ ) ctabed_->setColTab( vwr_.appearance().ddpars_.vd_ );
}


void uiFlatViewStdControl::zoomChgCB( CallBacker* )
{
    updatePosButtonStates();
}


void uiFlatViewStdControl::rubBandUsedCB( CallBacker* )
{
    uiToolBar* tb = toolBar();
    const uiAction* rubbandzoomact = tb->findAction( rubbandzoombut_ );
    if (tb && rubbandzoomact && tb->isOn(rubbandzoombut_) )
    {
	tb->turnOn( rubbandzoombut_, false );
	dragModeCB( const_cast<uiAction*>(rubbandzoomact) );
    }
}


void uiFlatViewStdControl::aspectRatioCB( CallBacker* cb )
{
    mCBCapsuleGet(uiSize,caps,cb);
    mDynamicCastGet(uiGraphicsView*,view,caps->caller);
    int vwridx = -1;
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	if ( &vwrs_[idx]->rgbCanvas() == view )
	    { vwridx = idx; break; }
    if ( vwridx == -1 ) return;

    uiFlatViewer& vwr = *vwrs_[vwridx];
    const uiWorldRect bb = vwr.boundingBox();
    const uiWorld2Ui& w2ui = vwr.getWorld2Ui();
    const uiWorldRect wr = w2ui.transform( vwr.getViewRect(false) );
    vwr.setBoundingRect( w2ui.transform(bb) );
    vwr.setView( getZoomOrPanRect(wr.centre(),wr.size(),wr,bb) );
    updateZoomManager();
}


void uiFlatViewStdControl::wheelMoveCB( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || !meh->hasEvent() ) return;

    const MouseEvent& ev = meh->event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    const int butid = ev.angle()>0 ?
	(ev.shiftStatus() ? vertzoominbut_ : zoominbut_) :
	(ev.shiftStatus() ? vertzoomoutbut_ : zoomoutbut_);

    uiToolBar* tb = toolBar();
    uiAction* action = tb ? const_cast<uiAction*>(tb->findAction( butid ) )
			  : nullptr;
    zoomCB( action );
}


void uiFlatViewStdControl::pinchZoomCB( CallBacker* cb )
{
    mDynamicCastGet(const GestureEventHandler*,evh,cb);
    if ( !evh || evh->isHandled() || vwrs_.isEmpty() )
	return;

    const GestureEvent* gevent = evh->getPinchEventInfo();
    if ( !gevent )
	return;

    uiFlatViewer& vwr = *vwrs_[0];
    const Geom::Size2D<double> cursz = vwr.curView().size();
    const double scalefac = double( gevent->scale() );
    Geom::Size2D<double> newsz( cursz.width() * (1/scalefac),
				cursz.height() * (1/scalefac) );
    Geom::Point2D<double> pos = vwr.getWorld2Ui().transform( gevent->pos() );

    const uiWorldRect wr = getZoomOrPanRect( pos, newsz, vwr.curView(),
					     vwr.boundingBox() );
    vwr.setView( wr );

    if ( gevent->getState() == GestureEvent::Finished )
	updateZoomManager();
}


void uiFlatViewStdControl::zoomCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,uiact,cb);
    if ( !uiact ) return;

    const int butid = uiact->getID();
    const bool zoomin = butid == zoominbut_ || butid == vertzoominbut_;
    const bool onlyvertzoom = butid == vertzoominbut_ ||
			      butid == vertzoomoutbut_;
    doZoom( zoomin, onlyvertzoom, *vwrs_[0] );
}


void uiFlatViewStdControl::doZoom( bool zoomin, bool onlyvertzoom,
				   uiFlatViewer& vwr )
{
    const int vwridx = vwrs_.indexOf( &vwr );
    if ( vwridx<0 || (!zoomin && zoommgr_.atStart(vwridx)) )
	return;

    const MouseEventHandler& meh =
	vwr.rgbCanvas().getNavigationMouseEventHandler();
    const bool hasmouseevent = meh.hasEvent();

    const Geom::Point2D<double> mousepos = hasmouseevent ?
	vwr.getWorld2Ui().transform(meh.event().pos()) : vwr.curView().centre();
    const Geom::Size2D<double> newsz =
	zoomin ? zoommgr_.forward(vwridx,onlyvertzoom,true)
	       : zoommgr_.back(vwridx,onlyvertzoom,hasmouseevent);
    setNewView( mousepos, newsz, &vwr );
}


void uiFlatViewStdControl::cancelZoomCB( CallBacker* )
{
    if ( !setup_.withfixedaspectratio_ )
	{ reInitZooms(); return; }

    const uiRect bbrect = vwr_.getWorld2Ui().transform( vwr_.boundingBox() );
    const float aspectratio = sCast(float,bbrect.width())/bbrect.height();
    const uiRect viewrect = vwr_.getViewRect( false );
    int height = viewrect.height();
    int width = sCast(int,aspectratio*height);
    if ( width > viewrect.width() )
    {
	width = viewrect.width();
	height = sCast(int,width/aspectratio);
    }

    vwr_.setBoundingRect( uiRect(0,0,width,height) );
    vwr_.setViewToBoundingBox();
    updateZoomManager();
}


void uiFlatViewStdControl::fitToScreenCB( CallBacker* )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setExtraBorders( uiSize(), uiSize() );
    reInitZooms();
}


void uiFlatViewStdControl::homeZoomOptSelCB( CallBacker* cb )
{
    float x1pospercm = getCurrentPosPerCM( true );
    float x2pospercm = getCurrentPosPerCM( false );

    mDynamicCastGet(uiAction*,itm,cb)
    const int itmid = itm ? itm->getID() : sLocalHZIdx;
    uiToolBar* tb = toolBar();
    if ( itmid == sLocalHZIdx )
    {
	defx1pospercm_ = x1pospercm;
	defx2pospercm_ = x2pospercm;
	tb->setSensitive( gotohomezoombut_, true );
    }
    else if ( itmid == sGlobalHZIdx )
	setGlobalZoomLevel( x1pospercm, x2pospercm, setup_.isvertical_ );
    else
    {
	const uiWorldRect& curview = vwrs_[0]->curView();
	x1start_ = mNINT32( curview.left() );
	x2start_ = curview.top();

	uiFlatViewZoomLevelDlg zoomlvldlg( this, x1start_, x2start_,
					   x1pospercm, x2pospercm,
					   setup_.isvertical_ );
	mAttachCB( zoomlvldlg.applyPushed, uiFlatViewStdControl::zoomApplyCB );
	zoomlvldlg.go();
    }
}


void uiFlatViewStdControl::zoomApplyCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewZoomLevelDlg*,dlg,cb)
    if ( !dlg )
	return;

    dlg->getNrPosPerCm( defx1pospercm_, defx2pospercm_ );

    dlg->getStartPos( x1start_, x2start_ );
    updateZoomLevel( x1start_, x2start_, defx1pospercm_, defx2pospercm_ );
}


#define sInchToCMFac 2.54f

float uiFlatViewStdControl::getCurrentPosPerCM( bool forx1 ) const
{
    const uiFlatViewer& vwr = *vwrs_[0];
    const uiRect bbrect = vwr.getWorld2Ui().transform(vwr.boundingBox());
    const int nrpixels = forx1 ? bbrect.hNrPics() : bbrect.vNrPics();
    const OD::Pair<int,int> dpi = uiMain::getDPI();
    const int dpiindir = forx1 ? dpi.first() : dpi.second();
    const float nrcms = (mCast(float,nrpixels)/dpiindir) * sInchToCMFac;
    const int extrastep = forx1 ? 1 : 0;
    //<-- For x2 all points are not considered as flatviewer does not expand
    //<-- boundingbox by extfac_(0.5) along x2 as wiggles are not drawn in
    //<-- extended area.
    return (vwr.posRange(forx1).nrSteps() + extrastep) / nrcms;
}


void uiFlatViewStdControl::setGlobalZoomLevel(
		float x1pospercm, float x2pospercm, bool isvertical )
{
    IOPar& sipars = SI().getPars();
    sipars.set( sKeyVW2DTrcsPerCM(), x1pospercm );
    if ( isvertical )
	sipars.set( sKeyVW2DZPerCM(), x2pospercm );

    SI().savePars();
}


void uiFlatViewStdControl::getGlobalZoomLevel(
		float& x1pospercm, float& x2pospercm, bool isvertical )
{
    const IOPar& sipars = SI().getPars();
    sipars.get( sKeyVW2DTrcsPerCM(), x1pospercm );
    if ( isvertical )
	sipars.get( sKeyVW2DZPerCM(), x2pospercm );
    else
	x2pospercm = x1pospercm;
}


void uiFlatViewStdControl::gotoHomeZoomCB( CallBacker* )
{
    setViewToCustomZoomLevel( *vwrs_[0] );
}


void uiFlatViewStdControl::setViewToCustomZoomLevel( uiFlatViewer& vwr )
{
    if ( !vwr.rgbCanvas().mainwin() )
	return;

    const bool ispoppedup = vwr.rgbCanvas().mainwin()->poppedUp();
    const float x1pospercm = (ispoppedup || mIsUdf(setup_.initialx1pospercm_))
			   ? defx1pospercm_ : setup_.initialx1pospercm_;
    const float x2pospercm = (ispoppedup || mIsUdf(setup_.initialx2pospercm_))
			   ? defx2pospercm_ : setup_.initialx2pospercm_;
    if ( mIsUdf(x1pospercm) || mIsUdf(x2pospercm) || mIsZero(x1pospercm,0.01) ||
	 mIsZero(x2pospercm,0.01) )
    {
	if ( !ispoppedup ) vwr.setViewToBoundingBox();
	return;
    }

    updateZoomLevel( x1pospercm, x2pospercm );
}


void uiFlatViewStdControl::updateZoomLevel( float x1start, float x2start,
					float x1pospercm,float x2pospercm )
{
    const uiRect viewrect = vwr_.getViewRect( false );
    const OD::Pair<int,int> screendpi = uiMain::getDPI();
    const float cmwidth = ((float)viewrect.width()/screendpi.first())
				* sInchToCMFac;
    const float cmheight = ((float)viewrect.height()/screendpi.second())
				* sInchToCMFac;
    const double hwdth = vwr_.posRange(true).step * cmwidth * x1pospercm / 2;
    const double hhght = vwr_.posRange(false).step * cmheight * x2pospercm / 2;

    const uiWorldRect bb = vwr_.boundingBox();
    uiWorldRect newwr;
    uiWorldPoint centerpoint;
    if ( mIsUdf(x1start) || mIsUdf(x2start) )
    {
	const bool ispoppedup = vwr_.rgbCanvas().mainwin()->poppedUp();
	centerpoint = !ispoppedup ? setup_.initialcentre_
				  : vwr_.curView().centre();
	if ( centerpoint == uiWorldPoint::udf() )
	    centerpoint = bb.centre();

	newwr = uiWorldRect( centerpoint.x-hwdth, centerpoint.y-hhght,
			     centerpoint.x+hwdth, centerpoint.y+hhght );
    }
    else
    {
	newwr = uiWorldRect( x1start, x2start,
			     x1start+hwdth*2, x2start+hhght*2 );
	centerpoint = newwr.centre();
    }

    vwr_.setBoundingRect( uiWorld2Ui(viewrect,newwr).transform(bb) );
    vwr_.setView( getZoomOrPanRect(centerpoint,newwr.size(),newwr,bb) );
    updateZoomManager();
}


void uiFlatViewStdControl::updateZoomLevel( float x1pospercm, float x2pospercm )
{
    updateZoomLevel( mUdf(float), mUdf(float), x1pospercm, x2pospercm );
}


void uiFlatViewStdControl::setVwrCursor( uiFlatViewer& vwr,
					 const MouseCursor& cursor )
{
    vwr.rgbCanvas().setCursor( cursor );
    vwr.setCursor( cursor );

    for ( int idx=0; idx<vwr.nrAuxData(); idx++ )
    {
	FlatView::AuxData* ad = vwr.getAuxData( idx );
	if ( !ad )
	    continue;
	ad->cursor_ = cursor;
    }

    vwr.bitmapDisp()->getDisplay()->setCursor( cursor );
    vwr.handleChange( FlatView::Viewer::Auxdata );
}


void uiFlatViewStdControl::handDragStarted( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || !meh->event().middleButton() ) return;

    MouseCursor cursor( MouseCursor::ClosedHand );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	setVwrCursor( *vwrs_[idx], cursor );

    MouseCursorManager::setOverride( cursor.shape_ );
    mousedownpt_ = meh->event().pos();
    mousepressed_ = true;
}


void uiFlatViewStdControl::handDragging( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh )
	return;

    const int vwridx = getViewerIdx( meh, false );
    if ( vwridx<0 )
	return;

    uiFlatViewer* vwr = vwrs_[vwridx];
    const uiWorld2Ui& w2ui = vwr->getWorld2Ui();
    const uiWorldPoint startwpt = w2ui.transform( mousedownpt_ );
    const uiWorldPoint curwpt = w2ui.transform( meh->event().pos() );
    mousedownpt_ = meh->event().pos();
    if ( !mousepressed_ )
	return;

    uiWorldRect newwr( vwr->curView() );
    newwr.translate( startwpt-curwpt );

    newwr = getZoomOrPanRect( newwr.centre(), newwr.size(), newwr,
			      vwr->boundingBox() );
    vwr->setView( newwr );
}


void uiFlatViewStdControl::handDragged( CallBacker* cb )
{
    if ( !mousepressed_ )
	return;

    handDragging( cb );
    MouseCursorManager::restoreOverride();
    MouseCursor cursor( !isEditModeOn() ? MouseCursor::Arrow
					: MouseCursor::Cross );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	setVwrCursor( *vwrs_[idx], cursor );

    mousepressed_ = false;
}


void uiFlatViewStdControl::flipCB( CallBacker* )
{
    flip( true );
}


void uiFlatViewStdControl::displayScaleBarCB( CallBacker* )
{
    const bool doshowscalebar = toolBar()->isOn( scalebarbut_ );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	uiFlatViewer& vwr = *vwrs_[idx];
	const bool updateonresize = vwr.updatesBitmapsOnResize();
	vwr.updateBitmapsOnResize( true );
	const float x1pospercm = getCurrentPosPerCM( true );
	const float x2pospercm = getCurrentPosPerCM( false );

	vwr.appearance().annot_.showscalebar_ = doshowscalebar;
	vwr.handleChange( FlatView::Viewer::Annot );

	vwr.updateBitmapsOnResize( updateonresize );
	updateZoomLevel( x1pospercm, x2pospercm );
    }
}


void uiFlatViewStdControl::displayColTabCB( CallBacker* )
{
    vwr_.appearance().annot_.showcolorbar_ = toolBar()->isOn( coltabbut_ );
    vwr_.handleChange( FlatView::Viewer::Annot );
}


void uiFlatViewStdControl::parsCB( CallBacker* )
{
    doPropertiesDialog();
}


void uiFlatViewStdControl::setEditMode( bool yn )
{
    uiToolBar* edittb = editToolBar();
    if ( edittb )
    {
	edittb->turnOn( editbut_, yn );
	auto* action = cCast(uiAction*,edittb->findAction(editbut_));
	editModeCB( action );
    }
}


bool uiFlatViewStdControl::isEditModeOn() const
{
    const uiToolBar* edittb =
	const_cast<uiFlatViewStdControl*>(this)->editToolBar();
    return edittb && edittb->isOn( editbut_ );
}


bool uiFlatViewStdControl::isRubberBandOn() const
{
    const uiToolBar* tb = const_cast<uiFlatViewStdControl*>(this)->toolBar();
    return tb && tb->findAction( rubbandzoombut_ ) &&
	   tb->isOn( rubbandzoombut_ );
}


void uiFlatViewStdControl::dragModeCB( CallBacker* )
{
    uiToolBar* tb = toolBar();
    uiToolBar* edittb = editToolBar();
    const bool iseditmode = edittb && edittb->isOn( editbut_ );
    const bool iszoommode = tb && tb->findAction( rubbandzoombut_ ) &&
			    tb->isOn( rubbandzoombut_ );

    bool editable = false;
    uiGraphicsViewBase::ODDragMode mode( uiGraphicsViewBase::NoDrag );
    MouseCursor cursor( MouseCursor::Arrow );
    if ( !iszoommode && iseditmode )
    {
	cursor = MouseCursor::Cross;
	editable = true;
    }
    if ( iszoommode )
	mode = uiGraphicsViewBase::RubberBandDrag;

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	vwrs_[idx]->rgbCanvas().setDragMode( mode );
	setVwrCursor( *vwrs_[idx], cursor );
	vwrs_[idx]->appearance().annot_.editable_ = editable;
	// TODO: Change while enabling tracking in Z-transformed 2D Viewers.
    }
}


void uiFlatViewStdControl::editModeCB( CallBacker* )
{
    uiToolBar* tb = toolBar();
    uiToolBar* edittb = editToolBar();
    const bool iseditmode = edittb && edittb->isOn( editbut_ );
    const bool iszoommode = tb && tb->findAction( rubbandzoombut_ ) &&
			    tb->isOn( rubbandzoombut_ );
    if ( iszoommode )
	tb->turnOn( rubbandzoombut_, false );

    MouseCursor cursor( iseditmode ? MouseCursor::Cross : MouseCursor::Arrow );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	setVwrCursor( *vwrs_[idx], cursor );
	vwrs_[idx]->appearance().annot_.editable_ =
	    iseditmode && !vwrs_[idx]->hasZAxisTransform();
	// TODO: Change while enabling tracking in Z-transformed 2D Viewers.
    }
}


void uiFlatViewStdControl::helpCB( CallBacker* )
{
    HelpProvider::provideHelp( helpkey_ );
}


bool uiFlatViewStdControl::handleUserClick( int vwridx )
{
    const MouseEvent& ev = mouseEventHandler(vwridx,true).event();
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	  !ev.altStatus() )
    {
	menu_.executeMenu(0);
	return true;
    }
    return false;
}


void uiFlatViewStdControl::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;

    mAddMenuItem( menu, &propertiesmnuitem_, true, false );
}


void uiFlatViewStdControl::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    const bool ishandled = mnuid==propertiesmnuitem_.id ;
    if ( ishandled )
	doPropertiesDialog();

    menu->setIsHandled( ishandled );
}


void uiFlatViewStdControl::coltabChg( CallBacker* )
{
    vwr_.appearance().ddpars_.vd_ = ctabed_->getDisplayPars();
    vwr_.handleChange( FlatView::Viewer::DisplayPars );
}


void uiFlatViewStdControl::keyPressCB( CallBacker* cb )
{
    mDynamicCastGet( const KeyboardEventHandler*, keh, cb );
    if ( !keh || !keh->hasEvent() ) return;

    uiToolBar* tb = toolBar();
    uiAction* rubbandzoombut =
	const_cast<uiAction*>( tb->findAction(rubbandzoombut_) );
    if ( tb && keh->event().key_==OD::KB_Escape && rubbandzoombut )
    {
	tb->turnOn( rubbandzoombut_, !tb->isOn(rubbandzoombut_) );
	dragModeCB( rubbandzoombut );
    }
}


NotifierAccess* uiFlatViewStdControl::editPushed()
{
    uiToolBar* edittb = editToolBar();
    uiAction* editact = edittb
		      ? const_cast<uiAction*>( edittb->findAction(editbut_) )
		      : nullptr;
    return editact ? &editact->toggled : nullptr;
}
