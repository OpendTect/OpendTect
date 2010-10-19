/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlayseqgendesc.cc,v 1.2 2010-10-19 15:14:08 cvsbert Exp $";

#include "uistratsinglayseqgendesc.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uimenu.h"
#include "uimsg.h"
#include "stratlayermodel.h"
#include "stratsinglaygen.h"
#include "stratreftree.h"
#include "propertyimpl.h"
#include "keyenum.h"


mImplFactory2Param(uiLayerSequenceGenDesc,uiParent*,
	Strat::LayerSequenceGenDesc&,uiLayerSequenceGenDesc::factory)


uiLayerSequenceGenDesc::uiLayerSequenceGenDesc( uiParent* p,
					Strat::LayerSequenceGenDesc& desc )
    : uiGraphicsView(p,"LayerSequence Gen Desc editor")
    , desc_(desc)
    , border_(10)
    , outeritm_(0)
    , emptyitm_(0)
{
    setPrefWidth( 200 );
    setPrefHeight( 500 );
    reSize.notify( mCB(this,uiLayerSequenceGenDesc,reDraw) );
    reDrawNeeded.notify( mCB(this,uiLayerSequenceGenDesc,reDraw) );

    getMouseEventHandler().buttonReleased.notify(
	    			mCB(this,uiLayerSequenceGenDesc,usrClickCB) );
}


void uiLayerSequenceGenDesc::usrClickCB( CallBacker* cb )
{
    MouseEventHandler& mevh = getMouseEventHandler();
    const int nruns = desc_.size();
    if ( !mevh.hasEvent() || mevh.isHandled()
	    || (nruns>0 && !rightMouseButton(mevh.event().buttonState())) )
	return;

    clickpos_ = mevh.event().pos();
    if ( workrect_.isOutside(clickpos_) )
	return;

    int mnuid = -1;
    if ( nruns > 0 )
    {
	uiPopupMenu mnu( parent(), "Action" );
	mnu.insertItem( new uiMenuItem("&Edit ..."), 0 );
	mnu.insertItem( new uiMenuItem("Add &Above ..."), 1 );
	mnu.insertItem( new uiMenuItem("Add &Below ..."), 2 );
	if ( nruns > 1 )
	{
	    mnu.insertSeparator();
	    mnu.insertItem( new uiMenuItem("&Remove"), 3 );
	}
	mnuid = mnu.exec();
    }

    bool ischgd = false;
    if ( mnuid == 0 )
	ischgd = descEditReq();
    else if ( mnuid ==1 || mnuid == 2 )
	ischgd = newDescReq( mnuid == 1 );
    else if ( mnuid == 3 )
	ischgd = descRemoveReq();

    if ( ischgd )
	reDraw(0);
}


void uiLayerSequenceGenDesc::reDraw( CallBacker* )
{
    uiRect& wr = const_cast<uiRect&>( workrect_ );
    wr.setLeft( border_.left() );
    wr.setRight( (int)(width() - border_.right() + .5) );
    wr.setTop( border_.top() );
    wr.setBottom( (int)(height() - border_.bottom() + .5) );

    if ( !outeritm_ )
    {
	outeritm_ = new uiRectItem;
	outeritm_->setPenColor( Color::Black() );
	scene().addItem( outeritm_ );
    }
    outeritm_->setRect( workrect_.left(), workrect_.top(),
	    		workrect_.width(), workrect_.height() );

    if ( desc_.isEmpty() )
    {
	if ( !emptyitm_ )
	{
	    emptyitm_ = new uiTextItem( "<Click to add>",
		    			mAlignment(HCenter,VCenter) );
	    emptyitm_->setPenColor( Color::Black() );
	    emptyitm_->setPos( workrect_.centre() );
	    scene().addItem( emptyitm_ );
	}
    }
    else
    {
	delete emptyitm_; emptyitm_ = 0;
	doDraw();
    }
}


uiSingleLayerSequenceGenDesc::DispUnit::DispUnit( uiGraphicsScene& scn,
				    const Strat::LayerGenerator& lg )
    : nm_(0)
    , scene_(scn)
    , genmine_(false)
{
    mDynamicCastGet(const Strat::SingleLayerGenerator*,slg,&lg)
    if ( slg )
	gen_ = slg;
    else
    {
	Strat::SingleLayerGenerator* newgen = new Strat::SingleLayerGenerator;
	gen_ = newgen; genmine_ = true;
	Property& pr = newgen->properties().get(0);
	mDynamicCastGet(ValueProperty*,vpr,&pr)
	vpr->val_ = lg.avgThickness();
    }

    nm_ = new uiTextItem( gen_->name(), mAlignment(HCenter,VCenter) );
    scene_.addItem( nm_ );
    nm_->setPenColor( Color::Black() );
    top_ = new uiLineItem;
    top_->setPenStyle( LineStyle(LineStyle::Dot) );
    scene_.addItem( top_ );
}


