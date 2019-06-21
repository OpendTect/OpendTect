/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/

#include "visemobjdisplay.h"

#include "emmanager.h"
#include "iopar.h"
#include "ioobj.h"
#include "keystrs.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "undo.h"
#include "callback.h"
#include "polygon.h"
#include "survinfo.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"

#include "visdrawstyle.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vistransform.h"
#include "vistexturechannel2rgba.h"
#include "vispolygonselection.h"
#include "zaxistransform.h"
#include "uistrings.h"


#define mSelColor Color( 0, 255, 0 )
#define mDefaultSize 4

namespace visSurvey
{

const char* EMObjectDisplay::sKeyEarthModelID()  { return "EarthModel ID"; }
const char* EMObjectDisplay::sKeyEdit()		 { return "Edit"; }
const char* EMObjectDisplay::sKeyOnlyAtSections()
			    { return "Display only on sections"; }
const char* EMObjectDisplay::sKeyLineStyle()	{ return "Linestyle"; }
const char* EMObjectDisplay::sKeySections()	{ return "Displayed Sections"; }
const char* EMObjectDisplay::sKeyPosAttrShown() { return "Pos Attribs shown"; }


EMObjectDisplay::EMObjectDisplay()
    : VisualObjectImpl(true)
    , emmgr_( EM::MGR() )
    , emobject_( 0 )
    , parmid_( DBKey::getInvalid() )
    , editor_( 0 )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , zaxistransform_( 0 )
    , displayonlyatsections_( false )
    , hasmoved( this )
    , changedisplay( this )
    , locknotifier( this )
    , drawstyle_( new visBase::DrawStyle )
    , nontexturecolisset_( false )
    , enableedit_( false )
    , restoresessupdate_( false )
    , burstalertison_( false )
    , channel2rgba_( 0 )
    , ctrldown_( false )
{
    parposattrshown_.erase();

    drawstyle_->ref();
    addNodeState( drawstyle_ );

    OD::LineStyle defls; defls.width_ = 4;
    drawstyle_->setLineStyle( defls );

    getMaterial()->setAmbience( 0.8 );
}


EMObjectDisplay::~EMObjectDisplay()
{
    turnOnSelectionMode( false );

    if ( channel2rgba_ ) channel2rgba_->unRef();
    channel2rgba_ = 0;

    removeNodeState( drawstyle_ );
    drawstyle_->unRef();
    drawstyle_ = 0;

    if ( transformation_ ) transformation_->unRef();

    setSceneEventCatcher( 0 );


    if ( posattribs_.size() || editor_ || emobject_ )
    {
	pErrMsg("You have not called removeEMStuff from"
		"inheriting object's constructor." );
	removeEMStuff(); //Lets hope for the best.
    }

    for ( int idx= 0; idx < posattribmarkers_.size(); idx++ )
    {
	removeChild( posattribmarkers_[idx]->osgNode() );
	posattribmarkers_.removeSingle(idx)->unRef();
    }

    clearSelections();

    emchangedata_.clearData();
}


bool EMObjectDisplay::setChannels2RGBA( visBase::TextureChannel2RGBA* t )
{
    if ( channel2rgba_ ) channel2rgba_->unRef();
    channel2rgba_ = t;
    if ( channel2rgba_ ) channel2rgba_->ref();

    return true;
}


visBase::TextureChannel2RGBA* EMObjectDisplay::getChannels2RGBA()
{ return channel2rgba_; }


const mVisTrans* EMObjectDisplay::getDisplayTransformation() const
{ return transformation_; }


void EMObjectDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ ) transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ ) transformation_->ref();

    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
	posattribmarkers_[idx]->setDisplayTransformation(transformation_);

    if ( editor_ ) editor_->setDisplayTransformation(transformation_);
}


