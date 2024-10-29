/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
#include "ioman.h"


MeasureToolMan::MeasureToolMan( uiODMain& appl )
    : appl_(appl)
    , picksetmgr_(Pick::SetMgr::getMgr("MeasureTool"))
{
    butidx_ = appl.menuMgr().viewTB()->addButton( "measure",
	tr("Display Distance"), mCB(this,MeasureToolMan,buttonClicked), true );

    TypeSet<SceneID> sceneids;
    appl.applMgr().visServer()->getSceneIds( sceneids );
    for ( int ids=0; ids<sceneids.size(); ids++ )
	addScene( sceneids[ids] );

    uiODSceneMgr& scenemgr = appl.sceneMgr();
    mAttachCB( scenemgr.treeToBeAdded, MeasureToolMan::sceneAdded );
    mAttachCB( scenemgr.sceneClosed, MeasureToolMan::sceneClosed );
    mAttachCB( scenemgr.activeSceneChanged, MeasureToolMan::sceneChanged );
    mAttachCB( visBase::DM().selMan().selnotifier, MeasureToolMan::objSelected);
    mAttachCB( IOM().surveyChanged, MeasureToolMan::surveyChanged );
    mAttachCB( picksetmgr_.locationChanged, MeasureToolMan::changeCB );
}


MeasureToolMan::~MeasureToolMan()
{
    detachAllNotifiers();
    delete measuredlg_;
    if ( !appl_.applMgr().visServer() )
	return;

    for ( int idx=0; idx<displayids_.size(); idx++ )
    {
	const SceneID sceneid = sceneids_[idx];
	appl_.applMgr().visServer()->removeObject( displayids_[idx], sceneid );
    }
}


void MeasureToolMan::objSelected( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(VisID,sel,cb);
    bool isownsel = false;
    for ( const auto& displayid : displayids_ )
	if ( displayid == sel )
	    isownsel = true;

    appl_.menuMgr().viewTB()->turnOn( butidx_, isownsel );
    if ( measuredlg_ && !isownsel )
    {
	if ( measuredlg_->doClear() )
	    clearCB( nullptr );

	mDetachCB( measuredlg_->windowClosed, MeasureToolMan::dlgClosed );
	closeAndNullPtr( measuredlg_ );
    }
}


void MeasureToolMan::dlgClosed( CallBacker* )
{
    if ( measuredlg_->doClear() )
	clearCB( nullptr );

    appl_.menuMgr().viewTB()->turnOn( butidx_, false );
    for ( const auto& displayid : displayids_ )
	visBase::DM().selMan().deSelect( displayid );

    measuredlg_ = nullptr;
}


void MeasureToolMan::manageDlg( bool show )
{
    if ( show )
    {
	if ( !measuredlg_ )
	{
	    measuredlg_ = new uiMeasureDlg( &appl_ );
	    mAttachCB( measuredlg_->lineStyleChange,
		       MeasureToolMan::lineStyleChangeCB );
	    mAttachCB( measuredlg_->velocityChange,
		       MeasureToolMan::velocityChangeCB );
	    mAttachCB( measuredlg_->dipUnitChange,
		       MeasureToolMan::velocityChangeCB );
	    mAttachCB( measuredlg_->clearPressed, MeasureToolMan::clearCB );
	    mAttachCB( measuredlg_->windowClosed, MeasureToolMan::dlgClosed );
	    lineStyleChangeCB( nullptr );
	}

	measuredlg_->show();
	appl_.sceneMgr().setToViewMode( false );
    }
    else if ( measuredlg_ )
	measuredlg_->close();

    for ( int idx=0; idx<displayids_.size(); idx++ )
    {
	if ( sceneids_[idx] != getActiveSceneID() )
	    continue;

	if ( show )
	    visBase::DM().selMan().select( displayids_[idx] );
	else
	    visBase::DM().selMan().deSelect( displayids_[idx] );
    }

    update();
}


void MeasureToolMan::buttonClicked( CallBacker* cb )
{
    const bool ison = appl_.menuMgr().viewTB()->isOn( butidx_ );
    manageDlg( ison );
}


static MultiID getMultiID( const SceneID& sceneid )
{
    // Create dummy multiid, I don't want to save these picks
    return MultiID( 9999, sceneid.asInt() );
}


void MeasureToolMan::sceneAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(SceneID,sceneid,cb);
    addScene( sceneid );
}


