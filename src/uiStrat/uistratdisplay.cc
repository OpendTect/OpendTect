/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdisplay.cc,v 1.22 2010-09-27 14:01:44 cvsbruno Exp $";

#include "uistratdisplay.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uidialog.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uispinbox.h"
#include "uistratutildlgs.h"
#include "uistratreftree.h"
#include "uitoolbar.h"
#include "uimenuhandler.h"

#include "genericnumer.h"
#include "draw.h"
#include "pixmap.h"
#include "randcolor.h"
#include "survinfo.h"

uiStratDisplay::uiStratDisplay( uiParent* p, uiStratRefTree& uitree )
    : uiGraphicsView(p,"Stratigraphy viewer")
    , drawer_(uiStratDrawer(scene(),data_))
    , uidatawriter_(uiStratDispToTreeTransl(uitree ))
    , uidatagather_(0)
    , assignlvlmnuitem_("&Specify marker boundary")
    , uicontrol_(0)
    , maxrg_(Interval<float>(0,2e3))
{
    uidatagather_ = new uiStratTreeToDispTransl( data_ );
    uidatagather_->newtreeRead.notify( mCB(this,uiStratDisplay,dataChanged) );

    getMouseEventHandler().buttonReleased.notify(
					mCB(this,uiStratDisplay,usrClickCB) );
    reSize.notify( mCB(this,uiStratDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );


    disableScrollZoom();
    scene().setMouseEventActive( true );
    createDispParamGrp();
    dataChanged( 0 );
}


uiStratDisplay::~uiStratDisplay()
{
    delete uidatagather_;
}


void uiStratDisplay::reSized( CallBacker* )
{
    drawer_.draw();
}


void uiStratDisplay::addControl( uiToolBar* tb )
{
    mDynamicCastGet(uiGraphicsView*,v,const_cast<uiStratDisplay*>(this))
    uiStratViewControl::Setup su( maxrg_ ); su.tb_ = tb;
    uicontrol_ = new uiStratViewControl( *v, su ); 
    uicontrol_->rangeChanged.notify( mCB(this,uiStratDisplay,controlRange) );
    resetRangeFromUnits();
}


void uiStratDisplay::controlRange( CallBacker* )
{
    if ( uicontrol_ )
    {
	rangefld_->setValue( uicontrol_->range() );
	dispParamChgd(0);
    }
}


void uiStratDisplay::createDispParamGrp()
{
    dispparamgrp_ = new uiGroup( parent(), "display Params Group" );
    dispparamgrp_->attach( centeredBelow, this );
    rangefld_ = new uiGenInput( dispparamgrp_, "Display between Ages (My)",
		FloatInpIntervalSpec()
		    .setName(BufferString("range start"),0)
		    .setName(BufferString("range stop"),1) );
    rangefld_->valuechanged.notify( mCB(this,uiStratDisplay,dispParamChgd ) );
    if ( data_.nrCols() && data_.nrUnits(0) > 1)
    {
	const StratDispData::Unit& topunit = *data_.getUnit(0,0);
	const StratDispData::Unit& botunit = 
	    				*data_.getUnit(0,data_.nrUnits(0)-1);
	Interval<float> zrg( topunit.zrg_.start, botunit.zrg_.stop );
	rangefld_->setValue( zrg );
	setZRange( Interval<float>( zrg.stop, zrg.start ) );
    }
   
    const CallBack cbv = mCB( this, uiStratDisplay, selCols );
    viewcolbutton_ = new uiPushButton( dispparamgrp_,"&View ",cbv,true ); 
    viewcolbutton_->attach( rightOf, rangefld_ );
}


class uiColViewerDlg : public uiDialog
{
public :
    uiColViewerDlg( uiParent* p, uiStratDrawer& drawer, StratDispData& ad )
	: uiDialog(p,uiDialog::Setup("View Columns","",mNoHelpID))
	, drawer_(drawer) 
	, data_(ad)			  
    {
	setCtrlStyle( LeaveOnly );

	BufferStringSet colnms;
	for ( int idx=0; idx<data_.nrCols(); idx++ )
	    colnms.add( data_.getCol( idx )->name_ );

	for ( int idx=0; idx<colnms.size(); idx++ )
	{
	    uiCheckBox* box = new uiCheckBox( this, colnms.get(idx) );
	    box->setChecked( data_.getCol( idx )->isdisplayed_ );
	    box->activated.notify( mCB(this,uiColViewerDlg,selChg) );
	    colboxflds_ += box;
	    if ( idx ) box->attach( alignedBelow, colboxflds_[idx-1] ); 
	}
    }

    void selChg( CallBacker* cb )
    {
	mDynamicCastGet(uiCheckBox*,box,cb)
	if ( !cb ) return;

	int idsel = colboxflds_.indexOf( box );
	for ( int idbox=0; idbox<colboxflds_.size(); idbox++ )
	{
	    NotifyStopper ns( colboxflds_[idbox]->activated );
	    bool ison = false;
	    ison = colboxflds_[idbox]->isChecked();
	    data_.getCol( idbox )->isdisplayed_ = ison;
	}
	drawer_.draw();
    }

protected:

    ObjectSet<uiCheckBox> 	colboxflds_;
    uiStratDrawer&		drawer_;
    StratDispData&		data_;
};


void uiStratDisplay::selCols( CallBacker* cb )
{
    uiColViewerDlg dlg( parent(), drawer_, data_ );
    dlg.go();
}


void uiStratDisplay::dataChanged( CallBacker* cb )
{
    drawer_.draw();
}


void uiStratDisplay::resetRangeFromUnits()
{
    if ( data_.nrCols()<=0 )
	return;
    const StratDispData::Column& col = *data_.getCol(0);
    if ( col.units_.size() == 0 )
	return;

    Interval<float> rg;
    float start = col.units_[0]->zrg_.start;
    float stop = col.units_[col.units_.size()-1]->zrg_.stop;
    rg.set( start, stop );
    rangefld_->setValue( rg );
    setZRange( rg );
}


void uiStratDisplay::setZRange( Interval<float> zrg )
{
    zrg.sort(false);
    drawer_.setZRange( zrg );
}


void uiStratDisplay::display( bool yn, bool shrk, bool maximize )
{
    uiGraphicsView::display( yn );
    dispparamgrp_->display( yn );
}


void uiStratDisplay::dispParamChgd( CallBacker* cb )
{
    Interval<float> rg = rangefld_->getFInterval();
    rg.start = (int)rg.start; rg.stop = (int)rg.stop;
    if ( rg.start < maxrg_.start || rg.stop > maxrg_.stop 
	    || rg.stop <= rg.start || rg.stop <= 0 ) 
	rg = maxrg_;

    rangefld_->setValue( rg );
    if ( uicontrol_ ) 
	uicontrol_->setRange( rg );
    setZRange( rg );
}


void uiStratDisplay::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;
    mAddMenuItem( menu, &assignlvlmnuitem_, true, false);
}