void EMObjectDisplay::setSceneEventCatcher( visBase::EventCatcher* ec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove( mCB(this,EMObjectDisplay,clickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = ec;
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify( mCB(this,EMObjectDisplay,clickCB));
	eventcatcher_->ref();
    }

    if ( editor_ ) editor_->setSceneEventCatcher( ec );
}


void EMObjectDisplay::clickCB( CallBacker* cb )
{
    if ( !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    if ( editor_ && !editor_->clickCB( cb ) )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    ctrldown_ = OD::ctrlKeyboardButton( eventinfo.buttonstate_ );

    bool onobject = getSectionID(&eventinfo.pickedobjids)!=-1;

    if ( !onobject && editor_ )
	onobject = eventinfo.pickedobjids.isPresent( editor_->id() );

    if  ( !onobject )
	return;

    bool keycb = false;
    bool mousecb = false;
    if ( eventinfo.type == visBase::Keyboard )
	keycb = eventinfo.key_==OD::KB_N && eventinfo.pressed;
    else if ( eventinfo.type == visBase::MouseClick )
    {
	mousecb =
	    eventinfo.pressed && OD::leftMouseButton( eventinfo.buttonstate_ );
    }

    if ( !keycb && !mousecb ) return;

    EM::PosID closestnode = findClosestNode( eventinfo.displaypickedpos );
    if ( mousecb && editor_ )
    {
	bool handled = editor_->mouseClick( closestnode,
	    OD::shiftKeyboardButton( eventinfo.buttonstate_ ),
	    OD::altKeyboardButton( eventinfo.buttonstate_ ),
	    OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) );
	if ( handled )
	    eventcatcher_->setHandled();
    }
    else if ( keycb )
    {
	const RowCol closestrc = closestnode.getRowCol();
	BufferString str = "Section: "; str += 0;
	str += " ("; str += closestrc.row();
	str += ","; str += closestrc.col(); str += ",";
	const Coord3 pos = emobject_->getPos( closestnode );
	str += pos.x_; str += ", "; str+= pos.y_; str += ", "; str+= pos.z_;
	str+=")";
	pErrMsg(str);
    }
}


void EMObjectDisplay::removeEMStuff()
{
    while ( posattribs_.size() )
	showPosAttrib( posattribs_[0], false );

    if ( editor_ )
    {
	removeChild( editor_->osgNode() );
	editor_->unRef();
	editor_ = 0;
    }

    if ( emobject_ )
    {
	emobject_->objectChanged().remove(
		mCB(this,EMObjectDisplay,emChangeCB));
	const int trackeridx =
	    MPE::engine().getTrackerByObject(emobject_->id());
	if ( trackeridx >= 0 )
	    MPE::engine().removeEditor(emobject_->id());

	emobject_->unRef();
	emobject_ = 0;
    }
}


EM::PosID EMObjectDisplay::findClosestNode(const Coord3&) const
{ return EM::PosID::getInvalid(); }


bool EMObjectDisplay::setEMObject( const DBKey& newid, TaskRunner* tskr )
{
    EM::Object* emobject = emmgr_.getObject( newid );
    if ( !emobject ) return false;

    removeEMStuff();

    emobject_ = emobject;
    emobject_->ref();
    emobject_->objectChanged().notify( mCB(this,EMObjectDisplay,emChangeCB) );

    if ( nontexturecolisset_ )
	emobject_->setPreferredColor( nontexturecol_ );

    restoresessupdate_ = !editor_ && parmid_.isValid();
    bool res = updateFromEM( tskr );
    restoresessupdate_ = false;

    mDynamicCastGet( const EM::Horizon*, hor, emobject_ );
    if ( hor && editor_ )
    {
	editor_->sower().setSequentSowMask(
	    true, OD::ButtonState(OD::LeftButton+OD::ControlButton) );
    }


    return res;
}


DBKey EMObjectDisplay::getObjectID() const
{
    return emobject_ ? emobject_->id() : DBKey::getInvalid();
}


DBKey EMObjectDisplay::getDBKey() const
{
    if ( !emobject_ ) return parmid_;

    return emobject_->dbKey();
}


BufferStringSet EMObjectDisplay::displayedSections() const
{
    return parsections_;
}


bool EMObjectDisplay::updateFromEM( TaskRunner* tskr )
{
    if ( !emobject_ )
	return false;

    setName( emobject_->name() );

    if ( !addSection( 0, tskr ) )
	return false;

    updateFromMPE();

    if ( !nontexturecolisset_ )
    {
	nontexturecol_ = emobject_->preferredColor();
	nontexturecolisset_ = true;
    }

    while ( parposattrshown_.size() )
    {
	showPosAttrib( parposattrshown_[0], true );
	parposattrshown_.removeSingle(0);
    }

    hasmoved.trigger();
    return true;
}


void EMObjectDisplay::updateFromMPE()
{
    const bool hastracker =
	MPE::engine().getTrackerByObject(getObjectID()) >= 0;
    if ( hastracker && !restoresessupdate_ )
    {
	setResolution( 0, 0 );
	showPosAttrib( EM::Object::sSeedNode(), true );
	showPosAttrib( EM::Object::sTerminationNode(), true );
	enableedit_ = true;
    }

    enableEditing( enableedit_ );
}


void EMObjectDisplay::showPosAttrib( int attr, bool yn )
{
    int attribindex = posattribs_.indexOf( attr );
    if ( yn )
    {
	if ( attribindex==-1 )
	{
	    posattribs_ += attr;
	    visBase::MarkerSet* markerset = visBase::MarkerSet::create();
	    markerset->ref();
	    Color clr = attr == EM::Object::sSeedNode()
				? Color::White()
				: Color::Green();
	    markerset->setMarkersSingleColor( clr );
	    addChild( markerset->osgNode() );
	    posattribmarkers_ += markerset;
	    markerset->setMaterial( 0 );
	    markerset->setScreenSize( mDefaultSize );
	    attribindex = posattribs_.size()-1;
	}

	updatePosAttrib(attr);

	if ( displayonlyatsections_ )
	{
	    setOnlyAtSectionsDisplay(false);
	    setOnlyAtSectionsDisplay(true);
	}
    }
    else if ( attribindex!=-1 && !yn )
    {
	posattribs_ -= attr;
	removeChild(posattribmarkers_[attribindex]->osgNode());
	posattribmarkers_.removeSingle(attribindex)->unRef();
    }
}


bool EMObjectDisplay::showsPosAttrib(int attr) const
{ return posattribs_.isPresent(attr); }


const OD::LineStyle* EMObjectDisplay::lineStyle() const
{
    return &drawstyle_->lineStyle();
}


void EMObjectDisplay::setLineStyle( const OD::LineStyle& ls )
{
    if ( emobject_ )
	emobject_->setPreferredLineStyle( ls );
    drawstyle_->setLineStyle( ls );
}


bool EMObjectDisplay::hasColor() const
{
    return true;
}


void EMObjectDisplay::setOnlyAtSectionsDisplay( bool yn )
{
    displayonlyatsections_ = yn;

    hasmoved.trigger();
    changedisplay.trigger();
}


bool EMObjectDisplay::displayedOnlyAtSections() const
{ return displayonlyatsections_; }


void EMObjectDisplay::setColor( Color col )
{
    if ( emobject_ )
    {
	emobject_->setPreferredColor( col );
    }
    else
    {
	nontexturecol_ = col;
	nontexturecolisset_ = true;
    }

    changedisplay.trigger();
}


Color EMObjectDisplay::getColor() const
{
    if ( emobject_ )
	return emobject_->preferredColor();

    if ( !nontexturecolisset_ )
    {
	nontexturecol_ = getRandStdDrawColor();
	nontexturecolisset_ = true;
    }

    return nontexturecol_;
}


MPEEditor* EMObjectDisplay::getEditor() { return editor_; }


void EMObjectDisplay::enableEditing( bool yn )
{
    if ( yn && !editor_ )
    {
	MPE::ObjectEditor* mpeeditor =
			MPE::engine().getEditor( getObjectID(), true );

	if ( !mpeeditor ) return;

	editor_ = MPEEditor::create();
	editor_->ref();
	editor_->setSceneEventCatcher( eventcatcher_ );
	editor_->setDisplayTransformation( transformation_ );
	editor_->sower().intersow();
	editor_->sower().reverseSowingOrder();
	editor_->setEditor(mpeeditor);
	addChild( editor_->osgNode() );
    }

    if ( editor_ ) editor_->turnOn(yn);
}


bool EMObjectDisplay::isEditingEnabled() const
{ return editor_ && editor_->isOn(); }


EM::SectionID EMObjectDisplay::getSectionID( const TypeSet<int>* path ) const
{
    for ( int idx=0; path && idx<path->size(); idx++ )
    {
	const EM::SectionID sectionid = getSectionID((*path)[idx]);
	if ( sectionid!=-1 ) return sectionid;
    }

    return -1;
}


void EMObjectDisplay::emChangeCB( CallBacker* cb )
{
   if ( cb )
    {
	mCBCapsuleUnpack( EM::ObjectCallbackData, cbdata, cb );
	emchangedata_.addCallBackData( &cbdata );
    }

    mEnsureExecutedInMainThread( EMObjectDisplay::emChangeCB );

    for ( int idx=0; idx<emchangedata_.size(); idx++ )
    {
	const EM::ObjectCallbackData* cbdata =
	    emchangedata_.getCallBackData( idx );
	if ( !cbdata )
	    continue;
	handleEmChange( *cbdata );
    }
    emchangedata_.clearData();
}


void EMObjectDisplay::getObjectInfo( BufferString& info ) const
{
    info.setEmpty();
    if ( !emobject_ )
	return;

    info = emobject_->getTypeStr();
    info.add( ": " ).add( getName() );
}


void EMObjectDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				       Coord3& pos,
				       BufferString& val,
				       BufferString& info ) const
{
    info.setEmpty(); val.setEmpty();
    if ( !emobject_ )
	return;

    info = emobject_->getTypeStr(); info += ": ";
    info += mFromUiStringTodo(name());
}


void EMObjectDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );

    if ( emobject_ && !emobject_->isFullyLoaded() )
	par.set( sKeySections(), displayedSections() );

    par.set( sKeyEarthModelID(), getDBKey() );
    par.setYN( sKeyEdit(), isEditingEnabled() );
    par.setYN( sKeyOnlyAtSections(), displayedOnlyAtSections() );
    par.set( sKey::Color(), (int)getColor().rgb() );

    if ( lineStyle() )
    {
	BufferString str;
	lineStyle()->toString( str );
	par.set( sKeyLineStyle(), str );
    }

    par.set( sKeyPosAttrShown(), posattribs_ );
}


bool EMObjectDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) ||
	 !par.get(sKeyEarthModelID(),parmid_) )

	 return false;

    PtrMan<IOObj> ioobj = parmid_.getIOObj();
    if ( !ioobj )
    {
	errmsg_ = uiStrings::phrCannotFind(tr("object %1 (%2)").arg(name())
								.arg(parmid_));
	return -1;
    }

    if ( scene_ )
	setDisplayTransformation( scene_->getUTM2DisplayTransform() );

    par.get( sKeySections(), parsections_ );
    BufferString linestyle;
    if ( par.get(sKeyLineStyle(),linestyle) )
    {
	OD::LineStyle ls;
	ls.fromString( linestyle );
	setLineStyle( ls );
    }

    par.getYN( sKeyEdit(), enableedit_ );

    nontexturecolisset_ = par.get(sKey::Color(),(int&)nontexturecol_.rgb() );

    bool filter = false;
    par.getYN( sKeyOnlyAtSections(), filter );
    setOnlyAtSectionsDisplay(filter);

    par.get( sKeyPosAttrShown(), parposattrshown_ );

    return true;
}


