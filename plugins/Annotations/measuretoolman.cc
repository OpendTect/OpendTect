/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


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
#include "ioman.h"
#include "pickset.h"

namespace Annotations
{

MeasureToolMan::MeasureToolMan( uiODMain& appl )
    : appl_(appl)
    , picksetmgr_(Pick::SetMgr::getMgr("MeasureTool"))
    , measuredlg_(0)
{
    butidx_ = appl.menuMgr().coinTB()->addButton( "measure",
	    "Display Distance", mCB(this,MeasureToolMan,buttonClicked), true );

    TypeSet<int> sceneids;
    appl.applMgr().visServer()->getChildIds( -1, sceneids );
    for ( int ids=0; ids<sceneids.size(); ids++ )
	addScene( sceneids[ids] );

    appl.sceneMgr().treeToBeAdded.notify(
			mCB(this,MeasureToolMan,sceneAdded) );
    appl.sceneMgr().sceneClosed.notify(
			mCB(this,MeasureToolMan,sceneClosed) );
    appl.sceneMgr().activeSceneChanged.notify(
			mCB(this,MeasureToolMan,sceneChanged) );
    visBase::DM().selMan().selnotifier.notify(
	    		mCB(this,MeasureToolMan,objSelected) );
    IOM().surveyChanged.notify( mCB(this,MeasureToolMan,surveyChanged) );
    picksetmgr_.locationChanged.notify( mCB(this,MeasureToolMan,changeCB) );
}


void MeasureToolMan::objSelected( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    bool isownsel = false;
    for ( int idx=0; idx<displayobjs_.size(); idx++ )
	if ( displayobjs_[idx]->id() == sel )
	    isownsel = true;

    appl_.menuMgr().coinTB()->turnOn( butidx_, isownsel );
    if ( measuredlg_ && !isownsel )
    {
	measuredlg_->windowClosed.remove( mCB(this,MeasureToolMan,dlgClosed) );
	appl_.sceneMgr().setToViewMode( true );
	measuredlg_->close();
	measuredlg_ = 0;
    }
}


void MeasureToolMan::dlgClosed( CallBacker* cb )
{
    appl_.menuMgr().coinTB()->turnOn( butidx_, false );
    appl_.sceneMgr().setToViewMode( true );
    for ( int idx=0; idx<displayobjs_.size(); idx++ )
	visBase::DM().selMan().deSelect( displayobjs_[idx]->id() );

    measuredlg_ = 0;
}


void MeasureToolMan::manageDlg( bool show )
{
    if ( show )
    {
	if ( !measuredlg_ )
	{
	    measuredlg_ = new uiMeasureDlg( &appl_ );
	    measuredlg_->lineStyleChange.notify(
				mCB(this,MeasureToolMan,lineStyleChangeCB) );
	    measuredlg_->velocityChange.notify(
		    		mCB(this,MeasureToolMan,velocityChangeCB) );
	    measuredlg_->clearPressed.notify( mCB(this,MeasureToolMan,clearCB));
	    measuredlg_->windowClosed.notify( 
		    		mCB(this,MeasureToolMan,dlgClosed) );
	    lineStyleChangeCB(0);
	}

	measuredlg_->show();
	appl_.sceneMgr().setToViewMode( false );
    }
    else if ( measuredlg_ )
	measuredlg_->close();

    for ( int idx=0; idx<displayobjs_.size(); idx++ )
    {
	if ( sceneids_[idx] == getActiveSceneID() )
	{
	    if ( show )
		visBase::DM().selMan().select( displayobjs_[idx]->id() );
	    else
		visBase::DM().selMan().deSelect( displayobjs_[idx]->id() );
	}
    }

    update();
}


void MeasureToolMan::buttonClicked( CallBacker* cb )
{
    const bool ison = appl_.menuMgr().coinTB()->isOn( butidx_ );
    manageDlg( ison );
}


static MultiID getMultiID( int sceneid )
{
    // Create dummy multiid, I don't want to save these picks
    BufferString mid( "9999.", sceneid );
    return MultiID( mid.buf() );
}


void MeasureToolMan::sceneAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sceneid,cb);
    addScene( sceneid );
}


void MeasureToolMan::addScene( int sceneid )
{
    visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
    psd->ref();

    Pick::Set* ps = new Pick::Set( "Measure picks" );
    ps->disp_.connect_ = Pick::Set::Disp::Open;
    ps->disp_.color_ = Color( 255, 0, 0 );
    psd->setSet( ps );
    psd->setSetMgr( &picksetmgr_ );
    picksetmgr_.set( getMultiID(sceneid), ps );

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
    sceneids_.remove( sceneidx );
}


int MeasureToolMan::getActiveSceneID() const
{
    const int sceneid = appl_.sceneMgr().getActiveSceneID();
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( sceneidx>=0 )
	return sceneid;

    if ( sceneids_.size() == 1 )
	return sceneids_[0];

    return -1;
}


static void giveCoordsToDialog( const Pick::Set& set, uiMeasureDlg& dlg )
{
    TypeSet<Coord3> crds;
    for ( int idx=0; idx<set.size(); idx++ )
	crds += set[idx].pos;

    dlg.fill( crds );
}


void MeasureToolMan::update()
{
    const int sceneid = getActiveSceneID();
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( !displayobjs_.validIdx(sceneidx) ) return;

    if ( displayobjs_[sceneidx]->getSet() && measuredlg_ )
	giveCoordsToDialog( *displayobjs_[sceneidx]->getSet(), *measuredlg_ );
}


void MeasureToolMan::sceneChanged( CallBacker* )
{
    const bool ison = appl_.menuMgr().coinTB()->isOn( butidx_ );
    if ( !ison ) return;

    const int sceneid = getActiveSceneID();
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( !displayobjs_.validIdx(sceneidx) ) return;

    visBase::DM().selMan().select( displayobjs_[sceneidx]->id() );

    if ( displayobjs_[sceneidx]->getSet() && measuredlg_ )
	giveCoordsToDialog( *displayobjs_[sceneidx]->getSet(), *measuredlg_ );

}


void MeasureToolMan::changeCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb);
    if ( !cd || !cd->set_ ) return;

    giveCoordsToDialog( *cd->set_, *measuredlg_ );
}


void MeasureToolMan::velocityChangeCB( CallBacker* )
{
    update();
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

    if ( measuredlg_ )
	measuredlg_->reset();
}


void MeasureToolMan::lineStyleChangeCB( CallBacker* )
{
    if ( !measuredlg_ ) return;

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


void MeasureToolMan::surveyChanged( CallBacker* )
{
    if ( measuredlg_ )
	measuredlg_->close();

    picksetmgr_.locationChanged.notify( mCB(this,MeasureToolMan,changeCB) );
}


} // namespace Annotations