void uiStratDisplay::handleMenuCB( CallBacker* cb )
{
    const StratDispData::Unit* unit = getUnitFromPos();
    if ( unit ) 
	uidatawriter_.handleUnitMenu( unit->name_ );
}


bool uiStratDisplay::isUnitBelowCurrent() const
{
    /*
    const StratDispData::Unit* curunit = getUnitFromPos();
    if ( !curunit ) return false;
    Interval<float> rg( curunit->zpos_, curunit->zposbot_ );
    int cidx = getColIdxFromPos( );
    if ( cidx > 0 || (cidx+1) < nrCols() ) 
    {
	for ( int idunit=0; idunit<nrUnits(cidx+1); idunit++ )
	{
	    const StratDispData::Unit* unit = getUnit( idunit, cidx+1 );
	    if ( unit->zpos_ >= rg.start && unit->zposbot_ <= rg.stop )
		return false;
	}
    }
    */
    return true;
}



void uiStratDisplay::usrClickCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh )
	return;
    if ( !mevh->hasEvent() )
	return;
    if ( mevh->isHandled() )
	return;

    mevh->setHandled( handleUserClick(mevh->event()) );
}


bool uiStratDisplay::handleUserClick( const MouseEvent& ev )
{
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	    !ev.altStatus() )
    {
	if ( getUnitFromPos() ) 
	    handleMenuCB( 0 );
	return true;
    }
    return false;
}


Geom::Point2D<float> uiStratDisplay::getPos() const
{
    uiStratDisplay* self = const_cast<uiStratDisplay*>( this );
    const float xpos = drawer_.xAxis()->getVal( 
			self->getMouseEventHandler().event().pos().x ); 
    const float ypos = drawer_.yAxis()->getVal( 
			self->getMouseEventHandler().event().pos().y ); 
    return Geom::Point2D<float>( xpos, ypos );
}


