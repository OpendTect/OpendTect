/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2008
 RCS:		$Id: measuretoolman.cc,v 1.2 2008-08-03 18:08:30 cvsnanne Exp $
________________________________________________________________________

-*/


#include "measuretoolman.h"

#include "uimeasuredlg.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "visdataman.h"
#include "vispicksetdisplay.h"
#include "visselman.h"

#include "draw.h"
#include "pickset.h"

namespace Annotations
{

MeasureToolMan::MeasureToolMan( uiODMain& appl )
    : appl_(appl)
    , picksetmgr_(Pick::SetMgr::getMgr("MeasureTool"))
    , measuredlg_(0)
{
    const CallBack cb( mCB(this,MeasureToolMan,buttonClicked) );
    appl.menuMgr().coinTB()->addButton( "measure.png", cb, 
	    				"Display Distance", true );

    appl.sceneMgr().treeToBeAdded.notify(
			mCB(this,MeasureToolMan,sceneAdded) );
    appl.sceneMgr().sceneClosed.notify(
			mCB(this,MeasureToolMan,sceneClosed) );
}


void MeasureToolMan::buttonClicked( CallBacker* cb )
{
    appl_.sceneMgr().setToViewMode( false );

    for ( int idx=0; idx<displayobjs_.size(); idx++ )
    {
	if ( true )
	    visBase::DM().selMan().select( displayobjs_[idx]->id() );
	else
	    visBase::DM().selMan().deSelect( displayobjs_[idx]->id() );
    }
}

static int id = 0;

void MeasureToolMan::sceneAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sceneid,cb);
    visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
    psd->ref();

    Pick::Set* ps = new Pick::Set( "Measure picks" );
    ps->disp_.connect_ = Pick::Set::Disp::Open;
    ps->disp_.color_ = Color( 255, 0, 0 );
    psd->setSet( ps );
    psd->setSetMgr( &picksetmgr_ );
    BufferString mid( "9999.", ++id );
    picksetmgr_.set( MultiID(mid.buf()), ps );
    picksetmgr_.locationChanged.notify( mCB(this,MeasureToolMan,changeCB) );

    appl_.applMgr().visServer()->addObject( psd, sceneid, false );
    sceneids_ += sceneid;
    displayobjs_ += psd;
}


void MeasureToolMan::sceneClosed( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sceneid,cb);
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( sceneidx<0 || sceneidx>=displayobjs_.size() )
	return;

    appl_.applMgr().visServer()->removeObject( displayobjs_[sceneidx], sceneid);
    displayobjs_[sceneidx]->unRef();
    displayobjs_.remove( sceneidx );
} 


void MeasureToolMan::changeCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb);
    if ( !cd || !cd->set_ ) return;

    if ( !measuredlg_ )
    {
	measuredlg_ = new uiMeasureDlg( 0 );
	LineStyle ls;
	ls.color_ = cd->set_->disp_.color_;
	ls.width_ = cd->set_->disp_.pixsize_;
	measuredlg_->setLineStyle( ls );
	measuredlg_->lineStyleChange.notify(
				mCB(this,MeasureToolMan,lineStyleChangeCB) ) ;
	measuredlg_->clearPressed.notify( mCB(this,MeasureToolMan,clearCB) );
    }

    measuredlg_->show();
    TypeSet<Coord3> crds;
    Pick::Set chgdset( *cd->set_ );
    if ( cd->ev_ == Pick::SetMgr::ChangeData::ToBeRemoved )
	chgdset.remove( cd->loc_ );

    for ( int idx=0; idx<chgdset.size(); idx++ )
	crds += chgdset[idx].pos;
    measuredlg_->fill( crds );
}


void MeasureToolMan::clearCB( CallBacker* )
{
    for ( int idx=0; idx<displayobjs_.size(); idx++ )
    {
	Pick::Set* ps = displayobjs_[idx]->getSet();
	if ( !ps ) continue;

	ps->erase();
	picksetmgr_.reportChange( this, *ps );
	displayobjs_[idx]->createLine();
    }

    measuredlg_->reset();
}


void MeasureToolMan::lineStyleChangeCB( CallBacker* cb )
{
    for ( int idx=0; idx<displayobjs_.size(); idx++ )
    {
	Pick::Set* ps = displayobjs_[idx]->getSet();
	if ( !ps ) continue;

      	LineStyle ls( measuredlg_->getLineStyle() );
	ps->disp_.color_ = ls.color_;
	ps->disp_.pixsize_ = ls.width_;
	picksetmgr_.reportDispChange( this, *ps );
    }
}


} // namespace Annotations
