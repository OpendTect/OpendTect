/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdisplay.cc,v 1.20 2010-09-17 15:45:35 cvsbruno Exp $";

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

uiStratDisplay::uiStratDisplay( uiParent* p, uiStratRefTree& tree )
    : uiAnnotDisplay(p,"Stratigraphy viewer")
    , uidatagather_(uiStratTreeToDispTransl(data_))
    , uidatawriter_(uiStratDispToTreeTransl(tree ))
    , assignlvlmnuitem_("&Specify marker boundary")
    , uicontrol_(0)
    , maxrg_(Interval<float>(0,4.5e3))
{
    disableScrollZoom();
    scene().setMouseEventActive( true );
    uidatagather_.newtreeRead.notify( mCB(this,uiStratDisplay,dataChanged) );
    createDispParamGrp();
    dataChanged( 0 );
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
    if ( nrCols() && nrUnits(0) )
    {
	const AnnotData::Unit& topunit = *getUnit(0,0);
	const AnnotData::Unit& botunit = *getUnit(nrUnits(0)-1,0);
	Interval<float> zrg;
	zrg.start = topunit.zpos_; 
	zrg.stop = botunit.zposbot_; 
	rangefld_->setValue( zrg );
	setZRange( Interval<float>( zrg.stop, zrg.start ) );
    }
   
    const CallBack cbv = mCB( this, uiStratDisplay, selCols );
    viewcolbutton_ = new uiPushButton( dispparamgrp_,"&View ",cbv,true ); 
    viewcolbutton_->attach( rightOf, rangefld_ );
    const CallBack cbf = mCB(&uidatawriter_,uiStratDispToTreeTransl,fillUndef);
    fillbutton_ = new uiPushButton( dispparamgrp_,"&Fill undefined",cbf,true ); 
    fillbutton_->attach( rightOf, viewcolbutton_ );
}


class uiColViewerDlg : public uiDialog
{
public :
    uiColViewerDlg( uiParent* p, uiAnnotDrawer& drawer, AnnotData& ad )
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
	    /*
	    if ( idsel<idbox )
		ison = true;
	    else if ( idsel == idbox ) 
	    */
	    ison = colboxflds_[idbox]->isChecked();

	    //colboxflds_[idbox]->setChecked( ison );
	    data_.getCol( idbox )->isdisplayed_ = ison;
	}
	drawer_.draw();
    }

protected:

    ObjectSet<uiCheckBox> 	colboxflds_;
    uiAnnotDrawer&		drawer_;
    AnnotData&			data_;
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
    const AnnotData::Column& col = *data_.getCol(0);
    if ( col.units_.size() == 0 )
	return;

    Interval<float> rg;
    float start = col.units_[0]->zpos_;
    float stop = col.units_[col.units_.size()-1]->zposbot_;
    rg.set( start, stop );
    rangefld_->setValue( rg );
    setZRange( rg );
}


void uiStratDisplay::setZRange( Interval<float> zrg )
{
    zrg.sort(false);
    uiAnnotDisplay::setZRange( zrg );
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
    const AnnotData::Unit* unit = getUnitFromPos();
    if ( unit ) 
	uidatawriter_.handleUnitMenu( unit->name_ );
    const AnnotData::Marker* mrk = getMrkFromPos();
    if ( mrk )
	uidatawriter_.handleUnitLvlMenu( mrk->id_ );
}


bool uiStratDisplay::isUnitBelowCurrent() const
{
    const AnnotData::Unit* curunit = getUnitFromPos();
    if ( !curunit ) return false;
    Interval<float> rg( curunit->zpos_, curunit->zposbot_ );
    int cidx = getColIdxFromPos( );
    if ( cidx > 0 || (cidx+1) < nrCols() ) 
    {
	for ( int idunit=0; idunit<nrUnits(cidx+1); idunit++ )
	{
	    const AnnotData::Unit* unit = getUnit( idunit, cidx+1 );
	    if ( unit->zpos_ >= rg.start && unit->zposbot_ <= rg.stop )
		return false;
	}
    }
    return true;
}





uiAnnotDrawer::uiAnnotDrawer( uiGraphicsScene& sc, const AnnotData& ad )
    : data_(ad) 
    , scene_(sc)  
    , xax_(new uiAxisHandler(&scene_,uiAxisHandler::Setup(uiRect::Top)))
    , yax_(new uiAxisHandler(&scene_,uiAxisHandler::Setup(uiRect::Left)
							.nogridline(true)))
{
    xax_->setBounds( Interval<float>( 0, 100 ) );
}


uiAnnotDrawer::~uiAnnotDrawer()
{
    eraseAll();
}


void uiAnnotDrawer::setNewAxis( uiAxisHandler* axis, bool isxaxis )
{
    if ( isxaxis ) 
	xax_ = axis; 
    else
	yax_ = axis;
}