int uiStratDisplay::getColIdxFromPos() const 
{
    float xpos = getPos().x;
    Interval<int> borders(0,0);
    for ( int idx=0; idx<data_.nrCols(); idx++ )
    {
	borders.stop += drawer_.colItem(idx).size_;
	if ( borders.includes( xpos ) ) 
	return idx;
	borders.start = borders.stop;
    }
    return -1;
}


const StratDispData::Unit* uiStratDisplay::getUnitFromPos( bool nocolidx ) const
{
    int cidx = nocolidx ? 0 : getColIdxFromPos();
    if ( cidx >=0 && cidx<data_.nrCols() )
    {
	Geom::Point2D<float> pos = getPos(); 
	for ( int idunit=0; idunit<data_.nrUnits(cidx); idunit++ )
	{
	    const StratDispData::Unit* unit = data_.getUnit( cidx, idunit );
	    if ( pos.y < unit->zrg_.stop && pos.y >= unit->zrg_.start )
		return unit;
	}
    }
    return 0;
}



uiStratDrawer::uiStratDrawer( uiGraphicsScene& sc, const StratDispData& ad )
    : data_(ad) 
    , scene_(sc)  
    , xax_(new uiAxisHandler(&scene_,uiAxisHandler::Setup(uiRect::Top)))
    , yax_(new uiAxisHandler(&scene_,uiAxisHandler::Setup(uiRect::Left)
							    .nogridline(true)))
{
    xax_->setBounds( Interval<float>( 0, 100 ) );
}


uiStratDrawer::~uiStratDrawer()
{
    eraseAll();
}


void uiStratDrawer::setNewAxis( uiAxisHandler* axis, bool isxaxis )
{
    if ( isxaxis ) 
	xax_ = axis; 
    else
	yax_ = axis;
}


void uiStratDrawer::updateAxis()
{
    xax_->setNewDevSize( (int)scene_.width()+10, (int)scene_.height()+10 );
    yax_->setNewDevSize( (int)scene_.height()+10, (int)scene_.width()+10 );
    xax_->setBegin( yax_ );     yax_->setBegin( xax_ );
    xax_->setEnd( yax_ );       yax_->setEnd( xax_ );
    yax_->plotAxis();
}


void uiStratDrawer::draw()
{
    eraseAll();
    updateAxis();
    drawColumns();
}


void uiStratDrawer::drawColumns()
{
    eraseAll();
    int pos = 0;
    for ( int idcol=0; idcol<data_.nrCols(); idcol++ )
    {
	if ( !data_.getCol( idcol )->isdisplayed_ ) continue;
	ColumnItem* colitm = new ColumnItem( data_.getCol( idcol )->name_ );
	colitms_ += colitm;
	colitm->pos_ = pos;
	colitm->size_ = (int)xax_->getVal( (int)(scene_.width()+10) )
	    	      /( data_.nrDisplayedCols() ) ;
	if ( colitm->size_ <0 ) 
	    colitm->size_ = 0;
	drawUnits( *colitm, idcol );
	drawBorders( *colitm, idcol );
	drawMarkers( *colitm, idcol );
	pos ++;
    }
}


#define mRemoveSet( itms ) \
    for ( int idx=0; idx<itms.size(); idx++ ) \
    scene_.removeItem( itms[idx] ); \
    deepErase( itms );
void uiStratDrawer::eraseAll()
{
    for ( int idx = colitms_.size()-1; idx>=0; idx-- )
    {
	ColumnItem* colitm = colitms_[idx];

	delete scene_.removeItem( colitm->borderitm_ ); 
	delete scene_.removeItem( colitm->bordertxtitm_ ); 
	mRemoveSet( colitm->mrktxtitms_ )
	mRemoveSet( colitm->unittxtitms_ )
	mRemoveSet( colitm->mrkitms_ )
	mRemoveSet( colitm->unititms_ )
    }
    deepErase( colitms_ );
}