uiSingleLayerSequenceGenDesc::DispUnit::~DispUnit()
{
    if ( genmine_ )
	delete const_cast<Strat::SingleLayerGenerator*>(gen_);
    delete nm_;
    delete top_;
}


uiSingleLayerSequenceGenDesc::uiSingleLayerSequenceGenDesc( uiParent* p,
	Strat::LayerSequenceGenDesc& d )
    : uiLayerSequenceGenDesc(p,d)
{
    for ( int idx=0; idx<desc_.size(); idx++ )
	insertDispUnit( *desc_[idx], idx );
}


void uiSingleLayerSequenceGenDesc::insertDispUnit(
			    const Strat::LayerGenerator& lgen, int newidx )
{
    DispUnit* newdisp = new DispUnit( scene(), lgen );
    if ( newidx < 0 || newidx >= disps_.size() )
	disps_ += newdisp;
    else
	disps_.insertAt( newdisp, newidx );
}


void uiSingleLayerSequenceGenDesc::doDraw()
{
    if ( disps_.isEmpty() ) return;
    float totth = 0;
    for ( int idx=0; idx<disps_.size(); idx++ )
	totth += disps_[idx]->gen_->avgThickness();
    if ( mIsZero(totth,mDefEps) ) return;

    float curz = 0;
    const float pixperm = workrect_.height() / totth;
    uiPoint midpt( workrect_.centre().x, 0 );
    uiPoint leftpt( workrect_.left(), 0 );
    uiPoint rightpt( workrect_.right(), 0 );
    for ( int idx=0; idx<disps_.size(); idx++ )
    {
	DispUnit& disp = *disps_[idx];
	const float th = disp.gen_->avgThickness();
	disp.topy_ = (int)(workrect_.top() + curz * pixperm);
	disp.boty_ = (int)(workrect_.top() + (curz+th) * pixperm);

	const float midz = curz + .5 * th;
	midpt.y = (disp.topy_ + disp.boty_) / 2;
	leftpt.y = rightpt.y = disp.topy_;

	disp.nm_->setPos( midpt );
	disp.top_->setLine( leftpt, rightpt );

	curz += th;
    }
}


uiSingleLayerSequenceGenDesc::DispUnit* uiSingleLayerSequenceGenDesc::curUnit()
{
    const int idx = curUnitIdx();
    return idx < 0 ? 0 : disps_[idx];
}


int uiSingleLayerSequenceGenDesc::curUnitIdx()
{
    if ( disps_.isEmpty() )
	return -1;

    for ( int idx=0; idx<disps_.size(); idx++ )
    {
	DispUnit* disp = disps_[idx];
	if ( clickpos_.y < disps_[idx]->topy_ )
	    return idx;
    }
    return disps_.size() - 1;;
}


bool uiSingleLayerSequenceGenDesc::newDescReq( bool above )
{
    uiMSG().error( "TODO: implement properly. Now adding something for test" );
    const Strat::UnitRef* ur = desc_.refTree().find(
	    			above ? "jur.Sandstone" : "bnt.delta.Shale" );
    mDynamicCastGet(const Strat::LeafUnitRef*,lur,ur)
    Strat::SingleLayerGenerator* newun = new Strat::SingleLayerGenerator( lur );
    Property& pr = newun->properties().get(0);
    mDynamicCastGet(ValueProperty*,vpr,&pr)
    vpr->val_ = above ? 5 : 10;

    const int curidx = curUnitIdx();
    const int newidx = above ? curidx : curidx + 1;
    if ( desc_.isEmpty() || newidx >= desc_.size() )
	desc_ += newun;
    else
	desc_.insertAt( newun, newidx );
    insertDispUnit( *newun, newidx );

    return true;
}


bool uiSingleLayerSequenceGenDesc::descEditReq()
{
    uiMSG().error( "TODO: implement" );
    return false;
}


bool uiSingleLayerSequenceGenDesc::descRemoveReq()
{
    const int curidx = curUnitIdx();
    if ( curidx < 0 ) return false;

    delete desc_.remove( curidx );
    delete disps_.remove( curidx );
    return true;
}
