/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdisplay.cc,v 1.2 2010-03-23 13:36:56 cvsbruno Exp $";

#include "uistratdisplay.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uidialog.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uimenuhandler.h"
#include "uistratmgr.h"

#include "draw.h"
#include "randcolor.h"
#include "survinfo.h"
#include "stratlevel.h"
#include "stratreftree.h"
#include "stratunitrepos.h"


uiStratDisplay::uiStratDisplay( uiParent* p )
    : uiAnnotDisplay(p)
    , uistratmgr_(new uiStratMgr(p))
{
    StepInterval<float> rg( SI().zRange(true) );
    rg.sort( false );
    setZRange( rg );
}


void uiStratDisplay::gatherInfo()
{
    addLevels();
    addNode( *((Strat::NodeUnitRef*)uistratmgr_->getCurTree()), 0 );
}


void uiStratDisplay::addLevels()
{
    TypeSet<Color> lvlcolors; BufferStringSet lvlnms;
    uistratmgr_->getLvlsTxtAndCol( lvlnms, lvlcolors );
    TypeSet< Interval<float> > lvlrgs;
    for ( int idx=0; idx<lvlnms.size(); idx++ )
    {
	Interval<float> rg; Color col;
	uistratmgr_->getLvlPars( lvlnms[idx]->buf(), rg, col );
	if ( mIsUdf( rg.start ) ) continue;
	uiAnnotDisplay::MarkerData* md = 
	    new uiAnnotDisplay::MarkerData( lvlnms[idx]->buf(), rg.start );
	md->col_ = col;
	lvlrgs += rg;
	mrkdatas_ += md;
    }
}


void uiStratDisplay::addNode( const Strat::NodeUnitRef& nur, int order ) 
{
    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const Strat::UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    addUnitAnnot( nur, order-1 );
	    if ( order+1 > nrsubannots_ ) nrsubannots_ = order+1;
	    mDynamicCastGet(const Strat::LeafUnitRef*,lur,&ref);
	    if ( !lur ) continue;
	    addUnitAnnot( *lur, order );
	}
	else
	{
	    mDynamicCastGet(const Strat::NodeUnitRef*,chldnur,&ref);
	    if ( chldnur )
	    { order++; addNode( *chldnur, order ); }
	}
    }
}


void uiStratDisplay::addUnitAnnot( const Strat::UnitRef& uref, int order )
{
    const Strat::Level* toplvl = Strat::eRT().getLevel( &uref, true );
    const Strat::Level* baselvl = Strat::eRT().getLevel( &uref, false );
    if ( !toplvl || !baselvl ) return;
    Interval<float> pos = getUnitPos( *toplvl, *baselvl );
    uiAnnotDisplay::AnnotData* ad = 
	new uiAnnotDisplay::AnnotData( uref.code(), pos.start, pos.stop ); 
    ad->col_ = toplvl->color_;
    ad->order_ = order;
    annotdatas_ += ad;
}


Interval<float> uiStratDisplay::getUnitPos( const Strat::Level& toplvl, 
					    const Strat::Level& baselvl ) 
{
    return Interval<float>( toplvl.timerg_.start, baselvl.timerg_.start );
}


uiAnnotDisplay::uiAnnotDisplay( uiParent* p )
    : uiGraphicsView(p,"Vertical Stratigraphy viewer")
    , nrsubannots_(1)
    , zax_(&scene(),uiAxisHandler::Setup(uiRect::Left))	      
	, menu_(*new uiMenuHandler(p,-1))
        , addannotmnuitem_("Add annot...",0)
        , remannotmnuitem_("Remove annot...",1)
{
    setPrefWidth( 70 );
    setPrefHeight( 600 );

    getMouseEventHandler().buttonReleased.notify(
					mCB(this,uiAnnotDisplay,usrClickCB) );
    setStretch( 2, 2 );
    reSize.notify( mCB(this,uiAnnotDisplay,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );

    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiAnnotDisplay,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiAnnotDisplay,handleMenuCB));
    
    finaliseDone.notify( mCB(this,uiAnnotDisplay,init) );
}