void uiStratDrawer::drawBorders( ColumnItem& colitm, int colidx )
{
    int x1 = xax_->getPix( (colitm.pos_)*colitm.size_ );
    int x2 = xax_->getPix( (colitm.pos_+1)*colitm.size_ );
    int y1 = yax_->getPix( yax_->range().stop );
    int y2 = yax_->getPix( yax_->range().start );
	
    TypeSet<uiPoint> rectpts;
    rectpts += uiPoint( x1, y1 );
    rectpts += uiPoint( x2, y1  );
    rectpts += uiPoint( x2, y2  );
    rectpts += uiPoint( x1, y2  );
    rectpts += uiPoint( x1, y1  );
    uiPolyLineItem* pli = scene_.addItem( new uiPolyLineItem( rectpts ) );
    pli->setPenStyle( LineStyle(LineStyle::Solid,1,Color::Black()) );
    colitm.borderitm_ = pli;

    uiTextItem* ti = scene_.addItem( new uiTextItem( colitm.name_ ) );
    ti->setTextColor( Color::Black() );
    ti->setPos( x1, y1 - 18 );
    ti->setZValue( 2 );
    colitm.bordertxtitm_ = ti;
}


void uiStratDrawer::drawMarkers( ColumnItem& colitm, int colidx )
{
    /*
    mRemoveSet( colitm.mrkitms_ );
    mRemoveSet( colitm.mrktxtitms_ );
    for ( int idx=0; idx<data_.getCol(colidx)->markers_.size(); idx++ )
    {
	const StratDispData::Marker& mrk = *data_.getCol(colidx)->markers_[idx];

	int x1 = xax_->getPix( (colitm.pos_)*colitm.size_ );
	int x2 = xax_->getPix( (colitm.pos_+1)*colitm.size_ );
	int y = yax_->getPix( mrk.zpos_ );

	uiLineItem* li = scene_.addItem( new uiLineItem(x1,y,x2,y,true) );
	LineStyle::Type lst = mrk.isdotted_ ? LineStyle::Dot : LineStyle::Solid;
	li->setPenStyle( LineStyle(lst,2,mrk.col_) );
	uiTextItem* ti = scene_.addItem( new uiTextItem( mrk.name_.buf() ) );
	ti->setPos( x1 + (x2-x1)/2, y ); 
	ti->setZValue( 2 );
	ti->setTextColor( mrk.col_ );
	colitm.mrktxtitms_ += ti;
	colitm.mrkitms_ += li;
    }
    */
}


void uiStratDrawer::drawUnits( ColumnItem& colitm, int colidx ) 
{
    mRemoveSet( colitm.unittxtitms_ );
    mRemoveSet( colitm.unititms_ );

    const Interval<float> rg = yax_->range();
    for ( int idx=0; idx<data_.getCol(colidx)->units_.size(); idx++ )
    {
	const StratDispData::Unit& unit = *data_.getCol(colidx)->units_[idx];
	Interval<float> unitrg = unit.zrg_;
	if ( ( unit.zrg_.start > rg.start && unit.zrg_.stop > rg.start ) ||
		 ( unit.zrg_.start < rg.stop && unit.zrg_.stop < rg.stop ) ||
			!unit.isdisplayed_ ) continue;

	int x1 = xax_->getPix( (colitm.pos_)*colitm.size_ );
	int x2 = xax_->getPix( (colitm.pos_+1)*colitm.size_ );
	bool ztop = ( unit.zrg_.start < rg.stop );
	bool zbase = ( unit.zrg_.stop > rg.start );
	int y1 = yax_->getPix( ztop ? rg.stop : unit.zrg_.start );
	int y2 = yax_->getPix( zbase ? rg.start : unit.zrg_.stop );

	TypeSet<uiPoint> rectpts;
	rectpts += uiPoint( x1, y1 );
	rectpts += uiPoint( x2, y1 );
	rectpts += uiPoint( x2, y2 );
	rectpts += uiPoint( x1, y2 );
	rectpts += uiPoint( x1, y1 );
	uiPolygonItem* pli = scene_.addPolygon( rectpts, true );
	pli->setPenColor( Color::Black() );
	if ( unit.color_ != Color::White() )
	    pli->setFillColor( unit.color_ );
	uiTextItem* ti = scene_.addItem( new uiTextItem( unit.name_.buf() ) );
	ti->setTextColor( Color::Black() );
	ti->setPos( x1, y2 - abs((y2-y1)/2) -10 );
	ti->setZValue( 2 );
	colitm.unittxtitms_ += ti;
	colitm.unititms_ += pli;
    }
}




#define mDefBut(but,fnm,cbnm,tt) \
        but = new uiToolButton( tb_, 0, ioPixmap(fnm), \
		mCB(this,uiStratViewControl,cbnm) ); \
    but->setToolTip( tt ); \
    tb_->addObject( but );

