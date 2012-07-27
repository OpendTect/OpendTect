/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uistratdisplay.cc,v 1.50 2012-07-27 11:05:15 cvsbert Exp $";

#include "uistratdisplay.h"

#include "uitoolbutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uimenu.h"
#include "uifont.h"
#include "uispinbox.h"
#include "uistratutildlgs.h"
#include "uistratreftree.h"
#include "uitoolbar.h"
#include "uimenuhandler.h"

#include "genericnumer.h"
#include "draw.h"
#include "randcolor.h"
#include "scaler.h"
#include "survinfo.h"

uiStratDisplay::uiStratDisplay( uiParent* p, uiStratRefTree& uitree )
    : uiGraphicsView(p,"Stratigraphy viewer")
    , drawer_(uiStratDrawer(scene(),data_))
    , uidatawriter_(uiStratDispToTree(uitree ))
    , uidatagather_(0)
    , uicontrol_(0)
    , islocked_(false)
    , maxrg_(Interval<float>(0,2e3))
{
    uidatagather_ = new uiStratTreeToDisp( data_ );
    uidatagather_->newtreeRead.notify( mCB(this,uiStratDisplay,reDraw) );

    getMouseEventHandler().buttonReleased.notify(
					mCB(this,uiStratDisplay,usrClickCB) );
    reSize.notify( mCB(this,uiStratDisplay,reDraw) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    disableScrollZoom();
    scene().setMouseEventActive( true );
    createDispParamGrp();
    setRange();
    reDraw( 0 );
}


uiStratDisplay::~uiStratDisplay()
{
    delete uidatagather_;
}


void uiStratDisplay::setTree()
{
    uidatagather_->setTree();
    setRange();
}


void uiStratDisplay::setRange()
{
    if ( data_.nrCols() && data_.nrUnits(0) )
    {
	const StratDispData::Unit& unstart = *data_.getUnit( 0, 0 );
	const StratDispData::Unit& unstop =*data_.getUnit(0,data_.nrUnits(0)-1);
	Interval<float> viewrg( unstart.zrg_.start, unstop.zrg_.stop );
	float wdth = viewrg.width(); wdth /= (float)10;
	if ( wdth <= 0 ) wdth = 10;
	viewrg.stop += wdth;
	setZRange( viewrg );
    }
    else
	setZRange( maxrg_ );
}


void uiStratDisplay::addControl( uiToolBar* tb )
{
    mDynamicCastGet(uiGraphicsView*,v,const_cast<uiStratDisplay*>(this))
    uiStratViewControl::Setup su( maxrg_ ); su.tb_ = tb;
    uicontrol_ = new uiStratViewControl( *v, su ); 
    uicontrol_->rangeChanged.notify( mCB(this,uiStratDisplay,controlRange) );
    uicontrol_->setRange( rangefld_->getFInterval() );
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
	if ( colnms.size() )
	{
	    allboxfld_ = new uiCheckBox( this, "All" );
	    allboxfld_->attach( alignedAbove, colboxflds_[0] );
	    allboxfld_->activated.notify( mCB(this,uiColViewerDlg,selAll) );
	}
    }

    void selAll( CallBacker* cb )
    {
	bool allsel = allboxfld_->isChecked();
	for ( int idx=0; idx<colboxflds_.size(); idx++ )
	    colboxflds_[idx]->setChecked( allsel );
    }

    void selChg( CallBacker* cb )
    {
	mDynamicCastGet(uiCheckBox*,box,cb)
	if ( !box ) return;

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
    uiCheckBox* 		allboxfld_;
    uiStratDrawer&		drawer_;
    StratDispData&		data_;
};


void uiStratDisplay::selCols( CallBacker* cb )
{
    uiColViewerDlg dlg( parent(), drawer_, data_ );
    dlg.go();
}


void uiStratDisplay::reDraw( CallBacker* cb )
{
    drawer_.draw();
}


void uiStratDisplay::setZRange( Interval<float> zrg )
{
    rangefld_->setValue( zrg );
    if ( uicontrol_ ) 
	uicontrol_->setRange( zrg );
    zrg.sort(false);
    drawer_.setZRange( zrg );
    drawer_.draw();
}


void uiStratDisplay::display( bool yn, bool shrk, bool maximize )
{
    uiGraphicsView::display( yn );
    dispparamgrp_->display( yn );
}


void uiStratDisplay::dispParamChgd( CallBacker* cb )
{
    Interval<float> rg = rangefld_->getFInterval();
    if ( rg.start < maxrg_.start || rg.stop > maxrg_.stop 
	    || rg.stop <= rg.start || rg.stop <= 0 ) 
	rg = maxrg_;

    setZRange( rg );
}


void uiStratDisplay::usrClickCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh || !mevh->hasEvent() || mevh->isHandled() )
	return;

    mevh->setHandled( handleUserClick(mevh->event()) );
}


