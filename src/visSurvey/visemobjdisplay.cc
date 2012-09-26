/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visemobjdisplay.h"

#include "emmanager.h"
#include "emobject.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "undo.h"

#include "visdrawstyle.h"
#include "visevent.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vistransform.h"
#include "vistexturechannel2rgba.h"
#include "zaxistransform.h"



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
    , em_( EM::EMM() )
    , emobject_( 0 )
    , parmid_( -1 )
    , editor_( 0 )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , zaxistransform_( 0 )
    , displayonlyatsections_( false )
    , hasmoved( this )
    , changedisplay( this )
    , locknotifier( this )
    , drawstyle_( visBase::DrawStyle::create() )
    , nontexturecolisset_( false )
    , enableedit_( false )
    , restoresessupdate_( false )
    , burstalertison_( false )
    , channel2rgba_( 0 )
{
    parposattrshown_.erase();

    drawstyle_->ref();
    addChild( drawstyle_->getInventorNode() );

    LineStyle defls; defls.width_ = 2;
    drawstyle_->setLineStyle( defls );

    getMaterial()->setAmbience( 0.8 );
}


EMObjectDisplay::~EMObjectDisplay()
{
    if ( channel2rgba_ ) channel2rgba_->unRef();
    channel2rgba_ = 0;

    removeChild( drawstyle_->getInventorNode() );
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

    bool onobject = getSectionID(&eventinfo.pickedobjids)!=-1;

    if ( !onobject && editor_ )
	onobject = eventinfo.pickedobjids.indexOf( editor_->id() )!=-1;

    if  ( !onobject )
	return;

    bool keycb = false;
    bool mousecb = false;
    if ( eventinfo.type == visBase::Keyboard )
	keycb = eventinfo.key=='n' && eventinfo.pressed;
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
	BufferString str = "Section: "; str += closestnode.sectionID();
	str += " ("; str += closestrc.row;
	str += ","; str += closestrc.col; str += ",";
	const Coord3 pos = emobject_->getPos( closestnode );
	str += pos.z; str += ", "; str+= pos.y; str += ", "; str+= pos.z;
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
	editor_->unRef();
	editor_ = 0;
    }

    if ( emobject_ )
    {
	emobject_->change.remove( mCB(this,EMObjectDisplay,emChangeCB));
	const int trackeridx = MPE::engine().getTrackerByObject(emobject_->id());
	if ( trackeridx >= 0 )
	{
	    MPE::engine().removeTracker( trackeridx );
	    MPE::engine().removeEditor(emobject_->id());
	}

	emobject_->unRef();
	emobject_ = 0;
    }
}


EM::PosID EMObjectDisplay::findClosestNode(const Coord3&) const
{ return EM::PosID(-1,-1,-1); }


bool EMObjectDisplay::setEMObject( const EM::ObjectID& newid, TaskRunner* tr )
{
    EM::EMObject* emobject = em_.getObject( newid );
    if ( !emobject ) return false;

    removeEMStuff();

    emobject_ = emobject;
    emobject_->ref();
    emobject_->change.notify( mCB(this,EMObjectDisplay,emChangeCB) );

    if ( nontexturecolisset_ )
    	emobject_->setPreferredColor( nontexturecol_ );

    restoresessupdate_ = !editor_ && parmid_!=MultiID(-1);
    bool res = updateFromEM( tr );
    restoresessupdate_ = false;

    return res;
}


EM::ObjectID EMObjectDisplay::getObjectID() const
{
    return emobject_ ? emobject_->id() : -1;
}


MultiID EMObjectDisplay::getMultiID() const
{
    if ( !emobject_ ) return parmid_;

    return emobject_->multiID();
}


BufferStringSet EMObjectDisplay::displayedSections() const
{
    if ( !emobject_ )
	return parsections_;
   
    BufferStringSet res; 
    for ( int idx=emobject_->nrSections()-1; idx>=0; idx-- )
    {
	mDeclareAndTryAlloc( BufferString*, buf,
	    BufferString(emobject_->sectionName(emobject_->sectionID(idx))) );
	res += buf;
    }

    return res;
}


bool EMObjectDisplay::updateFromEM( TaskRunner* tr )
{ 
    if ( !emobject_ ) return false;

    setName( emobject_->name() );

    for ( int idx=0; idx<emobject_->nrSections(); idx++ )
	addSection( emobject_->sectionID(idx), tr );

    updateFromMPE();

    if ( !nontexturecolisset_ )
    {
	nontexturecol_ = emobject_->preferredColor();
	nontexturecolisset_ = true;
    }

    while ( parposattrshown_.size() )
    {
	showPosAttrib( parposattrshown_[0], true );
	parposattrshown_.remove(0);
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
	showPosAttrib( EM::EMObject::sSeedNode(), true );
	enableedit_ = true;
    }

    enableEditing( enableedit_ );
}


