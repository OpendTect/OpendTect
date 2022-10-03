/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelltiecontrolview.h"

#include "ioman.h"
#include "keyboardevent.h"
#include "emsurfacetr.h"
#include "mouseevent.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "welltiepickset.h"
#include "welltiedata.h"
#include "welltiesetup.h"

#include "uiflatviewer.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uirgbarraycanvas.h"
#include "uiseparator.h"
#include "uitoolbutton.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uiwelldispprop.h"


static const char* sKeyZoom = "Viewer Zoom";

#define mErrRet(msg,act) \
{ uiMSG().error(msg); act; }

#define cb(fnm) mCB(this,uiControlView,fnm)

#define mDefBut(fnm,cbnm,tt,istoggle) \
    toolbar_->addButton( fnm, tt, cb(cbnm), istoggle );

WellTie::uiControlView::uiControlView( uiParent* p, uiToolBar* tb,
				       uiFlatViewer* vwr, Server& server )
    : uiFlatViewStdControl(*vwr, uiFlatViewStdControl::Setup()
			         .withcoltabed(false).withsnapshot(false))
    , toolbar_(tb)
    , curview_(uiWorldRect(0,0,0,0))
    , server_(server)
    , redrawNeeded(this)
    , redrawAnnotNeeded(this)
{
    mDynamicCastGet(uiMainWin*,mw,p)
    if ( mw )
	clearToolBar();
    else
	tb_->display(false);
    toolbar_->addSeparator();

    parsbut_ = mDefBut("2ddisppars",parsCB,tr("Set display parameters"),false);
    rubbandzoombut_ =
	mDefBut("rubbandzoom",dragModeCB,tr("Rubberband zoom"), true );
    vertzoominbut_ =
	mDefBut("vertzoomin",zoomCB,tr("Vertical zoom in"), false );
    vertzoomoutbut_ =
	mDefBut("vertzoomout",zoomCB,tr("Vertical zoom out"), false );
    cancelzoombut_ =
	mDefBut("cancelzoom",cancelZoomCB,tr("Cancel zoom"), false );
    editbut_ = mDefBut("seedpickmode",dragModeCB,tr("Pick mode (P)"), true );

    tb->addSeparator();
    horbut_ =
	mDefBut("loadhoronseis",loadHorizons,tr("Load Horizon(s)"), false );
    hormrkdispbut_ = mDefBut("drawhoronseis",dispHorMrks,
		    tr("Marker display properties"), false );
    tb->addSeparator();

    mAttachCB( vwr_.viewChanged, uiControlView::viewChangedCB );
}


WellTie::uiControlView::~uiControlView()
{
    detachAllNotifiers();
}


bool WellTie::uiControlView::handleUserClick( int vwridx )
{
    const MouseEvent& ev = mouseEventHandler(vwridx,true).event();
    const uiWorldPoint wp = vwr_.getWorld2Ui().transform( ev.pos() );
    if ( ev.leftButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	!ev.altStatus() && editToolBar()->isOn( editbut_ ) &&
	checkIfInside(wp.x,wp.y) )
    {
	vwr_.getAuxInfo( wp, infopars_ );
	const uiWorldRect& bbox = vwr_.boundingBox();
	const bool synth = ( wp.x < (bbox.right()-bbox.left())/2 );
	const SeisTrc* trc = synth ? server_.data().getSynthTrc()
				   : server_.data().getRealTrc();
	if ( !trc )
	    return false;

	server_.pickMgr().addPick( (float) wp.y, synth, trc );
	redrawAnnotNeeded.trigger();
	return true;
    }
    return false;
}


bool WellTie::uiControlView::checkIfInside( double xpos, double zpos )
{
    const uiWorldRect& bbox = vwr_.boundingBox();
    const Interval<double> xrg( bbox.left(), bbox.right() ),
			   zrg( bbox.bottom(), bbox.top() );
    if ( !xrg.includes( xpos, false ) || !zrg.includes( zpos, false ) )
	{
	mErrRet(tr("Please select your pick inside the work area"),
	    return false);
	}
    return true;
}


