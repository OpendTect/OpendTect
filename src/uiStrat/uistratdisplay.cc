/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdisplay.cc,v 1.8 2010-05-10 08:44:20 cvsbruno Exp $";

#include "uistratdisplay.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uidialog.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uispinbox.h"
#include "uistratutildlgs.h"
#include "uistratreftree.h"
#include "uimenuhandler.h"

#include "genericnumer.h"
#include "draw.h"
#include "randcolor.h"
#include "survinfo.h"

uiStratDisplay::uiStratDisplay( uiParent* p, uiStratRefTree& tree )
    : uiAnnotDisplay(p,"Stratigraphy viewer")
    , uidatagather_(new uiStratAnnotGather(data_,tree.stratmgr()))
    , uidatawriter_(new uiStratTreeWriter(tree))
    , speclvlmnuitem_("&Specify level boundary ...",4)			
    , propunitmnuitem_("&Properties ...",5)			
    , addsubunitmnuitem_("&Create sub-unit...",6)
{
    uidatagather_->newtreeRead.notify( mCB(this,uiStratDisplay,dataChanged) );
    createDispParamGrp();
    dataChanged( 0 );
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
    stepfld_ = new uiLabeledSpinBox( dispparamgrp_, "Gridline step");
    stepfld_->attach( rightOf, rangefld_ );
    stepfld_->box()->setInterval( 1, 500, 100 );
    stepfld_->box()->valueChanging.notify(mCB(this,uiStratDisplay,dispParamChgd));
    if ( nrCols() && nrUnits(0) )
    {
	const AnnotData::Unit& topunit = *getUnit(0,0);
	const AnnotData::Unit& botunit = *getUnit(nrUnits(0)-1,0);
	float start = topunit.zpos_, stop = botunit.zposbot_; 
	float step = fabs(stop-start)/100;
	rangefld_->setValue( StepInterval<float>( start, stop, step ) );
	setZRange( StepInterval<float>( stop, start, step ) );
    }
}


void uiStratDisplay::dataChanged( CallBacker* cb )
{
    makeAnnots();
    draw();
}


void uiStratDisplay::display( bool yn, bool shrk, bool maximize )
{
    uiGraphicsView::display( yn );
    dispparamgrp_->display( yn );
}


void uiStratDisplay::dispParamChgd( CallBacker* cb )
{
    StepInterval<float> rg = rangefld_->getFInterval();
    rg.step = stepfld_->box()->getValue();
    rg.sort( false );
    yax_.setRange( rg );
    draw();
}


void uiStratDisplay::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;


    int cidx = getColIdxFromPos();
    AnnotData::Column* col = cidx >=0 ? data_.getCol( cidx ) : 0;
    if ( !col ) return;
    bool unitfound = getUnitFromPos();
    mAddMenuItem( menu, &addunitmnuitem_, !unitfound && col->iseditable_,false);
    mAddMenuItem( menu, &remunitmnuitem_, unitfound && col->iseditable_, false);
    mAddMenuItem( menu, &propunitmnuitem_, unitfound && col->iseditable_,false);
    mAddMenuItem( menu, &speclvlmnuitem_, unitfound && col->iseditable_, false);
    mAddMenuItem( menu, &addsubunitmnuitem_,unitfound &&col->iseditable_,false);
}



void uiStratDisplay::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
    return;

    const AnnotData::Unit* unit = getUnitFromPos();
    bool ishandled = true;
    if ( mnuid == addsubunitmnuitem_.id )
    {
	int colidx = getColIdxFromPos();
	if ( colidx >= 0 )
	    uidatawriter_->addUnit( unit->name_ );
    }
    if ( mnuid == addunitmnuitem_.id )
    {
	int colidx = getColIdxFromPos();
	if ( colidx > 0 )
	{
	    const AnnotData::Unit* parunit = getParentUnitFromPos();
	    if ( parunit )
		uidatawriter_->addUnit( parunit->name_ );
	}
	else if ( colidx == 0 ) 
	    uidatawriter_->addUnit( "", false );
    }
    if ( mnuid == addcolmnuitem_.id )
    {
    }
    if ( mnuid == speclvlmnuitem_.id && unit )
	uidatawriter_->selBoundary( unit->name_ );
    if ( mnuid == remunitmnuitem_.id && unit )
	uidatawriter_->removeUnit( unit->name_ );
    if ( mnuid == propunitmnuitem_.id && unit )
	uidatawriter_->updateUnitProperties( unit->name_ );
}


const AnnotData::Unit* uiStratDisplay::getParentUnitFromPos() const
{
    Geom::Point2D<float> pos = getPos();
    int cidx = getColIdxFromPos( );
    if ( cidx > 0 || cidx < nrCols() ) 
    {
	for ( int idunit=0; idunit<nrUnits(cidx-1); idunit++ )
	{
	    const AnnotData::Unit* unit = getUnit( idunit, cidx-1 );
	    if ( pos.y < unit->zposbot_ && pos.y > unit->zpos_ )
		return unit;
	}
    }
    return 0;
}