void EMObjectDisplay::lock( bool yn )
{
    locked_ = yn;
    if ( emobject_ ) emobject_->lock(yn);
    locknotifier.trigger();
}


void EMObjectDisplay::updatePosAttrib( int attrib )
{
    const int attribindex = posattribs_.indexOf( attrib );
    if ( attribindex==-1 ) return;

    const TypeSet<EM::PosID>* pids = emobject_->getPosAttribList( attrib );
    if ( !pids ) return;

    visBase::MarkerSet* markerset = posattribmarkers_[attribindex];

    markerset->clearMarkers();
    markerset->setMarkerStyle( emobject_->getPosAttrMarkerStyle(attrib) );
    markerset->setDisplayTransformation(transformation_);

    mDynamicCastGet( EM::Horizon3D*, hor3d, emobject_ );
    for ( int idx=0; idx<pids->size(); idx++ )
    {
	const Coord3 pos = emobject_->getPos( (*pids)[idx] );
	if ( !pos.isDefined() )
	    continue;
	const int mkpos = markerset->addPos( pos, false );
	Color clr = emobject_->getPosAttrMarkerStyle(attrib).color_;

	if ( hor3d )
	{
	    const BinID pickedbid = SI().transform( pos.getXY() );
	    if ( hor3d->isNodeLocked(TrcKey(pickedbid)) )
		 clr = hor3d->getLockColor();
	}
	markerset->getMaterial()->setColor( clr, mkpos );
    }

    markerset->turnAllMarkersOn( true );
    markerset->forceRedraw( true );
    markerset->getCoordinates()->removeAfter( pids->size()-1 );

    if ( attrib==EM::Object::sSeedNode() && getEditor() )
	getEditor()->setMarkerSize( markerset->getScreenSize() );
}