void EMObjectDisplay::showPosAttrib( int attr, bool yn )
{
    int attribindex = posattribs_.indexOf(attr);
    if ( yn )
    {
	if ( attribindex==-1 )
	{
	    posattribs_ += attr;
	    visBase::DataObjectGroup* group= visBase::DataObjectGroup::create();
	    group->ref();
	    addChild( group->getInventorNode() );
	    posattribmarkers_ += group;

	    group->addObject( visBase::Material::create() );
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
	removeChild(posattribmarkers_[attribindex]->getInventorNode());
	posattribmarkers_[attribindex]->unRef();
	posattribmarkers_.remove(attribindex);
    }
}


bool EMObjectDisplay::showsPosAttrib(int attr) const
{ return posattribs_.indexOf(attr)!=-1; }


const LineStyle* EMObjectDisplay::lineStyle() const
{
    return &drawstyle_->lineStyle();
}


void EMObjectDisplay::setLineStyle( const LineStyle& ls )
{ drawstyle_->setLineStyle(ls); }


void EMObjectDisplay::getLineWidthBounds( int& min, int& max )
{
    drawstyle_->getLineWidthBounds( min, max );
}


bool EMObjectDisplay::hasColor() const
{
    return true;
}


void EMObjectDisplay::setOnlyAtSectionsDisplay(bool yn)
{
    displayonlyatsections_ = yn;

    hasmoved.trigger();
    changedisplay.trigger();
}


bool EMObjectDisplay::getOnlyAtSectionsDisplay() const
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
	    			MPE::engine().getEditor(getObjectID(),true);

	if ( !mpeeditor ) return;

	editor_ = MPEEditor::create();
	editor_->ref();
	editor_->setSceneEventCatcher( eventcatcher_ );
	editor_->setDisplayTransformation( transformation_ );
	editor_->sower().intersow();
	editor_->sower().reverseSowingOrder();
	editor_->setEditor(mpeeditor);
	addChild( editor_->getInventorNode() );
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
    bool triggermovement = false;


    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event==EM::EMObjectCallbackData::SectionChange )
    {
	const EM::SectionID sectionid = cbdata.pid0.sectionID();
	if ( emobject_->sectionIndex(sectionid)>=0 )
	    addSection( sectionid, 0 );
	else
	{
	    removeSectionDisplay(sectionid);
	    hasmoved.trigger();
	}

	triggermovement = true;
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::BurstAlert)
    {
	burstalertison_ = !burstalertison_;
	if ( !burstalertison_ )
	{
	    for ( int idx=0; idx<posattribs_.size(); idx++ ) 
		updatePosAttrib(posattribs_[idx]); 

	    triggermovement = true;
	}

    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	if ( !burstalertison_ )
	{
	    for ( int idx=0; idx<posattribs_.size(); idx++ ) 
	    {
		const TypeSet<EM::PosID>* pids = 
			emobject_->getPosAttribList(posattribs_[idx]); 
		if ( !pids || pids->indexOf(cbdata.pid0)==-1 ) 
		    continue; 

		updatePosAttrib(posattribs_[idx]); 
	    }
	    triggermovement = true;
	}
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::AttribChange )
    {
	if ( !burstalertison_ && posattribs_.indexOf(cbdata.attrib)!=-1 )
	{
	    updatePosAttrib(cbdata.attrib);
	    if ( displayonlyatsections_ )
		triggermovement = true;
	}
    }

    if ( triggermovement )
	hasmoved.trigger();
}


void EMObjectDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				       Coord3& pos,
				       BufferString& val,
				       BufferString& info ) const
{
    info = ""; val = "";
    if ( !emobject_ ) return;

    info = emobject_->getTypeStr(); info += ": "; info += name();
	                    
    const EM::SectionID sid = getSectionID(&eventinfo.pickedobjids);

    if ( sid==-1 ) return;

    BufferString sectionname = emobject_->sectionName(sid);
    if ( sectionname.isEmpty() ) sectionname = sid;
    info += ", Section: "; info += sectionname;
}


void EMObjectDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    if ( emobject_ && !emobject_->isFullyLoaded() )
	par.set( sKeySections(), displayedSections() );

    par.set( sKeyEarthModelID(), getMultiID() );
    par.setYN( sKeyEdit(), isEditingEnabled() );
    par.setYN( sKeyOnlyAtSections(), getOnlyAtSectionsDisplay() );
    par.set( sKey::Color(), (int)getColor().rgb() );

    if ( lineStyle() )
    {
	BufferString str;
	lineStyle()->toString( str );
	par.set( sKeyLineStyle(), str );
    }

    par.set( sKeyPosAttrShown(), posattribs_ );
    fillSOPar( par, saveids );
}


int EMObjectDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    if ( !par.get(sKeyEarthModelID(),parmid_) )
	return -1;

    PtrMan<IOObj> ioobj = IOM().get( parmid_ );
    if ( !ioobj )
    {
	errmsg_ = "Cannot locate object ";
	errmsg_ += name();
	errmsg_ += " (";
	errmsg_ += parmid_;
	errmsg_ += ")";
	return -1;
    }

    if ( scene_ )
	setDisplayTransformation( scene_->getUTM2DisplayTransform() );

    par.get( sKeySections(), parsections_ );
    BufferString linestyle;
    if ( par.get(sKeyLineStyle(),linestyle) )
    {
	LineStyle ls;
	ls.fromString( linestyle );
	setLineStyle( ls );
    }

    par.getYN( sKeyEdit(), enableedit_ );

    nontexturecolisset_ = par.get(sKey::Color(),(int&)nontexturecol_.rgb() );

    bool filter = false;
    par.getYN( sKeyOnlyAtSections(), filter );
    setOnlyAtSectionsDisplay(filter);

    par.get( sKeyPosAttrShown(), parposattrshown_ );

    return useSOPar( par );
}


void EMObjectDisplay::lock( bool yn ) 
{
    locked_ = yn;
    if ( emobject_ ) emobject_->lock(yn);
    locknotifier.trigger();
}


void EMObjectDisplay::updatePosAttrib( int attrib )
{
    const int attribindex = posattribs_.indexOf(attrib);
    if ( attribindex==-1 ) return;

    mDynamicCastGet(visBase::Material*, posattrmat,
		    posattribmarkers_[attribindex]->getObject(0) );
    posattrmat->setColor( emobject_->getPosAttrMarkerStyle(attrib).color_ );

    int markeridx = 1;		// element 0 contains material, start at 1

    const TypeSet<EM::PosID>* pids = emobject_->getPosAttribList(attrib);

    for ( int idx=0; pids && idx<pids->size(); idx++ )
    {
	const Coord3 pos = emobject_->getPos((*pids)[idx]);
	if ( !pos.isDefined() ) continue;

	visBase::Marker* marker;
	if ( markeridx < posattribmarkers_[attribindex]->size() )
	{
	    mDynamicCast( visBase::Marker*, marker,
			  posattribmarkers_[attribindex]->getObject(markeridx));
	}
	else
	{
	    marker = visBase::Marker::create();
	    posattribmarkers_[attribindex]->addObject(marker);
	    marker->setMaterial(0);
	}

	marker->setMarkerStyle( emobject_->getPosAttrMarkerStyle(attrib) );
	marker->setDisplayTransformation(transformation_);
	marker->setCenterPos(pos);
	markeridx++;
    }

    while ( posattribmarkers_[attribindex]->size() > markeridx  )
	posattribmarkers_[attribindex]->removeObject( markeridx );
}


EM::PosID EMObjectDisplay::getPosAttribPosID( int attrib,
					      const TypeSet<int>& path ) const
{
    EM::PosID res(-1,-1,-1);
    const int attribidx = posattribs_.indexOf(attrib);
    if ( attribidx<0 )
	return res;

    const visBase::DataObjectGroup& group = *posattribmarkers_[attribidx];
    TypeSet<int> visids;
    for ( int idx=1; idx<group.size(); idx++ )
	visids += group.getObject( idx )->id();

    for ( int idx=0; idx<path.size(); idx++ )
    {
	const int index = visids.indexOf( path[idx] );
	if ( index==-1 )
	    continue;

	const TypeSet<EM::PosID>* pids = emobject_->getPosAttribList(attrib);
	if ( index < pids->size() )
	    res = (*pids)[index];

	break;
    }

    return res;
}


void EMObjectDisplay::removeSelection( const Selector<Coord3>& selector,
	TaskRunner* tr)
{
    const int lastid = EM::EMM().undo().currentEventID();
    em_.removeSelected( emobject_->id(), selector, tr );
    if ( lastid!=EM::EMM().undo().currentEventID() )
    {
	EM::EMM().undo().setUserInteractionEnd(
					EM::EMM().undo().currentEventID() );
    }
}


}; // namespace visSurvey