void uiStratDisplay::makeAnnots()
{
    makeAnnotCol( "Levels", 0, true ); 
    makeAnnotCol( "Description", 4, false );
    makeAnnotCol( "Lithology", 5, false );
}


void uiStratDisplay::makeAnnotCol( const char* txt, int annotpos, bool level )
{
    AnnotData::Column* annotcol = new AnnotData::Column( txt );
    for ( int idcol=0; idcol<nrCols(); idcol++ )
    {
	const AnnotData::Column& col = *data_.getCol( idcol );
	for ( int idunit=0; idunit<col.units_.size(); idunit++ )
	{
	    const AnnotData::Unit* unit = col.units_[idunit];
	    if ( !unit || !unit->annots_.size() ) continue;
	    const BufferStringSet& annot = unit->annots_;
	    if ( level )
	    {
		AnnotData::Marker* topmrk = new AnnotData::Marker( 
					    annot[annotpos]->buf(), 
					    unit->zpos_ );
		topmrk->col_ = Color( 
				(unsigned int)atoi(annot[annotpos+1]->buf()) );
		annotcol->markers_ += topmrk;
		topmrk->isnmabove_ = true;
		AnnotData::Marker* botmrk = new AnnotData::Marker( 
					    annot[annotpos+2]->buf(),
					    unit->zposbot_ );
		botmrk->col_ = Color( 
				(unsigned int)atoi(annot[annotpos+3]->buf()) );
		annotcol->markers_ += botmrk;
		botmrk->isnmabove_ = false;
	    }
	    else
	    {
		AnnotData::Unit* newunit = new AnnotData::Unit(  
					    annot[annotpos]->buf(),
					    unit->zpos_,
					    unit->zposbot_ );
		newunit->col_ = Color::White();
		annotcol->units_ += newunit;
	    }
	}
    }
    annotcol->iseditable_ = false;
    data_.addCol( annotcol );
}



uiAnnotDisplay::uiAnnotDisplay( uiParent* p, const char* nm )
    : uiGraphicsView(p,nm)
    , data_(AnnotData()) 
    , xax_(&scene(),uiAxisHandler::Setup(uiRect::Top))
    , yax_(&scene(),uiAxisHandler::Setup(uiRect::Left).nogridline(true))
    , menu_(*new uiMenuHandler(p,-1))
    , addunitmnuitem_("Add Unit...",0)
    , remunitmnuitem_("Remove unit...",1)
    , addcolmnuitem_("Add Column...",2)
{
    xax_.setBounds( StepInterval<float>( 0, 100, 10 ) );

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
    draw();
    show();
}


void uiAnnotDisplay::reSized( CallBacker* )
{
    draw();
}


void uiAnnotDisplay::updateAxis()
{
    xax_.setNewDevSize( width(), height() );
    yax_.setNewDevSize( height(), width() );
    xax_.setBegin( &yax_ );     yax_.setBegin( &xax_ );
    xax_.setEnd( &yax_ );       yax_.setEnd( &xax_ );
    yax_.plotAxis();
}



void uiAnnotDisplay::draw()
{
    eraseAll();
    updateAxis();
    drawColumns();
}


void uiAnnotDisplay::drawColumns()
{
    for ( int idx=0; idx<nrCols(); idx++ )
    {
	ColumnItem* colitm = new ColumnItem( data_.getCol( idx )->name_ );
	colitms_ += colitm;
	colitm->size_ = (int)xax_.getVal( (int)(width()) )/nrCols() ;
	if ( colitm->size_ <0 ) colitm->size_ = 0;

	drawBorders( idx );
	drawMarkers( idx );
	drawUnits( idx );
    }
}


#define mRemoveSet( itms ) \
    for ( int idx=0; idx<itms.size(); idx++ ) \
    scene().removeItem( itms[idx] ); \
    deepErase( itms );
void uiAnnotDisplay::eraseAll()
{
    for ( int idx = colitms_.size()-1; idx>=0; idx-- )
    {
	ColumnItem* colitm = colitms_[idx];

	delete scene().removeItem( colitm->borderitm_ ); 
	delete scene().removeItem( colitm->bordertxtitm_ ); 
	mRemoveSet( colitm->mrktxtitms_ )
	mRemoveSet( colitm->unittxtitms_ )
	mRemoveSet( colitm->mrkitms_ )
	mRemoveSet( colitm->unititms_ )
    }
    deepErase( colitms_ );
}