void uiAnnotDrawer::updateAxis()
{
    xax_->setNewDevSize( (int)scene_.width()+10, (int)scene_.height()+10 );
    yax_->setNewDevSize( (int)scene_.height()+10, (int)scene_.width()+10 );
    xax_->setBegin( yax_ );     yax_->setBegin( xax_ );
    xax_->setEnd( yax_ );       yax_->setEnd( xax_ );
    yax_->plotAxis();
}



void uiAnnotDrawer::draw()
{
    eraseAll();
    updateAxis();
    drawColumns();
}


void uiAnnotDrawer::drawColumns()
{
    eraseAll();
    int pos = 0;
    for ( int idcol=0; idcol<nrCols(); idcol++ )
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
void uiAnnotDrawer::eraseAll()
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


void uiAnnotDrawer::drawBorders( ColumnItem& colitm, int colidx )
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


void uiAnnotDrawer::drawMarkers( ColumnItem& colitm, int colidx )
{
    mRemoveSet( colitm.mrkitms_ );
    mRemoveSet( colitm.mrktxtitms_ );
    for ( int idx=0; idx<data_.getCol(colidx)->markers_.size(); idx++ )
    {
	const AnnotData::Marker& mrk = *data_.getCol(colidx)->markers_[idx];

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
}


void uiAnnotDrawer::drawUnits( ColumnItem& colitm, int colidx ) 
{
    mRemoveSet( colitm.unittxtitms_ );
    mRemoveSet( colitm.unititms_ );

    Interval<float> rg =  yax_->range();
    for ( int idx=0; idx<data_.getCol(colidx)->units_.size(); idx++ )
    {
	const AnnotData::Unit& unit = *data_.getCol(colidx)->units_[idx];
	if ( ( unit.zpos_ >rg.start && unit.zposbot_ > rg.start ) ||
	     ( unit.zpos_ < rg.stop && unit.zposbot_ < rg.stop ) ||
		!unit.draw_ ) continue;

	int x1 = xax_->getPix( (colitm.pos_)*colitm.size_ );
	int x2 = xax_->getPix( (colitm.pos_+1)*colitm.size_ );
	bool ztop = ( unit.zpos_ < rg.stop );
	bool zbase = ( unit.zposbot_ > rg.start );
	int y1 = yax_->getPix( ztop ? rg.stop : unit.zpos_ );
	int y2 = yax_->getPix( zbase ? rg.start : unit.zposbot_ );

	TypeSet<uiPoint> rectpts;
	rectpts += uiPoint( x1, y1 );
	rectpts += uiPoint( x2, y1 );
	rectpts += uiPoint( x2, y2 );
	rectpts += uiPoint( x1, y2 );
	rectpts += uiPoint( x1, y1 );
	uiPolygonItem* pli = scene_.addPolygon( rectpts, true );
	pli->setPenColor( Color::Black() );
	if ( unit.col_ != Color::White() )
	    pli->setFillColor( unit.col_ );
	uiTextItem* ti = scene_.addItem( new uiTextItem( unit.name_ ) );
	ti->setTextColor( Color::Black() );
	ti->setPos( x1, y2 - abs((y2-y1)/2) -10 );
	ti->setZValue( 2 );
	colitm.unittxtitms_ += ti;
	colitm.unititms_ += pli;
    }
}




uiAnnotDisplay::uiAnnotDisplay( uiParent* p, const char* nm )
    : uiGraphicsView(p,nm)
    , data_(AnnotData()) 
    , menu_(*new uiMenuHandler(p,-1))
    , addunitmnuitem_("Add Unit...",0)
    , remunitmnuitem_("Remove unit...",1)
    , addcolmnuitem_("Add Column...",2)
    , drawer_(uiAnnotDrawer(scene(),data_))
{
    getMouseEventHandler().buttonReleased.notify(
					mCB(this,uiAnnotDisplay,usrClickCB) );
    reSize.notify( mCB(this,uiAnnotDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiAnnotDisplay,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiAnnotDisplay,handleMenuCB));
    
    finaliseDone.notify( mCB(this,uiAnnotDisplay,init) );
}


uiAnnotDisplay::~uiAnnotDisplay()
{
    menu_.unRef();
}


void uiAnnotDisplay::init( CallBacker* )
{
    drawer_.draw();
    show();
}


void uiAnnotDisplay::reSized( CallBacker* )
{
    drawer_.draw();
}


void uiAnnotDisplay::usrClickCB( CallBacker* cb )
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


bool uiAnnotDisplay::handleUserClick( const MouseEvent& ev )
{
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	!ev.altStatus() )
    {
	if ( getUnitFromPos() ) 
	    handleMenuCB( 0 );
	else if ( getMrkFromPos() )
	    menu_.executeMenu(0);
	return true;
    }
    return false;
}


void uiAnnotDisplay::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;

    int cidx = getColIdxFromPos();
    AnnotData::Column* col = cidx >=0 ? data_.getCol( cidx ) : 0;
    if ( !col ) return;
    bool unitfound = getUnitFromPos();
    mAddMenuItem( menu, &addunitmnuitem_, !unitfound && col->iseditable_,false);
    mAddMenuItem( menu, &remunitmnuitem_, unitfound && col->iseditable_, false);
}