uiStratViewControl::uiStratViewControl( uiGraphicsView& v, Setup& su )
    : viewer_(v)
    , zoomfac_(1)
    , manip_(false)		 
    , rangeChanged(this)
    , tb_(su.tb_)
    , boundingrange_(su.maxrg_)	 
{
    if ( tb_ ) 
	tb_->addSeparator();
    else
    {	
	tb_ = new uiToolBar( v.parent(), "Viewer toolbar", uiToolBar::Top );
	mDynamicCastGet(uiMainWin*,mw,v.parent())
	if ( mw )
	    mw->addToolBar( tb_ );
    }
    mDefBut(zoominbut_,"zoomforward.png",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward.png",zoomCB,"Zoom out");
    mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode")

    viewer_.getKeyboardEventHandler().keyPressed.notify(
				mCB(this,uiStratViewControl,keyPressed) );

    MouseEventHandler& meh = mouseEventHandler();
    meh.wheelMove.notify( mCB(this,uiStratViewControl,wheelMoveCB) );
    meh.buttonPressed.notify(mCB(this,uiStratViewControl,handDragStarted));
    meh.buttonReleased.notify(mCB(this,uiStratViewControl,handDragged));
    meh.movement.notify( mCB(this,uiStratViewControl,handDragging));
}


void uiStratViewControl::setSensitive( bool yn )
{
    manipdrawbut_->setSensitive( yn );
    zoominbut_->setSensitive( yn );
    zoomoutbut_->setSensitive( yn );
}


void uiStratViewControl::zoomCB( CallBacker* but )
{
    const bool zoomin = but == zoominbut_;
    const Interval<float> rg( range_ );
    const float margin = rg.width()/4;
    if ( zoomin && rg.width() > 2)
    {
	range_.set( rg.start + margin, rg.stop - margin );
    }
    else 
    {
	range_.set( rg.start - margin, rg.stop + margin );
    }
    if ( range_.start < boundingrange_.start 
	    			|| range_.stop > boundingrange_.stop)
	range_ = boundingrange_;

    rangeChanged.trigger();
}


MouseEventHandler& uiStratViewControl::mouseEventHandler()
{
    return viewer_.getNavigationMouseEventHandler();
}


void uiStratViewControl::wheelMoveCB( CallBacker* )
{
    const MouseEventHandler& mvh = mouseEventHandler();
    const MouseEvent& ev = mvh.event();
    if ( mIsZero(ev.angle(),0.01) )
	return;
    zoomCB( ev.angle() < 0 ? zoominbut_ : zoomoutbut_ );
}


void uiStratViewControl::stateCB( CallBacker* )
{
    if ( !manipdrawbut_ ) return;
    manip_ = !manip_;

    manipdrawbut_->setPixmap( manip_ ? "altview.png" : "altpick.png" );
    viewer_.setDragMode( !manip_ ? uiGraphicsViewBase::RubberBandDrag
			         : uiGraphicsViewBase::ScrollHandDrag);
    viewer_.scene().setMouseEventActive( true );
}


void uiStratViewControl::keyPressed( CallBacker* )
{
    const KeyboardEvent& ev = viewer_.getKeyboardEventHandler().event();
    if ( ev.key_ == OD::Escape )
	stateCB( 0 );
}


void uiStratViewControl::handDragStarted( CallBacker* )
{
    if ( mouseEventHandler().event().rightButton() )
	return;
    mousepressed_ = true;
    startdragpos_ = mouseEventHandler().event().pos().y;
}


#define mHandDragFac width/80
void uiStratViewControl::handDragging( CallBacker* )
{
    if ( viewer_.dragMode() != uiGraphicsViewBase::ScrollHandDrag 
	|| !mousepressed_ || !manip_ ) return;
    viewdragged_ = true;
    stopdragpos_ = mouseEventHandler().event().pos().y;
    bool goingup = ( startdragpos_ > stopdragpos_ );
    float fac = goingup? -1 : 1;
    Interval<float> rg( range_ );
    float width = rg.width();
    float center = rg.start + width/2 - fac*mHandDragFac;
    rg.set( center - width/2, center + width/2 );
    if ( rg.start < boundingrange_.start )
	rg.set( boundingrange_.start, range_.stop ); 
    if ( rg.stop > boundingrange_.stop )
	rg.set( range_.start, boundingrange_.stop );

    range_ = rg;
    rangeChanged.trigger();
}


void uiStratViewControl::handDragged( CallBacker* )
{
    mousepressed_ = false;
    if ( viewer_.dragMode() != uiGraphicsViewBase::ScrollHandDrag )
	return;
    viewdragged_ = false;
}
