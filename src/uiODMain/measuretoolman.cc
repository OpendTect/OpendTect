/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2008
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
#include "dbman.h"
#include "picksetmanager.h"

static const char* sKeyCategory = "MeasureTool";


MeasureToolMan::MeasureToolMan( uiODMain& appl )
    : appl_(appl)
    , measuredlg_(0)
{
    butidx_ = appl.menuMgr().viewTB()->addButton( "measure",
	tr("Display Distance"), mCB(this,MeasureToolMan,buttonClicked), true );

    TypeSet<int> sceneids;
    appl.applMgr().visServer()->getChildIds( -1, sceneids );
    for ( int ids=0; ids<sceneids.size(); ids++ )
	addScene( sceneids[ids] );

    uiODSceneMgr& scenemgr = appl.sceneMgr();
    mAttachCB( scenemgr.treeToBeAdded, MeasureToolMan::sceneAdded );
    mAttachCB( scenemgr.sceneClosed, MeasureToolMan::sceneClosed );
    mAttachCB( scenemgr.activeSceneChanged, MeasureToolMan::sceneChanged );
    mAttachCB( visBase::DM().selMan().selnotifier, MeasureToolMan::objSelected);
    mAttachCB( DBM().surveyChanged, MeasureToolMan::surveyChanged );
}


MeasureToolMan::~MeasureToolMan()
{
    detachAllNotifiers();
    delete measuredlg_;
    if ( !appl_.applMgr().visServer() )
	return;

    for ( int idx=0; idx<displayobjs_.size(); idx++ )
    {
	const int sceneid = sceneids_[idx];
	appl_.applMgr().visServer()->removeObject( displayobjs_[idx], sceneid );
	displayobjs_[idx]->unRef();
    }
}


void MeasureToolMan::objSelected( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    bool isownsel = false;
    for ( int idx=0; idx<displayobjs_.size(); idx++ )
	if ( displayobjs_[idx]->id() == sel )
	    isownsel = true;

    appl_.menuMgr().viewTB()->turnOn( butidx_, isownsel );
    if ( measuredlg_ && !isownsel )
    {
	if ( measuredlg_->doClear() )
	    clearCB( 0 );

	measuredlg_->windowClosed.remove( mCB(this,MeasureToolMan,dlgClosed) );
	measuredlg_->close();
	measuredlg_ = 0;
    }
}


void MeasureToolMan::dlgClosed( CallBacker* )
{
    if ( measuredlg_->doClear() )
	clearCB( 0 );

    appl_.menuMgr().viewTB()->turnOn( butidx_, false );
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
    const bool ison = appl_.menuMgr().viewTB()->isOn( butidx_ );
    manageDlg( ison );
}


void MeasureToolMan::sceneAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sceneid,cb);
    addScene( sceneid );
}


void MeasureToolMan::addScene( int sceneid )
{
    visSurvey::PickSetDisplay* psd = new visSurvey::PickSetDisplay();
    psd->allowDoubleClick( false );
    psd->ref();

    Pick::Set* ps = new Pick::Set( "Measure picks", sKeyCategory );
    ps->setConnection( Pick::Set::Disp::Open );
    ps->setDispColor( Color( 255, 0, 0 ) );
    psd->setSet( ps );
    mAttachCB( ps->objectChanged(), MeasureToolMan::changeCB );

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
    displayobjs_.removeSingle( sceneidx )->unRef();
    sceneids_.removeSingle( sceneidx );
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


static void giveCoordsToDialog( const Pick::Set& ps, uiMeasureDlg& dlg )
{
    TypeSet<Coord3> crds;
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
	crds += psiter.get().pos();
    psiter.retire();

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
    const bool ison = appl_.menuMgr().viewTB()->isOn( butidx_ );
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
    mGetMonitoredChgDataWithCaller( cb, chgdata, caller );
    mDynamicCastGet(Pick::Set*,ps,caller)
    if ( !ps || chgdata.changeType() == Pick::Set::cDispChange() )
	return;

    if ( measuredlg_ )
	giveCoordsToDialog( *ps, *measuredlg_ );
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
	if ( !ps )
	    continue;

	ps->setEmpty();
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
	if ( !ps )
	    continue;

	OD::LineStyle ls( measuredlg_->getLineStyle() );
	Pick::Set::Disp disp = ps->getDisp();
	disp.mkstyle_.color_ = ls.color_;
	disp.mkstyle_.size_ = ls.width_;
	ps->setDisp( disp );
    }
}


void MeasureToolMan::surveyChanged( CallBacker* )
{
    if ( measuredlg_ )
	measuredlg_->close();
}