const visBase::MarkerSet* EMObjectDisplay::getSeedMarkerSet() const
{
    const int attribindex = posattribs_.indexOf( EM::Object::sSeedNode() );
    if ( attribindex==-1 || posattribmarkers_.size()<attribindex )
	return 0;

    return posattribmarkers_[attribindex];
}


EM::PosID EMObjectDisplay::getPosAttribPosID( int attrib,
    const TypeSet<int>& path, const Coord3& clickeddisplaypos ) const
{
    EM::PosID res = EM::PosID::getInvalid();
    const int attribidx = posattribs_.indexOf(attrib);
    if ( attribidx<0 )
	return res;

    const visBase::MarkerSet* markerset = posattribmarkers_[attribidx];
    if ( !path.isPresent(markerset->id()) )
	return res;

    double minsqdist = mUdf(float);
    int minidx = -1;

    const TypeSet<EM::PosID>* pids = emobject_->getPosAttribList( attrib );

    for ( int idx=0; idx<pids->size(); idx++ )
    {
	Coord3 nodecrd = emobject_->getPos( (*pids)[idx] );
	if ( transformation_ )
	    transformation_->transform( nodecrd );

	const double sqdist = clickeddisplaypos.sqDistTo( nodecrd );
	if ( sqdist<minsqdist )
	{
	    minsqdist = sqdist;
	    minidx = idx;
	}
    }

    if ( minidx !=-1 )
	res = (*pids)[minidx];

    return res;
}


bool EMObjectDisplay::removeSelections( TaskRunner* taskr )
{
    if ( selectionids_.isEmpty() )
	return false;

    Undo& undo = emmgr_.undo( emobject_->id() );
    const int lastid = undo.currentEventID();

    emobject_->removePositions( selectionids_ );

    if ( lastid!=undo.currentEventID() )
	undo.setUserInteractionEnd( undo.currentEventID() );

    updateAuxData();
    clearSelections();
    return true;
}


void EMObjectDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );
    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
	posattribmarkers_[idx]->setPixelDensity( dpi );

}


void EMObjectDisplay::turnOnSelectionMode( bool yn )
{
    ctrldown_ = false;

    if ( scene_ && scene_->getPolySelection() )
    {
	if ( yn )
	    mAttachCBIfNotAttached(
	    scene_->getPolySelection()->polygonFinished(),
	    EMObjectDisplay::polygonFinishedCB );
	else
	{
	    mDetachCB(
	    scene_->getPolySelection()->polygonFinished(),
	    EMObjectDisplay::polygonFinishedCB );
	    unSelectAll();
	}
    }

}


void EMObjectDisplay::polygonFinishedCB( CallBacker* cb )
{
      if ( !scene_ || ! scene_->getPolySelection() )
	  return;

    visBase::PolygonSelection* polysel =  scene_->getPolySelection();

    if ( (!polysel->hasPolygon() && !polysel->singleSelection()) )
    {
	unSelectAll();
	return;
    }

    if ( !ctrldown_ )
	unSelectAll();

    UserShowWait usw( this, uiStrings::sUpdatingDisplay() );
    updateSelections();
    polysel->clear();
}



void EMObjectDisplay::updateSelections()
{
    const Selector<Coord3>* selector = scene_->getSelector();
    if ( !selector )
	return;

    mDynamicCastGet( EM::Horizon2D*, hor2d, emobject_ );
    mDynamicCastGet( EM::Horizon3D*, hor3d, emobject_ );

    Color selectioncolor =  Color::Orange();
    if ( hor2d || hor3d )
	selectioncolor = hor3d ? hor3d->selectionColor()
				: hor2d->selectionColor();

    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
    {
	for ( int idy=0; idy<posattribmarkers_[idx]->size(); idy++ )
	{
	    visBase::MarkerSet* markerset = posattribmarkers_[idx];
	    if ( !markerset )
		continue;
	    markerset->getMaterial()->setColor( Color::White(),idy );

	    const visBase::Coordinates* coords = markerset->getCoordinates();
	    if ( !coords )
		continue;
	    const Coord3 pos = coords->getPos( idy );
	    const BinID pickedbid = SI().transform( pos.getXY() );
	    const bool lockednode = !pickedbid.isUdf() && hor3d &&
		hor3d->isNodeLocked( TrcKey(pickedbid) );

	    if ( selector->includes(pos) && !lockednode )
	    {
		markerset->getMaterial()->setColor(
		selectioncolor, idy );
	    }
	    else if ( lockednode && hor3d )
	    {
		markerset->getMaterial()->setColor(
		    hor3d->getLockColor(),idy);
	    }
	}
    }
}


