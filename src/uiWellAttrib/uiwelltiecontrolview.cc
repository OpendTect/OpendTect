/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/


static const char* rcsID mUsedVar = "$Id$";

#include "uiwelltiecontrolview.h"

#include "flatviewzoommgr.h"
#include "keyboardevent.h"
#include "emsurfacetr.h"
#include "mouseevent.h"
#include "welltiepickset.h"
#include "welltiedata.h"
#include "welltiesetup.h"

#include "uiflatviewer.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uirgbarraycanvas.h"
#include "uiseparator.h"
#include "uitoolbutton.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uiwelldispprop.h"
#include "uiworld2ui.h"


static const char* sKeyZoom = "Viewer Zoom";

namespace WellTie
{

#define mErrRet(msg,act) \
{ uiMSG().error(msg); act; }
#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( toolbar_, fnm, tt, mCB(this,uiControlView,cbnm) ); \
    toolbar_->addButton( but );

uiControlView::uiControlView( uiParent* p, uiToolBar* tb, 
				uiFlatViewer* vwr, Server& server )
    : uiFlatViewStdControl(*vwr, uiFlatViewStdControl::Setup()
						    .withcoltabed(false))
    , toolbar_(tb)
    , manip_(true)
    , selhordlg_(0)
    , curview_(uiWorldRect(0,0,0,0))
    , server_(server)
    , redrawNeeded(this)
    , redrawAnnotNeeded(this)
    , mrkrdlg_(0)			     
{
    mDynamicCastGet(uiMainWin*,mw,p)
    if ( mw )
	mw->removeToolBar( tb_ );
    else
	tb_->display(false);
    toolbar_->addSeparator();
    toolbar_->addObject( vwr_.rgbCanvas().getSaveImageButton(toolbar_) );
    mDefBut(parsbut_,"2ddisppars",parsCB,"Set display parameters");
    mDefBut(zoominbut_,"zoomforward",altZoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward",altZoomCB,"Zoom out");
    mDefBut(manipdrawbut_,"altpick",stateCB,"Switch view mode (Esc)");
    mDefBut(editbut_,"seedpickmode",editCB,"Pick mode (P)");

    toolbar_->addSeparator();
    mDefBut(horbut_,"loadhoronseis",loadHorizons,"Load Horizon(s)");
    mDefBut(hormrkdispbut_,"drawhoronseis",dispHorMrks,
	    					"Marker display properties");
    editbut_->setToggleButton( true );

    vwr_.rgbCanvas().getKeyboardEventHandler().keyPressed.notify(
	                    mCB(this,uiControlView,keyPressCB) );
    toolbar_->addSeparator();
}


bool uiControlView::handleUserClick()
{
    const MouseEvent& ev = mouseEventHandler(0).event();
    uiWorld2Ui w2u; vwr_.getWorld2Ui(w2u);
    const uiWorldPoint wp = w2u.transform( ev.pos() );
    if ( ev.leftButton() && !ev.ctrlStatus() && !ev.shiftStatus() 
	&& !ev.altStatus() && editbut_->isOn() && checkIfInside(wp.x,wp.y) )
    {
	vwr_.getAuxInfo( wp, infopars_ );
	const uiWorldRect& bbox = vwr_.boundingBox();
	bool synth = ( wp.x < (bbox.right()-bbox.left())/2 );
	const SeisTrc& trc = synth ? server_.data().synthtrc_ 
				   : server_.data().seistrc_; 
	server_.pickMgr().addPick( (float) wp.y, synth, &trc );
	redrawAnnotNeeded.trigger();
	return true;
    }
    return false;
}


bool uiControlView::checkIfInside( double xpos, double zpos )
{
    const uiWorldRect& bbox = vwr_.boundingBox();
    const Interval<double> xrg( bbox.left(), bbox.right() ),
			   zrg( bbox.bottom(), bbox.top() );
    if ( !xrg.includes( xpos, false ) || !zrg.includes( zpos, false ) ) 
	{ mErrRet("Please select your pick inside the work area",return false);}
    return true;
}


void uiControlView::rubBandCB( CallBacker* cb )
{
    setSelView();
}


void uiControlView::altZoomCB( CallBacker* but )
{
    const uiWorldRect& bbox = vwr_.boundingBox();
    const Interval<double> xrg( bbox.left(), bbox.right());
    zoomCB( but );
    uiWorldRect wr = vwr_.curView();
    wr.setLeftRight( xrg );
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> size = wr.size();
    setNewView( centre, size );
    curview_ = wr;
}


void uiControlView::wheelMoveCB( CallBacker* )
{
    if ( !vwr_.rgbCanvas().
	getNavigationMouseEventHandler().hasEvent() )
	return;

    const MouseEvent& ev =
	vwr_.rgbCanvas().getNavigationMouseEventHandler().event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    altZoomCB( ev.angle() < 0 ? zoominbut_ : zoomoutbut_ );
}


void uiControlView::keyPressCB( CallBacker* )
{
    const KeyboardEvent& ev =
	vwr_.rgbCanvas().getKeyboardEventHandler().event();
    if ( ev.key_ == OD::P )
	setEditOn( !editbut_->isOn() ); 
}


void uiControlView::setSelView( bool isnewsel, bool viewall )
{
    uiWorldRect wr = (curview_.height() > 0)  ? curview_ : vwr_.boundingBox();
    if ( isnewsel && vwr_.rgbCanvas().getSelectedArea() )
    {
	const uiRect viewarea = *vwr_.rgbCanvas().getSelectedArea();
	if ( viewarea.topLeft() == viewarea.bottomRight() || 
		viewarea.width() < 10 || viewarea.height() < 10 )
	    return;
	uiWorld2Ui w2u; vwr_.getWorld2Ui( w2u );
	wr = w2u.transform( viewarea );
    }
    const uiWorldRect bbox = vwr_.boundingBox();
    Interval<double> zrg( wr.top() , wr.bottom() );
    if ( viewall )
	zrg.set( bbox.top(), bbox.bottom() );
    wr.setTopBottom( zrg );
    Interval<double> xrg( bbox.left(), bbox.right());
    wr.setLeftRight( xrg );
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> newsz = wr.size();

    curview_ = wr;
    setNewView( centre, newsz );
}


bool uiControlView::isZoomAtStart() const
{ return zoommgr_.atStart(); }


void uiControlView::setEditOn( bool yn )
{
    editbut_->setOn( yn );
    editCB( 0 );
}


class uiMrkDispDlg : public uiDialog
{
public :

    uiMrkDispDlg( uiParent* p, DispParams& pms )
	: uiDialog(p,uiDialog::Setup("Display Markers/Horizons","",mNoHelpID)
		.modal(false))
	, pms_(pms)
	, redrawneeded_(this)		   
    {
	setCtrlStyle( LeaveOnly );
	uiGroup* topgrp = new uiGroup( this, "top group" );
	dispmrkfld_ = new uiCheckBox( topgrp, "display markers");
	dispmrkfld_->setChecked( pms_.isvwrmarkerdisp_ );
	dispmrkfld_->activated.notify( mCB(this,uiMrkDispDlg,dispChged) );
	disphorfld_ = new uiCheckBox( topgrp, "display horizons");
	disphorfld_->setChecked( pms_.isvwrhordisp_ );
	disphorfld_->activated.notify( mCB(this,uiMrkDispDlg,dispChged) );
	disphorfld_->attach( alignedBelow, dispmrkfld_ );

	dispmrkfullnamefld_ = new uiCheckBox( topgrp, "display full name" );
	dispmrkfullnamefld_->setChecked(
	pms_.isvwrmarkerdisp_ && pms_.dispmrkfullnames_ );
	dispmrkfullnamefld_->activated.notify(mCB(this,uiMrkDispDlg,dispChged));
	dispmrkfullnamefld_->attach( rightOf, dispmrkfld_ );
	disphorfullnamefld_ = new uiCheckBox( topgrp, "display full name" );
	disphorfullnamefld_->attach( rightOf, disphorfld_ );
	disphorfullnamefld_->setChecked(
			    pms_.isvwrhordisp_ && pms_.disphorfullnames_ );
	disphorfullnamefld_->activated.notify(mCB(this,uiMrkDispDlg,dispChged));

	uiSeparator* sep = new uiSeparator( this, "Well2Seismic Sep" );
	sep->attach( stretchedBelow, topgrp );

	mrkdispfld_ = new uiWellMarkersDispProperties( this, 
		uiWellDispProperties::Setup( "Marker size", "Marker color" ), 
		pms_.mrkdisp_, pms_.allmarkernms_, true );	
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
    uiCheckBox* 	dispmrkfld_;
    uiCheckBox* 	disphorfld_;
    uiCheckBox*         dispmrkfullnamefld_;
    uiCheckBox*         disphorfullnamefld_;
    uiWellMarkersDispProperties* mrkdispfld_;
};


void uiControlView::dispHorMrks( CallBacker* )
{
    if ( !mrkrdlg_ )
    {
	mrkrdlg_ = new uiMrkDispDlg( this, server_.dispParams() );
	mrkrdlg_->redrawneeded_.notify(mCB(this,uiControlView,reDrawNeeded) );
    }
    mrkrdlg_->go();
}


void uiControlView::reDrawNeeded( CallBacker* )
{
    redrawAnnotNeeded.trigger();
}


void uiControlView::loadHorizons( CallBacker* )
{
    bool is2d = server_.is2D();
    PtrMan<CtxtIOObj> ctxt = is2d ? mMkCtxtIOObj( EMHorizon2D ) 
				  : mMkCtxtIOObj( EMHorizon3D );
    if ( !selhordlg_ )
	selhordlg_ = new uiIOObjSelDlg( this, *ctxt, "Select horizon", true );
    TypeSet<MultiID> horselids;
    if ( selhordlg_->go() )
    {
	for ( int idx=0; idx<selhordlg_->nrSel(); idx++ )
	    horselids += selhordlg_->selected( idx );
    }
    delete ctxt->ioobj; 
    BufferString errmsg; uiTaskRunner tr( this );
    server_.horizonMgr().setUpHorizons( horselids, errmsg, tr );
    if ( !errmsg.isEmpty() )
	{ mErrRet( errmsg, return ) }

    server_.dispParams().isvwrhordisp_ = true;
    redrawNeeded.trigger();
}


void uiControlView::fillPar( IOPar& iop ) const
{
    iop.set( sKeyZoom, Interval<double>( curview_.top(), curview_.bottom() ) );
}


void uiControlView::usePar( const IOPar& iop ) 
{
    Interval<double> zrg; 
    iop.get( sKeyZoom, zrg );
    curview_.setTopBottom( zrg );
    setSelView( false, false );
    redrawNeeded.trigger();
}

void uiControlView::applyProperties(CallBacker*)
{
    uiFlatViewControl::applyProperties(0);
    setSelView( true, true );
}

}; //namespace 