bool uiStratDisplay::handleUserClick( const MouseEvent& ev )
{
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	    !ev.altStatus() && !islocked_ )
    {
	if ( getColIdxFromPos() == uidatagather_->levelColIdx() )
	{
	    const StratDispData::Level* lvl = getLevelFromPos();
	    if ( !lvl ) return false; 
	    uiMenuItem* assmnuitm = new uiMenuItem( "Assign marker boundary" );
	    uiPopupMenu menu( parent(), "Action" );
	    menu.insertItem( assmnuitm, 1 );
	    const int mnuid = menu.exec();
	    if ( mnuid<0 ) 
		return false;
	    else if ( mnuid == 1 )
		uidatawriter_.setUnitLvl( lvl->unitcode_ );
	}
	else if ( getUnitFromPos() )
	{
	    uidatawriter_.handleUnitMenu( getUnitFromPos()->fullCode() );
	}
	else if ( getParentUnitFromPos() || getColIdxFromPos() == 0 ) 
	{
	    const StratDispData::Unit* unit = getColIdxFromPos() > 0 ? 
						getParentUnitFromPos() : 0;
	    bool addunit = data_.nrUnits( 0 ) == 0;  
	    if ( !addunit )
	    {
		uiMenuItem* assmnuitm = new uiMenuItem( "Add Unit" );
		uiPopupMenu menu( parent(), "Action" );
		menu.insertItem( assmnuitm, 0 );
		const int mnuid = menu.exec();
		addunit = mnuid == 0;
	    }
	    if ( addunit )
		uidatawriter_.addUnit( unit ? unit->fullCode() : 0 );
	}
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
	if ( borders.includes( xpos, true ) ) 
	    return idx;
	borders.start = borders.stop;
    }
    return -1;
}


const StratDispData::Unit* uiStratDisplay::getUnitFromPos() const
{
    return getUnitFromPos( getColIdxFromPos() );
}


const StratDispData::Unit* uiStratDisplay::getParentUnitFromPos() const
{
    return getUnitFromPos( getColIdxFromPos()-1 );
}


