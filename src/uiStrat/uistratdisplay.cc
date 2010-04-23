/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdisplay.cc,v 1.6 2010-04-23 08:36:27 cvsbruno Exp $";

#include "uistratdisplay.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uidialog.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uimenuhandler.h"

#include "draw.h"
#include "randcolor.h"
#include "survinfo.h"

uiStratDisplay::uiStratDisplay( uiParent* p )
    : uiGraphicsView(p,"Vertical Stratigraphy viewer")
    , data_(uiStratDisp()) 
    , xax_(&scene(),uiAxisHandler::Setup(uiRect::Top)
					.nogridline(false)
					.border(uiBorder(0))
					.noborderspace(true)
					.noaxisline(false)
					.ticsz(-15))
    , yax_(&scene(),uiAxisHandler::Setup(uiRect::Left)
					.nogridline(false)
					.noaxisline(true)
					.border(uiBorder(0))
					.noborderspace(true))
    , menu_(*new uiMenuHandler(p,-1))
    , addunitmnuitem_("Add Unit...",0)
    , remunitmnuitem_("Remove unit...",1)
{
    setPrefWidth( 70 );
    setPrefHeight( 600 );
    
    StepInterval<float> rg( SI().zRange(true) );
    rg.sort( false );
    setZRange( rg );

    getMouseEventHandler().buttonReleased.notify(
					mCB(this,uiStratDisplay,usrClickCB) );
    reSize.notify( mCB(this,uiStratDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiStratDisplay,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiStratDisplay,handleMenuCB));
    
    data_.dataChanged.notify( mCB(this,uiStratDisplay,doDataChange) );
    finaliseDone.notify( mCB(this,uiStratDisplay,init) );
}


uiStratDisplay::~uiStratDisplay()
{
    menu_.unRef();
}


int uiStratDisplay::nrSubUnits()
{
    int nrsubunits = 0;
    for ( int idx=0; idx<nrUnits(); idx++ )
    {
	if ( getUnit(idx)->order_ > nrsubunits )
	    nrsubunits  = getUnit(idx)->order_;
    }
    return nrsubunits+1;
}


void uiStratDisplay::gatherInfo()
{
    data_.gatherInfo();
}


void uiStratDisplay::doDataChange( CallBacker* )
{
    dataChanged();
}


void uiStratDisplay::init( CallBacker* )
{
    dataChanged();
    show();
}


void uiStratDisplay::reSized( CallBacker* )
{
    draw();
}


void uiStratDisplay::updateAxis()
{
    xax_.setNewDevSize( width(), height() );
    yax_.setNewDevSize( height(), width() );
    xax_.setBegin( &yax_ );     yax_.setBegin( &xax_ );
    xax_.setEnd( &yax_ );       yax_.setEnd( &xax_ );
}


void uiStratDisplay::dataChanged()
{
    gatherInfo(); draw();
}


void uiStratDisplay::draw()
{
    updateAxis();
    drawLevels();
    drawUnits();
}

#define mRemoveSet( itms ) \
    for ( int idx=0; idx<itms.size(); idx++ ) \
    scene().removeItem( itms[idx] ); \
    deepErase( itms );
void uiStratDisplay::drawLevels()
{
    mRemoveSet( lvlitms_ );
    for ( int idx=0; idx<nrLevels(); idx++ )
    {
	uiStratDisp::Level& lvl = *getLevel( idx );

	int x1 = lvl.order_*width();
	int x2 = x1 + width();
	int y = yax_.getPix( lvl.zpos_ );

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	li->setPenStyle( LineStyle(LineStyle::Solid,3,lvl.col_) );
	lvlitms_ += li;
    }
}


void uiStratDisplay::drawUnits()
{
    mRemoveSet( txtitms_ );
    mRemoveSet( unititms_ );
    int subwidthfactor = width()/nrSubUnits();

    for ( int idx=0; idx<nrUnits(); idx++ )
    {
	uiStratDisp::Unit& unit = *getUnit( idx );
	
	int x1 = unit.order_*subwidthfactor;
	int x2 = x1 + subwidthfactor;
	int y1 = yax_.getPix( unit.zpos_ );
	int y2 = yax_.getPix( unit.zposbot_ );

	TypeSet<uiPoint> rectpts;
	rectpts += uiPoint( x1, y1 );
	rectpts += uiPoint( x2, y1  );
	rectpts += uiPoint( x2, y2  );
	rectpts += uiPoint( x1, y2  );
	rectpts += uiPoint( x1, y1  );
	uiPolygonItem* pli = scene().addPolygon( rectpts, true );
	pli->setFillColor( unit.col_ );
	uiTextItem* ti = scene().addItem( new uiTextItem( unit.name_ ) );
	ti->setTextColor( Color::White() );
	ti->setPos( x2/2, y1+(y2-y1)/2 );
	ti->rotate( 270 );
	ti->setZValue( unit.order_ );
	txtitms_ += ti;
	unititms_ += pli;
    }
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
	menu_.executeMenu(0);
	return true;
    }
    return false;
}


void uiStratDisplay::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;

    mAddMenuItem( menu, &remunitmnuitem_, true, false );
    mAddMenuItem( menu, &addunitmnuitem_, true, false );
}


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
	botdepthfld_ = new uiGenInput(this,"Bottom Value",FloatInpSpec(zpos+1));
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
	unit_ = new uiStratDisp::Unit( nm, topdpt, botdpt );
	unit_->col_ =  colorfld_->color();
	return true;
    }

    uiStratDisp::Unit* unit() { return unit_; }

protected :

    uiGenInput*         namefld_;
    uiGenInput*         topdepthfld_;
    uiGenInput*         botdepthfld_;
    uiColorInput*       colorfld_;
    uiStratDisp::Unit* 	unit_;
};


void uiStratDisplay::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
    return;

    bool ishandled = true;
    if ( mnuid==addunitmnuitem_.id )
    {
	uiCreateAnnotDlg dlg( parent(), 
		(float)getMouseEventHandler().event().pos().y );
	if ( dlg.go() )
	{
	    uiStratDisp::Unit* unit = dlg.unit();
	    if ( unit ) data_.units_ += unit;
	}
	draw();
    }
}