void uiAnnotDisplay::drawBorders( int colidx )
{
    ColumnItem* colitm = colitms_[colidx];

    int x1 = xax_.getPix( (colidx)*colitm->size_ );
    int x2 = xax_.getPix( (colidx+1)*colitm->size_ );
    int y1 = yax_.getPix( yax_.range().stop );
    int y2 = yax_.getPix( yax_.range().start );
	
    TypeSet<uiPoint> rectpts;
    rectpts += uiPoint( x1, y1 );
    rectpts += uiPoint( x2, y1  );
    rectpts += uiPoint( x2, y2  );
    rectpts += uiPoint( x1, y2  );
    rectpts += uiPoint( x1, y1  );
    uiPolyLineItem* pli = scene().addItem( new uiPolyLineItem( rectpts ) );
    pli->setPenStyle( LineStyle(LineStyle::Solid,1,Color::Black()) );
    colitm->borderitm_ = pli;

    uiTextItem* ti = scene().addItem( new uiTextItem( colitm->name_ ) );
    ti->setTextColor( Color::Black() );
    ti->setPos( x1, y1 - 18 );
    ti->setZValue( 2 );
    colitm->bordertxtitm_ = ti;
}


void uiAnnotDisplay::drawMarkers( int colidx )
{
    ColumnItem* colitm = colitms_[colidx];
    mRemoveSet( colitm->mrkitms_ );
    mRemoveSet( colitm->mrktxtitms_ );
    for ( int idx=0; idx<nrMarkers( colidx ); idx++ )
    {
	AnnotData::Marker& mrk = *getMarker( idx, colidx );

	int x1 = xax_.getPix( (colidx)*colitm->size_ );
	int x2 = xax_.getPix( (colidx+1)*colitm->size_ );
	int y = yax_.getPix( mrk.zpos_ );

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	li->setPenStyle( LineStyle(LineStyle::Solid,2,mrk.col_) );
	uiTextItem* ti = scene().addItem( new uiTextItem( mrk.name_.buf() ) );
	ti->setPos( x1 + (x2-x1)/2, y ); 
	ti->setZValue( 2 );
	ti->setTextColor( mrk.col_ );
	colitm->mrktxtitms_ += ti;
	colitm->mrkitms_ += li;
    }
}


void uiAnnotDisplay::drawUnits( int colidx ) 
{
    ColumnItem* colitm = colitms_[colidx];
    mRemoveSet( colitm->unittxtitms_ );
    mRemoveSet( colitm->unititms_ );

    for ( int idx=0; idx<nrUnits( colidx ); idx++ )
    {
	AnnotData::Unit& unit = *getUnit( idx, colidx );

	int x1 = xax_.getPix( (colidx)*colitm->size_ );
	int x2 = xax_.getPix( (colidx+1)*colitm->size_ );
	int y1 = yax_.getPix( unit.zpos_ );
	int y2 = yax_.getPix( unit.zposbot_ );

	TypeSet<uiPoint> rectpts;
	rectpts += uiPoint( x1, y1 );
	rectpts += uiPoint( x2, y1 );
	rectpts += uiPoint( x2, y2 );
	rectpts += uiPoint( x1, y2 );
	rectpts += uiPoint( x1, y1 );
	uiPolygonItem* pli = scene().addPolygon( rectpts, true );
	pli->setPenColor( Color::Black() );
	if ( unit.col_ != Color::White() )
	    pli->setFillColor( unit.col_ );
	uiTextItem* ti = scene().addItem( new uiTextItem( unit.name_ ) );
	ti->setTextColor( Color::Black() );
	ti->setPos( x1, y2 - abs((y2-y1)/2) -10 );
	ti->setZValue( 2 );
	colitm->unittxtitms_ += ti;
	colitm->unititms_ += pli;
    }
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
    //mAddMenuItem( menu, &addcolmnuitem_, true, false );
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
    draw();
}


Geom::Point2D<float> uiAnnotDisplay::getPos() const
{
    uiAnnotDisplay* self = const_cast<uiAnnotDisplay*>( this );
    float xpos = xax_.getVal( self->getMouseEventHandler().event().pos().x ); 
    float ypos = yax_.getVal( self->getMouseEventHandler().event().pos().y ); 
    return Geom::Point2D<float>( xpos, ypos );
}


int uiAnnotDisplay::getColIdxFromPos() const 
{
    float xpos = getPos().x;
    Interval<int> borders(0,0);
    for ( int idx=0; idx<nrCols(); idx++ )
    {
	borders.stop += colitms_[idx]->size_;
	if ( borders.includes( xpos ) ) 
	    return idx;
	borders.start = borders.stop;
    }
    return -1;
}



const AnnotData::Unit* uiAnnotDisplay::getUnitFromPos() const
{
    int cidx = getColIdxFromPos();
    if ( cidx >=0 && cidx<nrCols() )
    {
	Geom::Point2D<float> pos = getPos(); 
	for ( int idunit=0; idunit<nrUnits(cidx); idunit++ )
	{
	    const AnnotData::Unit* unit = getUnit( idunit, cidx );
	    if ( pos.y < unit->zposbot_ && pos.y > unit->zpos_ )
		return unit;
	}
    }
    return 0;
}