uiAnnotDisplay::uiCreateColDlg::uiCreateColDlg( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Specify column parameters","",mNoHelpID))
{
    namefld_ = new uiGenInput( this, "Title", StringInpSpec("Title") );
}

bool uiAnnotDisplay::uiCreateColDlg::acceptOK( CallBacker* )
{ colname_ = namefld_->text(); return true; }
		
		    
class uiCreateAnnotDlg : public uiDialog
{
public :
    uiCreateAnnotDlg( uiParent* p, float zpos )
	: uiDialog(p,uiDialog::Setup("Add Annotation",
				    "Specify properties",mNoHelpID))
    {
	namefld_ = new uiGenInput( this, "Name", StringInpSpec("Annot") );
	topdepthfld_ = new uiGenInput( this, "Top value", FloatInpSpec(zpos) );
	topdepthfld_->attach( alignedBelow, namefld_ );
	botdepthfld_ = new uiGenInput(this,"Bottom Value",FloatInpSpec(zpos+100));
	botdepthfld_->attach( alignedBelow, topdepthfld_ );
	uiColorInput::Setup csu( getRandStdDrawColor() );
	csu.lbltxt( "Color" ).withalpha(false);
	colorfld_ = new uiColorInput( this, csu, "Color" );
	colorfld_->attach( alignedBelow, botdepthfld_ );
    }

    bool acceptOK( CallBacker* )
    {
	const char* nm = namefld_->text();
	float topdpt = topdepthfld_->getfValue();
	float botdpt = botdepthfld_->getfValue();
	unit_ = new AnnotData::Unit( nm, topdpt, botdpt );
	unit_->col_ =  colorfld_->color();
	return true;
    }

    AnnotData::Unit* unit() { return unit_; }

protected :

    uiGenInput*         namefld_;
    uiGenInput*         topdepthfld_;
    uiGenInput*         botdepthfld_;
    uiColorInput*       colorfld_;
    AnnotData::Unit* 	unit_;
};


void uiAnnotDisplay::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
    return;

    bool ishandled = true;
    if ( mnuid == addunitmnuitem_.id )
    {
	uiCreateAnnotDlg dlg( parent(), getPos().y );
	if ( dlg.go() )
	{
	    AnnotData::Unit* unit = dlg.unit();
	    if ( unit )
	    {
		int cidx = getColIdxFromPos();
		AnnotData::Column* col = cidx >=0 ? data_.getCol( cidx ) : 0;
		if ( col )
		    col->units_ += unit;
	    }
	}
    }
    if ( mnuid == addcolmnuitem_.id )
    {
	uiCreateColDlg dlg( parent() );
	if ( dlg.go() )
	    data_.addCol( new AnnotData::Column( dlg.colname_ ) );
    }
    if ( mnuid == remunitmnuitem_.id )
    {
	//delete getColFromPos( xpos );
    }
    drawer_.draw();
}


Geom::Point2D<float> uiAnnotDisplay::getPos() const
{
    uiAnnotDisplay* self = const_cast<uiAnnotDisplay*>( this );
    const float xpos = drawer_.xAxis()->getVal( 
	    		self->getMouseEventHandler().event().pos().x ); 
    const float ypos = drawer_.yAxis()->getVal( 
	    		self->getMouseEventHandler().event().pos().y ); 
    return Geom::Point2D<float>( xpos, ypos );
}


int uiAnnotDisplay::getColIdxFromPos() const 
{
    float xpos = getPos().x;
    Interval<int> borders(0,0);
    for ( int idx=0; idx<nrCols(); idx++ )
    {
	borders.stop += drawer_.colItem(idx).size_;
	if ( borders.includes( xpos ) ) 
	    return idx;
	borders.start = borders.stop;
    }
    return -1;
}


const AnnotData::Unit* uiAnnotDisplay::getUnitFromPos( bool nocolidx ) const
{
    int cidx = nocolidx ? 0 : getColIdxFromPos();
    if ( cidx >=0 && cidx<nrCols() )
    {
	Geom::Point2D<float> pos = getPos(); 
	for ( int idunit=0; idunit<nrUnits(cidx); idunit++ )
	{
	    const AnnotData::Unit* unit = getUnit( idunit, cidx );
	    if ( pos.y < unit->zposbot_ && pos.y >= unit->zpos_ )
		return unit;
	}
    }
    return 0;
}


#define mEps drawer_.yAxis()->range().width()/100
const AnnotData::Marker* uiAnnotDisplay::getMrkFromPos() const
{
    int cidx = getColIdxFromPos();
    if ( cidx >=0 && cidx<nrCols() )
    {
	Geom::Point2D<float> pos = getPos(); 
	for ( int idmrk=0; idmrk<nrMarkers(cidx); idmrk++ )
	{
	    const AnnotData::Marker* mrk = getMarker( idmrk, cidx );
	    if ( pos.y < (mrk->zpos_+mEps)  && pos.y > (mrk->zpos_-mEps) )
		return mrk;
	}
    }
    return 0;
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