void MeasureToolMan::addScene( const SceneID& sceneid )
{
    RefMan<visSurvey::PickSetDisplay> psd = new visSurvey::PickSetDisplay();
    psd->allowDoubleClick( false );

    RefMan<Pick::Set> ps = new Pick::Set( "Measure picks" );
    ps->disp_.connect_ = Pick::Set::Disp::Open;
    ps->disp_.color_ = OD::Color( 255, 0, 0 );
    psd->setSet( ps.ptr() );
    psd->setSetMgr( &picksetmgr_ );
    picksetmgr_.set( getMultiID(sceneid), ps.ptr() );

    appl_.applMgr().visServer()->addObject( psd.ptr(), sceneid, false);
    sceneids_ += sceneid;
    displayids_ += psd->id();
}


void MeasureToolMan::sceneClosed( CallBacker* cb )
{
    mCBCapsuleUnpack(SceneID,sceneid,cb);
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( !displayids_.validIdx(sceneidx) )
	return;

    appl_.applMgr().visServer()->removeObject( displayids_[sceneidx], sceneid );
    displayids_.removeSingle( sceneidx );
    sceneids_.removeSingle( sceneidx );
}


SceneID MeasureToolMan::getActiveSceneID() const
{
    const SceneID sceneid = appl_.sceneMgr().getActiveSceneID();
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( sceneidx>=0 )
	return sceneid;

    if ( sceneids_.size() == 1 )
	return sceneids_.first();

    return SceneID::udf();
}


static void giveCoordsToDialog( const Pick::Set& set, uiMeasureDlg& dlg )
{
    TypeSet<Coord3> crds;
    set.getLocations( crds );
    dlg.fill( crds );
}


RefMan<visSurvey::PickSetDisplay> MeasureToolMan::getDisplayObj(
							const VisID& visid )
{
    if ( !appl_.applMgr().visServer() )
	return nullptr;

    visBase::DataObject* dataobj =
				appl_.applMgr().visServer()->getObject( visid );
    RefMan<visSurvey::PickSetDisplay> displayobj =
				dCast(visSurvey::PickSetDisplay*,dataobj);
    return displayobj;
}


ConstRefMan<visSurvey::PickSetDisplay> MeasureToolMan::getDisplayObj(
						const VisID& visid ) const
{
    return mSelf().getDisplayObj( visid );
}


RefMan<Pick::Set> MeasureToolMan::getSet( const VisID& visid )
{
    RefMan<visSurvey::PickSetDisplay> displayobj = getDisplayObj( visid );
    if ( !displayobj )
	return nullptr;

    return displayobj->getSet();
}


ConstRefMan<Pick::Set> MeasureToolMan::getSet( const VisID& visid ) const
{
    return mSelf().getSet( visid );
}


void MeasureToolMan::update()
{
    const SceneID sceneid = getActiveSceneID();
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( !displayids_.validIdx(sceneidx) )
	return;

    ConstRefMan<Pick::Set> ps = getSet( displayids_[sceneidx] );
    if ( ps && measuredlg_ )
	giveCoordsToDialog( *ps.ptr(), *measuredlg_ );
}


void MeasureToolMan::sceneChanged( CallBacker* )
{
    const bool ison = appl_.menuMgr().viewTB()->isOn( butidx_ );
    if ( !ison ) return;

    const SceneID sceneid = getActiveSceneID();
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( !displayids_.validIdx(sceneidx) )
	return;

    const VisID& displayid = displayids_[sceneidx];
    visBase::DM().selMan().select( displayid );
    RefMan<Pick::Set> ps = getSet( displayid );
    if ( ps && measuredlg_ )
	giveCoordsToDialog( *ps.ptr(), *measuredlg_ );

}


void MeasureToolMan::changeCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb);
    if ( !cd || !cd->set_ )
	return;

    if ( measuredlg_ )
	giveCoordsToDialog( *cd->set_, *measuredlg_ );
}


void MeasureToolMan::velocityChangeCB( CallBacker* )
{
    update();
}


void MeasureToolMan::clearCB( CallBacker* )
{
    for ( const auto& displayid : displayids_ )
    {
	RefMan<Pick::Set> ps = getSet( displayid );
	if ( !ps )
	    continue;

	ps->setEmpty();
	picksetmgr_.reportChange( this, *ps );
    }

    if ( measuredlg_ )
	measuredlg_->reset();
}


void MeasureToolMan::lineStyleChangeCB( CallBacker* )
{
    if ( !measuredlg_ )
	return;

    for ( const auto& displayid : displayids_ )
    {
	RefMan<Pick::Set> ps = getSet( displayid );
	if ( !ps )
	    continue;

	OD::LineStyle ls( measuredlg_->getLineStyle() );
	ps->disp_.color_ = ls.color_;
	ps->disp_.pixsize_ = ls.width_;
	picksetmgr_.reportDispChange( this, *ps );
    }
}


void MeasureToolMan::surveyChanged( CallBacker* )
{
    if ( measuredlg_ )
	measuredlg_->close();

    mAttachCBIfNotAttached( picksetmgr_.locationChanged,
			    MeasureToolMan::changeCB );
}