void EMObjectDisplay::clearSelections()
{
   deepErase( selectors_ );
   selectionids_.setEmpty();
}


void EMObjectDisplay::unSelectAll()
{
    for ( int idx=0; idx<posattribmarkers_.size(); idx++ )
	posattribmarkers_[idx]->setMarkersSingleColor( Color::White() );

    clearSelections();
    // if there are locked seeds, we need recover their color.
    updateLockedSeedsColor();
}


void EMObjectDisplay::handleEmChange( const EM::ObjectCallbackData& cbdata )
{
    bool triggermovement = false;
    ConstRefMan<EM::ChangeAuxData> cbaux =
				cbdata.auxDataAs<EM::ChangeAuxData>();
    if ( cbdata.changeType()==EM::Object::cBurstAlert())
    {
	burstalertison_ = !burstalertison_;
	if ( !burstalertison_ )
	{
	    while ( !addsectionids_.isEmpty() )
	    {
		addSection( addsectionids_[0], 0 );
		addsectionids_.removeSingle( 0 );
	    }

	    for ( int idx=0; idx<posattribs_.size(); idx++ )
		updatePosAttrib(posattribs_[idx]);

	    triggermovement = true;
	}

    }
    else if ( cbdata.changeType()==EM::Object::cPositionChange() && cbaux )
    {
	if ( !burstalertison_ )
	{
	    for ( int idx=0; idx<posattribs_.size(); idx++ )
	    {
		const TypeSet<EM::PosID>* pids =
			emobject_->getPosAttribList(posattribs_[idx]);
		if ( !pids || !pids->isPresent(cbaux->pid0) )
		    continue;

		updatePosAttrib(posattribs_[idx]);
	    }
	    triggermovement = true;
	}
    }
    else if ( cbdata.changeType()==EM::Object::cAttribChange() && cbaux )
    {
	if ( !burstalertison_ && posattribs_.isPresent(cbaux->attrib) )
	    updatePosAttrib(cbaux->attrib);
    }
    else if ( cbdata.changeType()==EM::Object::cLockChange() )
    {
	mDynamicCastGet( EM::Horizon3D*, hor3d, emobject_ );
	if ( hor3d )
	{
	    for ( int idx = 0; idx<posattribs_.size(); idx++ )
		updatePosAttrib( posattribs_[idx] );
	}
    }
    else if ( cbdata.changeType()==EM::Object::cLockColorChange() )
    {
	updateLockedSeedsColor();
    }

    if ( displayonlyatsections_ )
	setOnlyAtSectionsDisplay( true );
    else if ( triggermovement )
	hasmoved.trigger();
}


void EMObjectDisplay::updateLockedSeedsColor()
{
    const int attribindex=posattribs_.indexOf( EM::Object::sSeedNode() );
    if ( attribindex==-1 ) return;

    visBase::MarkerSet* markerset = posattribmarkers_[attribindex];
    if ( !markerset ) return;

    const Color seedclr = emobject_->getPosAttrMarkerStyle(attribindex).color_;

    const visBase::Coordinates* coords = markerset->getCoordinates();
    mDynamicCastGet( EM::Horizon3D*, hor3d, emobject_ );
    if ( !coords || !hor3d ) return;
    for ( int idx=0; idx<coords->size(); idx++ )
    {
	const BinID pickedbid = SI().transform( coords->getPos(idx).getXY() );
	if ( pickedbid.isUdf() )
		continue;
	if ( hor3d->isNodeLocked(TrcKey(pickedbid)) )
	    markerset->getMaterial()->setColor( hor3d->getLockColor(), idx );
	else
	    markerset->getMaterial()->setColor( seedclr, idx );
    }
    markerset->forceRedraw( true );
}


} // namespace visSurvey