void WellTie::uiControlView::rubBandCB( CallBacker* cb )
{
    setSelView();
    rubberBandUsed.trigger();
}


void WellTie::uiControlView::wheelMoveCB( CallBacker* )
{
    if ( !vwr_.rgbCanvas().
	getNavigationMouseEventHandler().hasEvent() )
	return;

    const MouseEvent& ev =
	vwr_.rgbCanvas().getNavigationMouseEventHandler().event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    const int actid = ev.angle() < 0 ? vertzoominbut_ : vertzoomoutbut_;

    zoomCB( const_cast<uiAction*>(toolBar()->findAction(actid)) );
}


void WellTie::uiControlView::keyPressCB( CallBacker* )
{
    const KeyboardEvent& ev =
	vwr_.rgbCanvas().getKeyboardEventHandler().event();
    if ( ev.key_ == OD::KB_P )
	setEditMode( !editToolBar()->isOn(editbut_) );
}


void WellTie::uiControlView::viewChangedCB( CallBacker* )
{
    // TODO: Check if this is really necessary.
    curview_ = vwr_.curView();
}


void WellTie::uiControlView::setSelView( bool isnewsel, bool viewall )
{
    uiWorldRect wr = (curview_.height() > 0)  ? curview_ : vwr_.boundingBox();
    if ( isnewsel && vwr_.rgbCanvas().getSelectedArea() )
    {
	const uiRect viewarea = *vwr_.rgbCanvas().getSelectedArea();
	if ( viewarea.topLeft() == viewarea.bottomRight() ||
		viewarea.width() < 10 || viewarea.height() < 10 )
	    return;
	wr = vwr_.getWorld2Ui().transform( viewarea );
    }
    const uiWorldRect bbox = vwr_.boundingBox();
    Interval<double> zrg( wr.top() , wr.bottom() );
    if ( viewall )
	zrg.set( bbox.top(), bbox.bottom() );
    wr.setTopBottom( zrg );
    Interval<double> xrg( bbox.left(), bbox.right());
    wr.setLeftRight( xrg );

    const Geom::Size2D<double> newsz = wr.size();
    wr = getZoomOrPanRect( wr.centre(), newsz, wr, bbox );
    vwr_.setView( wr );
    zoommgr_.add( newsz );
    zoomChanged.trigger();
}


namespace WellTie
{

class uiMrkDispDlg : public uiDialog
{ mODTextTranslationClass(uiMrkDispDlg);
public :

    uiMrkDispDlg( uiParent* p, DispParams& pms )
	: uiDialog(p,uiDialog::Setup(tr("Display Markers/Horizons"),
				     uiString::emptyString(),mNoHelpKey)
		.modal(false))
	, pms_(pms)
	, redrawneeded_(this)
    {
	setCtrlStyle( CloseOnly );
	uiGroup* topgrp = new uiGroup( this, "top group" );
	dispmrkfld_ = new uiCheckBox( topgrp, tr("display markers"));
	dispmrkfld_->setChecked( pms_.isvwrmarkerdisp_ );
	dispmrkfld_->activated.notify( mCB(this,uiMrkDispDlg,dispChged) );
	disphorfld_ = new uiCheckBox( topgrp, tr("display horizons"));
	disphorfld_->setChecked( pms_.isvwrhordisp_ );
	disphorfld_->activated.notify( mCB(this,uiMrkDispDlg,dispChged) );
	disphorfld_->attach( alignedBelow, dispmrkfld_ );

	dispmrkfullnamefld_ = new uiCheckBox( topgrp, tr("display full name") );
	dispmrkfullnamefld_->setChecked(
	pms_.isvwrmarkerdisp_ && pms_.dispmrkfullnames_ );
	dispmrkfullnamefld_->activated.notify(mCB(this,uiMrkDispDlg,dispChged));
	dispmrkfullnamefld_->attach( rightOf, dispmrkfld_ );
	disphorfullnamefld_ = new uiCheckBox( topgrp, tr("display full name") );
	disphorfullnamefld_->attach( rightOf, disphorfld_ );
	disphorfullnamefld_->setChecked(
			    pms_.isvwrhordisp_ && pms_.disphorfullnames_ );
	disphorfullnamefld_->activated.notify(mCB(this,uiMrkDispDlg,dispChged));

	uiSeparator* sep = new uiSeparator( this, "Well2Seismic Sep" );
	sep->attach( stretchedBelow, topgrp );

	mrkdispfld_ = new uiWellMarkersDispProperties( this,
		uiWellDispProperties::Setup(
		mJoinUiStrs(sMarker(),sSize().toLower()),
		mJoinUiStrs(sMarker(),sColor().toLower()))
		.onlyfor2ddisplay(true), pms_.mrkdisp_, pms_.allmarkernms_ );
	mrkdispfld_->attach( ensureBelow, sep );
	mrkdispfld_->attach( alignedBelow, topgrp );
	mrkdispfld_->putToScreen();
	mrkdispfld_->propChanged.notify(mCB(this,uiMrkDispDlg,dispChged));

	dispChged(0);
    }

