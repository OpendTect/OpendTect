/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/


static const char* rcsID = "$Id: uiwelltiecontrolview.cc,v 1.24 2009-12-08 09:03:30 cvsbruno Exp $";

#include "uiwelltiecontrolview.h"

#include "flatviewzoommgr.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "pixmap.h"
#include "welltiepickset.h"

#include "uibutton.h"
#include "uiflatviewer.h"
#include "uimsg.h"
#include "uirgbarraycanvas.h"
#include "uitoolbar.h"
#include "uiworld2ui.h"


namespace WellTie
{

#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }
#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( toolbar_, 0, ioPixmap(fnm), \
			    mCB(this,uiControlView,cbnm) ); \
    but->setToolTip( tt ); \
    toolbar_->addObject( but );

uiControlView::uiControlView( uiParent* p, uiToolBar* toolbar,uiFlatViewer* vwr)
    : uiFlatViewStdControl(*vwr, uiFlatViewStdControl::Setup()
						    .withcoltabed(false))
    , toolbar_(toolbar)
    , manip_(true)
{
    mDynamicCastGet(uiMainWin*,mw,p)
    if ( mw )
	mw->removeToolBar( tb_ );
    else
	tb_->display(false);
    toolbar_->addSeparator();
    toolbar_->addObject( vwr_.rgbCanvas().getSaveImageButton(toolbar_) );
    mDefBut(parsbut_,"2ddisppars.png",parsCB,"Set display parameters");
    mDefBut(zoominbut_,"zoomforward.png",altZoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward.png",altZoomCB,"Zoom out");
    mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode (Esc)");
    mDefBut(editbut_,"seedpickmode.png",editCB,"Pick mode (P)");
    editbut_->setToggleButton( true );

    vwr_.rgbCanvas().getKeyboardEventHandler().keyPressed.notify(
	                    mCB(this,uiControlView,keyPressCB) );
    toolbar_->addSeparator();
}


void uiControlView::finalPrepare()
{
    updatePosButtonStates();
    MouseEventHandler& mevh =
	vwr_.rgbCanvas().getNavigationMouseEventHandler();
    mevh.wheelMove.notify( mCB(this,uiControlView,wheelMoveCB) );
    mevh.buttonPressed.notify(
	mCB(this,uiControlView,handDragStarted));
    mevh.buttonReleased.notify(
	mCB(this,uiControlView,handDragged));
    mevh.movement.notify( mCB(this,uiControlView,handDragging));
}


bool uiControlView::handleUserClick()
{
    const MouseEvent& ev = mouseEventHandler(0).event();
    uiWorld2Ui w2u; vwr_.getWorld2Ui(w2u);
    const uiWorldPoint wp = w2u.transform( ev.pos() );
    if ( ev.leftButton() && !ev.ctrlStatus() && !ev.shiftStatus() 
	&& !ev.altStatus() && checkIfInside(wp.x,wp.y) && editbut_->isOn()  )
    {
	vwr_.getAuxInfo( wp, infopars_ );
	const uiWorldRect& bbox = vwr_.boundingBox();
	picksetmgr_->addPick( bbox.left(), bbox.right(), wp.x, wp.y );
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
	mErrRet("Please select your pick inside the work area");
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
    {
	editbut_->setOn( !editbut_->isOn() );
	editCB( 0 );
    }
}


void uiControlView::setSelView( bool isnewsel, bool viewall )
{
    const uiRect viewarea = isnewsel ? 
	*vwr_.rgbCanvas().getSelectedArea() : getViewRect( &vwr_ );
    if ( viewarea.topLeft() == viewarea.bottomRight() ) return;

    uiWorld2Ui w2u; vwr_.getWorld2Ui( w2u );
    uiWorldRect wr = w2u.transform( viewarea );

    const uiWorldRect bbox = vwr_.boundingBox();
    Interval<double> zrg( wr.top() , wr.bottom() );
    if ( viewall )
	zrg.set( bbox.top(), bbox.bottom() );
    wr.setTopBottom( zrg );
    Interval<double> xrg( bbox.left(), bbox.right());
    wr.setLeftRight( xrg );
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> newsz = wr.size();

    vwr_.handleChange( FlatView::Viewer::All );

    setNewView( centre, newsz );
}


const bool uiControlView::isZoomAtStart() const
{ return zoommgr_.atStart(); }


void uiControlView::setEditOn( bool yn )
{
    editbut_->setOn( yn );
    editCB( 0 );
}


/*
uiTieClipingDlg::uiTieClipingDlg( uiParent* p, SeisTrcBuffer& trcbuf )
	: uiDialog(p,"Set trace clipping")
	, trcbuf_(trcbuf)  
{
    tracetypechoicefld_ = new uiGenInput( this, "Traces : ",
			BoolInpSpec(true,"Synthetics","Seismics") );
    sliderfld_ = new uiSliderExtra( this, iSliderExtra::Setup()
					.lbl("Clipping (%)")
					.sldrsize(220)
					.withedit(true)
					.isvertical(false), "Clipping slider" );
    sliderfld_->sldr()->setInterval( Interval<float>(0,100) );
    timerangefld_ = new uiGenInput( this, "Time range",
				    FloatInpIntervalSpec()
				    .setName(BufferString(" range start"),0)
				    .setName(BufferString(" range stop"),1) ); 
}


void uiTieClipingDlg::getFromScreen( Callbacker* )
{
    cd_.issynthetic_ = tracetypechoicefld_->getBoolValue()
    cd_.timerange_ = timerangefld_->getValues();
    cd_.cliprate_ = sliderfld_->sldr()->value(); 
}


void uiTieClipingDlg::putToScreen( Callbacker* )
{
    tracetypechoicefld_->setValue( cd_.issynthetic_ );
    timerange_ = timerangefld_->setValues( cd_.timerange_ );
    sliderfld_->sldr()->value() = cd_.cliprate_; 
}
*/

}; //namespace 