const StratDispData::Unit* uiStratDisplay::getUnitFromPos( int cidx ) const
{
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


#define mEps drawer_.yAxis()->range().width()/100
const StratDispData::Level* uiStratDisplay::getLevelFromPos() const
{
    const int cidx = getColIdxFromPos();
    if ( cidx >=0 && cidx<data_.nrCols() )
    {
	Geom::Point2D<float> pos = getPos(); 
	for ( int idlvl=0; idlvl<data_.nrLevels(cidx); idlvl++ )
	{
	    const StratDispData::Level* lvl= data_.getLevel( cidx, idlvl );
	    if ( pos.y < (lvl->zpos_+mEps)  && pos.y > (lvl->zpos_-mEps) )
		return lvl;
	}
    }
    return 0;
}



uiStratDrawer::uiStratDrawer( uiGraphicsScene& sc, const StratDispData& ad )
    : data_(ad) 
    , scene_(sc)  
    , xax_(0)
    , yax_(0)
    , emptyitm_(0)	     
{
    initAxis();
}


void uiStratDrawer::initAxis()
{
    uiAxisHandler::Setup xsu( uiRect::Top );
    uiAxisHandler* xaxis = new uiAxisHandler( &scene_, xsu );
    xaxis->setBounds( Interval<float>( 0, 100 ) );
    setNewAxis( xaxis, true );
    uiBorder border = uiBorder( 20 );
    uiAxisHandler::Setup ysu( uiRect::Left );
    ysu.border( border ).nogridline( true );
    uiAxisHandler* yaxis = new uiAxisHandler( &scene_, ysu );
    setNewAxis( yaxis, false );
}


uiStratDrawer::~uiStratDrawer()
{
    eraseAll();
}


void uiStratDrawer::setNewAxis( uiAxisHandler* axis, bool isxaxis )
{
    if ( isxaxis ) 
	{ delete xax_; xax_ = axis; } 
    else
	{ delete yax_; yax_ = axis; }

    if ( xax_ && yax_ )
    {
	xax_->setBegin( yax_ );
	yax_->setBegin( xax_ );
    }
}


void uiStratDrawer::updateAxis()
{
    xax_->updateDevSize();
    yax_->updateDevSize();
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
    const int nrcols = data_.nrCols(); 

    for ( int idcol=0; idcol<nrcols; idcol++ )
    {
	if ( !data_.getCol( idcol )->isdisplayed_ ) continue;
	ColumnItem* colitm = new ColumnItem( data_.getCol( idcol )->name_ );
	colitms_ += colitm;
	colitm->pos_ = pos;
	colitm->size_ = (int)xax_->getVal( (int)(scene_.width()+10) )
	    	      /( data_.nrDisplayedCols() ) ;
	if ( colitm->size_ <0 ) 
	    colitm->size_ = 0;
	drawBorders( *colitm );
	drawLevels( *colitm );
	drawUnits( *colitm );
	pos ++;
    }

    if ( nrcols && data_.nrUnits(0) == 0 )
	drawEmptyText();
    else
	{ delete emptyitm_; emptyitm_ = 0; }
}


void uiStratDrawer::eraseAll()
{
    for ( int idx = colitms_.size()-1; idx>=0; idx-- )
    {
	ColumnItem* colitm = colitms_[idx];

	delete colitm->borderitm_; colitm->borderitm_ = 0;
	delete colitm->bordertxtitm_; colitm->bordertxtitm_ = 0;
	colitm->txtitms_.erase();
	colitm->lvlitms_.erase();
	colitm->unititms_.erase();
    }
    deepErase( colitms_ );
}


void uiStratDrawer::drawBorders( ColumnItem& colitm )
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


void uiStratDrawer::drawLevels( ColumnItem& colitm )
{
    if ( !colitm.lvlitms_.isEmpty() )
	{ colitm.lvlitms_.erase(); colitm.txtitms_.erase(); }
    const int colidx = colitms_.indexOf( &colitm );
    if ( colidx < 0 ) return;
    for ( int idx=0; idx<data_.getCol(colidx)->levels_.size(); idx++ )
    {
	const StratDispData::Level& lvl = *data_.getCol(colidx)->levels_[idx];

	int x1 = xax_->getPix( (colitm.pos_)*colitm.size_ );
	int x2 = xax_->getPix( (colitm.pos_+1)*colitm.size_ );
	int y = yax_->getPix( lvl.zpos_ );

	uiLineItem* li = scene_.addItem( new uiLineItem(x1,y,x2,y,true) );
	LineStyle::Type lst = lvl.name_.isEmpty() ? LineStyle::Dot 
	    					  : LineStyle::Solid;
	li->setPenStyle( LineStyle(lst,2,lvl.color_) );
	uiTextItem* ti = scene_.addItem( new uiTextItem( lvl.name_.buf() ) );
	ti->setPos( x1 + (x2-x1)/2, y ); 
	ti->setZValue( 2 );
	ti->setTextColor( lvl.color_ );
    
	colitm.txtitms_ += ti;
	colitm.lvlitms_ += li;
    }
}


void uiStratDrawer::drawEmptyText()
{
    delete emptyitm_; emptyitm_ =0;

    const int x = xax_->getPix( 0 );
    const int y1 = yax_->getPix( yax_->range().stop );
    const int y2 = yax_->getPix( yax_->range().start );

    uiTextItem* ti = scene_.addItem( new uiTextItem( "<Click to add>" ) );
    ti->setTextColor( Color::Black() );
    ti->setPos( x, y2 - abs((y2-y1)/2) -10 );
    ti->setZValue( 2 );
    emptyitm_ = ti;
}


void uiStratDrawer::drawUnits( ColumnItem& colitm ) 
{
    colitm.txtitms_.erase(); colitm.unititms_.erase();
    const int colidx = colitms_.indexOf( &colitm );
    if ( colidx < 0 ) return;

    const Interval<float> rg = yax_->range();

    for ( int unidx=0; unidx<data_.getCol(colidx)->units_.size(); unidx++ )
    {
	const StratDispData::Unit& unit = *data_.getCol(colidx)->units_[unidx];
	Interval<float> unitrg = unit.zrg_;
	if ( ( ( !rg.includes(unitrg.start,true) && !rg.includes(unitrg.stop,true) )
	    && ( !unitrg.includes(rg.start,true) && !unitrg.includes(rg.stop,true) ) )
		|| !unit.isdisplayed_ ) continue;
	unitrg.limitTo( rg );

	int x1 = xax_->getPix( (colitm.pos_)*colitm.size_ );
	int x2 = xax_->getPix( (colitm.pos_+1)*colitm.size_ );
	bool ztop = ( unitrg.start < rg.stop );
	bool zbase = ( unitrg.stop > rg.start );
	int y1 = yax_->getPix( ztop ? rg.stop : unitrg.start );
	int y2 = yax_->getPix( zbase ? rg.start : unitrg.stop );

	TypeSet<uiPoint> rectpts;
	rectpts += uiPoint( x1, y1 );
	rectpts += uiPoint( x2, y1 );
	rectpts += uiPoint( x2, y2 );
	rectpts += uiPoint( x1, y2 );
	rectpts += uiPoint( x1, y1 );
	uiPolygonItem* pli = scene_.addPolygon( rectpts, true );
	pli->setPenColor( Color::Black() );
	if ( unit.color_ != Color::White() )
	    pli->setFillColor( unit.color_, true );

	BufferString unm( unit.name() );
	for ( int idx=1; idx<unm.size(); idx++ )
	{
	    BufferString tmpnm = unm; tmpnm[idx] = '\0';
	    if ( FontList().get().width( tmpnm ) > ( x2-x1 ) )
		{ unm[idx-1] = '\0'; break; }
	}

	uiTextItem* ti = scene_.addItem( new uiTextItem( unm ) );
	ti->setTextColor( Color::Black() );
	ti->setPos( x1, y2 - abs((y2-y1)/2) -10 );
	ti->setZValue( 2 );
	colitm.txtitms_ += ti;
	colitm.unititms_ += pli;
    }
}




#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, fnm, tt, mCB(this,uiStratViewControl,cbnm) ); \
    tb_->addButton( but );

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
    mDefBut(zoominbut_,"zoomforward",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward",zoomCB,"Zoom out");
    mDefBut(manipdrawbut_,"altpick",stateCB,"Switch view mode")

    viewer_.getKeyboardEventHandler().keyPressed.notify(
				mCB(this,uiStratViewControl,keyPressed) );

    MouseEventHandler& meh = mouseEventHandler();
    meh.wheelMove.notify( mCB(this,uiStratViewControl,wheelMoveCB) );
    meh.buttonPressed.notify(mCB(this,uiStratViewControl,handDragStarted));
    meh.buttonReleased.notify(mCB(this,uiStratViewControl,handDragged));
    meh.movement.notify( mCB(this,uiStratViewControl,handDragging));
    viewer_.rubberBandUsed.notify( mCB(this,uiStratViewControl,rubBandCB) );

    viewer_.setDragMode( uiGraphicsViewBase::RubberBandDrag );
    viewer_.scene().setMouseEventActive( true );
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
    const MouseEventHandler& meh = mouseEventHandler();
    if ( zoomin && rg.width() > 0.001 ) 
    {
	if ( meh.hasEvent() )
	{
	    const Geom::Point2D<int> pos = meh.event().pos();
	    const uiRect& allarea = viewer_.getSceneRect();
	    Interval<float> allrg( allarea.topLeft().y, 
				   allarea.bottomRight().y );
	    LinScaler scaler( allrg.start, range_.start, 
			      allrg.stop, range_.stop );
	    const float rgpos = scaler.scale( pos.y );
	    range_.set( rgpos -margin, rgpos + margin );
	}
	else
	    range_.set( rg.start + margin, rg.stop - margin );
    }
    else 
    {
	range_.set( rg.start - margin, rg.stop + margin );
    }
    range_.limitTo( boundingrange_ );

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

    manipdrawbut_->setPixmap( manip_ ? "altview" : "altpick" );
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
    const float newpos = mouseEventHandler().event().pos().y;
    const float fac = ( startdragpos_ > newpos )? -1 : 1;
    startdragpos_ = newpos;
    Interval<float> rg( range_ );
    const float width = rg.width();
    const float shift = mHandDragFac < 0.0005 ? 0.0005 : mHandDragFac;
    const float center = rg.start + width/2 - fac*shift;
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


void uiStratViewControl::rubBandCB( CallBacker* )
{
    //Why is the navigationMouseEventHandler not used for rubberbanding ?
    if ( viewer_.getMouseEventHandler().hasEvent()
	    && viewer_.getMouseEventHandler().event().rightButton() )
	return;

    const uiRect* selarea = viewer_.getSelectedArea();
    if ( !selarea || (selarea->topLeft() == selarea->bottomRight()) 
	    || (selarea->width()<5 && selarea->height()<5) )
	return;
    const uiRect& allarea = viewer_.getSceneRect();

    Interval<float> selrg( selarea->topLeft().y, selarea->bottomRight().y );
    selrg.sort();
    const uiFont& font = FontList().get();
    const int border = 2*font.height();

    Interval<float> allrg( allarea.topLeft().y + border,
			    allarea.bottomRight().y - border );
    allrg.sort();

    LinScaler scaler( allrg.start, range_.start, allrg.stop, range_.stop );
    range_.set( scaler.scale( selrg.start ), scaler.scale( selrg.stop ) );
    range_.limitTo( boundingrange_ );
    rangeChanged.trigger();
}