    void dispChged( CallBacker* )
    {
	pms_.isvwrmarkerdisp_ = dispmrkfld_->isChecked();
	pms_.isvwrhordisp_ = disphorfld_->isChecked();
	pms_.dispmrkfullnames_ = dispmrkfullnamefld_->isChecked();
	pms_.disphorfullnames_ = disphorfullnamefld_->isChecked();
	dispmrkfullnamefld_->setSensitive( pms_.isvwrmarkerdisp_ );
	disphorfullnamefld_->setSensitive( pms_.isvwrhordisp_ );

	redrawneeded_.trigger();
    }

    Notifier<uiMrkDispDlg>	redrawneeded_;

protected:

    DispParams&		pms_;
    uiCheckBox*	dispmrkfld_;
    uiCheckBox*	disphorfld_;
    uiCheckBox*         dispmrkfullnamefld_;
    uiCheckBox*         disphorfullnamefld_;
    uiWellMarkersDispProperties* mrkdispfld_;
};

} // namespace WellTie


void WellTie::uiControlView::dispHorMrks( CallBacker* )
{
    if ( !mrkrdlg_ )
    {
	mrkrdlg_ = new uiMrkDispDlg( this, server_.dispParams() );
	mrkrdlg_->redrawneeded_.notify(mCB(this,uiControlView,reDrawNeeded) );
    }
    mrkrdlg_->go();
}


void WellTie::uiControlView::reDrawNeeded( CallBacker* )
{
    redrawAnnotNeeded.trigger();
}


void WellTie::uiControlView::loadHorizons( CallBacker* )
{
    const SeisIOObjInfo seisinfo( server_.data().setup().seisid_ );
    if ( !seisinfo.isOK() )
	return;

    const bool is2d = seisinfo.is2D();
    const IOObjContext horctxt = is2d ? mIOObjContext( EMHorizon2D )
				      : mIOObjContext( EMHorizon3D );
    if ( !selhordlg_ )
    {
	uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
	selhordlg_ = new uiIOObjSelDlg( this, sdsu, horctxt );
    }
    TypeSet<MultiID> horselids;
    if ( selhordlg_->go() )
	selhordlg_->getChosen( horselids );

    uiString errmsg; uiTaskRunner taskrunner( this );
    server_.horizonMgr().setUpHorizons( horselids, errmsg, taskrunner );
    if ( !errmsg.isEmpty() )
	{ mErrRet( errmsg, return ) }

    server_.dispParams().isvwrhordisp_ = true;
    redrawNeeded.trigger();
}


void WellTie::uiControlView::fillPar( IOPar& iop ) const
{
    iop.set( sKeyZoom, Interval<double>( curview_.top(), curview_.bottom() ) );
}


void WellTie::uiControlView::usePar( const IOPar& iop )
{
    Interval<double> zrg;
    iop.get( sKeyZoom, zrg );
    curview_.setTopBottom( zrg );
    setSelView( false, false );
    redrawNeeded.trigger();
}

void WellTie::uiControlView::applyProperties(CallBacker*)
{
    uiFlatViewControl::applyProperties(0);
    setSelView( true, true );
}