void uiAnnotDisplay::init( CallBacker* )
{
    dataChanged();
    show();
}


void uiAnnotDisplay::reSized( CallBacker* )
{
    draw();
}


void uiAnnotDisplay::updateAxis()
{
    zax_.setNewDevSize( height(), width() );
}


void uiAnnotDisplay::dataChanged()
{
    gatherInfo(); draw();
}


void uiAnnotDisplay::gatherInfo()
{}


void uiAnnotDisplay::draw()
{
    updateAxis();
    drawLevels();
    drawAnnots();
}


void uiAnnotDisplay::drawLevels()
{
    for ( int idx=0; idx<mrkdatas_.size(); idx++ )
    {
	uiAnnotDisplay::MarkerData& ld = *mrkdatas_[idx];
	delete scene().removeItem( ld.itm_  );
	delete scene().removeItem( ld.txtitm_  );

	int x1 = ld.order_*width();
	int x2 = x1 + width();
	int y = zax_.getPix( ld.zpos_ );

	uiLineItem* li = scene().addItem( new uiLineItem(x1,y,x2,y,true) );
	li->setPenStyle( LineStyle(LineStyle::Solid,3,ld.col_) );
	ld.itm_ = li;
    }
}


void uiAnnotDisplay::drawAnnots()
{
    int subwidthfactor = width()/nrsubannots_;

    for ( int idx=0; idx<annotdatas_.size(); idx++ )
    {
	uiAnnotDisplay::AnnotData& ad = *annotdatas_[idx];
	delete scene().removeItem( ad.plitm_  );
	delete scene().removeItem( ad.txtitm_  );

	int x1 = ad.order_*subwidthfactor;
	int x2 = x1 + subwidthfactor;
	int y1 = zax_.getPix( ad.zpos_ );
	int y2 = zax_.getPix( ad.zposbot_ );

	TypeSet<uiPoint> rectpts;
	rectpts += uiPoint( x1, y1 );
	rectpts += uiPoint( x2, y1  );
	rectpts += uiPoint( x2, y2  );
	rectpts += uiPoint( x1, y2  );
	rectpts += uiPoint( x1, y1  );
	uiPolygonItem* pli = scene().addPolygon( rectpts, true );
	pli->setFillColor( ad.col_ );
	uiTextItem* ti = scene().addItem( new uiTextItem( ad.name_ ) );
	ti->setTextColor( Color::White() );
	ti->setPos( x2/2, y1+(y2-y1)/2 );
	ti->rotate( 270 );
	ti->setZValue(1);
	ad.txtitm_ = ti;
	ad.plitm_ = pli;
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

    mAddMenuItem( menu, &remannotmnuitem_, true, false );
    mAddMenuItem( menu, &addannotmnuitem_, true, false );
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
	annot_ = new uiAnnotDisplay::AnnotData( nm, topdpt, botdpt );
	annot_->col_ =  colorfld_->color();
	return true;
    }

    uiAnnotDisplay::AnnotData* annot() { return annot_; }

protected :

    uiGenInput*         namefld_;
    uiGenInput*         topdepthfld_;
    uiGenInput*         botdepthfld_;
    uiColorInput*       colorfld_;
    uiAnnotDisplay::AnnotData* annot_;
};


void uiAnnotDisplay::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
    return;

    bool ishandled = true;
    if ( mnuid==addannotmnuitem_.id )
    {
	uiCreateAnnotDlg dlg( parent(), 
		(float)getMouseEventHandler().event().pos().y );
	if ( dlg.go() )
	{
	    uiAnnotDisplay::AnnotData* annot = dlg.annot();
	    if ( annot ) annotdatas_ += annot;
	}
	draw();
    }
}


