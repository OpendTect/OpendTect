/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismpeeditor.cc,v 1.6 2005-02-08 08:41:57 kristofer Exp $";

#include "vismpeeditor.h"

#include "errh.h"
#include "geeditor.h"
#include "emeditor.h"
#include "emobject.h"
#include "vismarker.h"
#include "visdragger.h"
#include "visevent.h"
#include "visshapescale.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::MPEEditor );


namespace visSurvey
{

MPEEditor::MPEEditor()
    : geeditor( 0 )
    , noderightclick( this )
    , emeditor( 0 )
    , translationdragger( 0 )
    , eventcatcher( 0 )
    , transformation( 0 )
    , activenode( -1, -1, -1 )
    , issettingpos( 0 )
{ }


MPEEditor::~MPEEditor()
{
    setEditor( (Geometry::ElementEditor*) 0 );
    setEditor( (MPE::ObjectEditor*) 0 );
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );

    if ( translationdragger )
    {
	translationdragger->started.remove(mCB(this,MPEEditor,dragStart));
	translationdragger->motion.remove(mCB(this,MPEEditor,dragMotion));
	translationdragger->finished.remove(mCB(this,MPEEditor,dragStop));
	translationdragger->unRef();
	translationdragger = 0;
    }

}


void MPEEditor::setEditor( Geometry::ElementEditor* ge )
{
    if ( ge ) setEditor( (MPE::ObjectEditor*) 0 );
    CallBack movementcb( mCB(this,MPEEditor,nodeMovement) );
    CallBack numnodescb( mCB(this,MPEEditor,changeNumNodes) );
    if ( geeditor )
    {
	geeditor->getElement().movementnotifier.remove( movementcb );
	geeditor->editpositionchange.remove( numnodescb );
    }

    geeditor = ge;

    if ( geeditor )
    {
	geeditor->getElement().movementnotifier.notify( movementcb );
	geeditor->editpositionchange.notify( numnodescb );
	changeNumNodes(0);
    }
}


void MPEEditor::setEditor( MPE::ObjectEditor* eme )
{
    if ( eme ) setEditor( (Geometry::ElementEditor*) 0 );
    CallBack movementcb( mCB(this,MPEEditor,nodeMovement) );
    CallBack numnodescb( mCB(this,MPEEditor,changeNumNodes) );

    if ( emeditor )
    {
	const_cast<EM::EMObject&>(emeditor->emObject()).
	    notifier.remove( movementcb );
	emeditor->editpositionchange.remove( numnodescb );
    }

    emeditor = eme;

    if ( emeditor )
    {
	const_cast<EM::EMObject&>(emeditor->emObject()).
	    notifier.notify( movementcb );
	emeditor->editpositionchange.notify( numnodescb );
	changeNumNodes(0);
    }
}


void MPEEditor::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher )
    {
	eventcatcher->eventhappened.remove( mCB( this, MPEEditor, clickCB ) );
	eventcatcher->unRef();
    }

    eventcatcher = nev;

    if ( eventcatcher )
    {
	eventcatcher->eventhappened.notify( mCB( this, MPEEditor, clickCB ) );
	eventcatcher->ref();
    }
}


EM::PosID  MPEEditor::markerId(const visBase::Marker* marker) const
{
    const int idx=markers.indexOf(marker);
    return idx==-1 ? EM::PosID(-1,-1,-1) : posids[idx];
}


void MPEEditor::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation )
	transformation->unRef();

    transformation = nt;
    if ( transformation )
	transformation->ref();

    for ( int idx=0; idx<markers.size(); idx++ )
    {
	markers[idx]->setDisplayTransformation( transformation );
    }

    if ( translationdragger )
	translationdragger->setDisplayTransformation( transformation );
}


bool MPEEditor::isDraggerShown() const
{
    return translationdragger && translationdragger->isOn();
}


EM::PosID MPEEditor::getNodePosID(int visid) const
{
    for ( int idx=0; idx<markers.size(); idx++ )
    {
	if ( markers[idx]->id()==visid )
	    return posids[idx];
    }

    return EM::PosID::udf();
}


void MPEEditor::changeNumNodes(CallBacker*)
{
    if ( emeditor )
	emeditor->getEditIDs( posids );
    else if ( geeditor )
    {
	posids.erase();
	TypeSet<GeomPosID> gpids;
	geeditor->getEditIDs( gpids );
	for ( int idy=0; idy<gpids.size(); idy++ )
	    posids += EM::PosID( -1, -1, gpids[idy] );
    }
    else
    {
	posids.erase();
    }

    while ( posids.size()>markers.size() )
    {
	visBase::Marker* node = visBase::Marker::create();
	node->setSelectable(false);
	node->ref();
	addChild( node->getInventorNode() );
	node->setDisplayTransformation( transformation );
	node->setMaterial( 0 );
	markers += node;
    }
	
    while ( posids.size()<markers.size() )
    {
	const int lastmarker = posids.size()-1;
	removeChild(markers[lastmarker]->getInventorNode() );
	markers[lastmarker]->unRef();
	markers.remove(lastmarker);
    }

    if ( !emeditor && !geeditor )
	return;

    for ( int idx=0; idx<posids.size(); idx++ )
    {
        updateNodePos( idx, emeditor
	    ? emeditor->getPosition( posids[idx] )
	    : geeditor->getPosition( posids[idx].subID()) );
    }

}


void MPEEditor::nodeMovement(CallBacker* cb)
{
    if ( emeditor )
    {
	mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
	if ( cbdata.event!=EM::EMObjectCallbackData::PositionChange )
	    return;

	const int idx = posids.indexOf( cbdata.pid0 );
	if ( idx==-1 ) return;

	const Coord3 pos = emeditor->getPosition( cbdata.pid0 );
	updateNodePos( idx,  emeditor->getPosition(cbdata.pid0) );
    }
    else if ( geeditor )
    {
	TypeSet<GeomPosID> pids;
	mCBCapsuleUnpack(const TypeSet<GeomPosID>*,gpids,cb);
	if ( gpids ) pids = *gpids;
	else geeditor->getEditIDs( pids );

	for ( int idx=0; idx<pids.size(); idx++ )
	{
	    for ( int idy=0; idy<posids.size(); idy++ )
	    {
		if ( posids[idy].subID()==pids[idx] )
		    updateNodePos( idy, geeditor->getPosition(pids[idx]));
	    }
	}
    }
}


void MPEEditor::updateNodePos( int idx, const Coord3& pos )
{
    visBase::Marker* node = markers[idx];
    node->setCenterPos( pos );

    if ( posids[idx] == activenode )
    {
	if ( !issettingpos )
	{
	    translationdragger->setPos( pos );
	    node->turnOn(false);
	}
    }
    else
    {
	node->turnOn(true);
    }
}

	

void MPEEditor::clickCB( CallBacker* cb )
{
    if ( eventcatcher->isEventHandled() || !isOn() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type!=visBase::MouseClick || eventinfo.mousebutton ||
	 !eventinfo.pressed )
	return;

    if ( translationdragger &&
	    eventinfo.pickedobjids.indexOf(translationdragger->id())!=-1 )
	return;

    int nodeidx = -1;
    for ( int idx=0; idx<markers.size(); idx++ )
    {
	if ( eventinfo.pickedobjids.indexOf(markers[idx]->id()) != -1 )
	{
	    nodeidx = idx;
	    break;
	}
    }

    if ( nodeidx==-1 && isDraggerShown() )
    {
	activenode = -1;
	updateDraggers();
	eventcatcher->eventIsHandled();
    }
    else if ( nodeidx!=-1 && activenode!=posids[nodeidx] )
    {
	activenode = posids[nodeidx];
	updateDraggers();
	eventcatcher->eventIsHandled();
    }
}


void MPEEditor::dragStart( CallBacker* cb )
{
    if ( emeditor ) emeditor->startEdit();
}


void MPEEditor::dragMotion( CallBacker* cb )
{
    if ( translationdragger && translationdragger==cb )
    {
	issettingpos++;
	const Coord3 np = translationdragger->getPos();
	if ( emeditor ) emeditor->setPosition( activenode, np );
	if ( geeditor ) geeditor->setPosition( activenode.subID(), np );
	issettingpos--;
    }
}


void MPEEditor::dragStop( CallBacker* cb )
{
    if ( emeditor ) emeditor->finishEdit();
}


void MPEEditor::updateDraggers()
{
    for ( int idx=0; idx<markers.size(); idx++ )
    {
	if ( !markers[idx] ) continue;
	markers[idx]->turnOn(true);
    }

    if ( activenode==-1 )
    {
	if ( translationdragger )
	    translationdragger->turnOn(false);

	return;
    }

    const int nodeidx = posids.indexOf(activenode);
    markers[nodeidx]->turnOn(false);


    if ( (emeditor && ( emeditor->mayTranslate3D(activenode) ||
			emeditor->mayTranslate2D(activenode) ||
			emeditor->mayTranslate1D(activenode)) ) ||
	 (geeditor && ( geeditor->mayTranslate3D(activenode.subID()) ||
			geeditor->mayTranslate2D(activenode.subID()) ||
			geeditor->mayTranslate1D(activenode.subID())) ) )
    {
	if ( !translationdragger )
	{
	    translationdragger = visBase::Dragger::create();
	    translationdragger->ref();
	    translationdragger->setDisplayTransformation( transformation );
	    addChild( translationdragger->getInventorNode() );

	    translationdragger->started.notify(mCB(this,MPEEditor,dragStart));
	    translationdragger->motion.notify(mCB(this,MPEEditor,dragMotion));
	    translationdragger->finished.notify(mCB(this,MPEEditor,dragStop));

	}

	translationdragger->turnOn(true);
	translationdragger->setPos( emeditor
		? emeditor->getPosition(activenode)
		: geeditor->getPosition(activenode.subID()) );

	if ( emeditor ? emeditor->mayTranslate3D(activenode)
		      : geeditor->mayTranslate3D(activenode.subID()))
	{
	    translationdragger->setDraggerType( visBase::Dragger::Translate3D );
	    translationdragger->setDefaultRotation();
	}
	else if ( emeditor ? emeditor->mayTranslate2D(activenode)
			   : geeditor->mayTranslate2D(activenode.subID()))
	{
	    translationdragger->setDraggerType( visBase::Dragger::Translate2D );
	    const Coord3 defnormal( 0, 0, 1 );
	    const Coord3 desnormal = emeditor 
		? emeditor->translation2DNormal( activenode ).normalize()
		: geeditor->translation2DNormal( activenode.subID() )
								.normalize();
	    const float angle = acos( defnormal.dot(desnormal) );
	    const Coord3 axis = defnormal.cross(desnormal);
	    translationdragger->setRotation( axis, angle );
	}
	else if ( emeditor ? emeditor->mayTranslate1D(activenode)
			   : geeditor->mayTranslate1D(activenode.subID()))
	{
	    translationdragger->setDraggerType( visBase::Dragger::Translate1D );
	    const Coord3 defori( 1, 0, 0 );
	    const Coord3 desori = emeditor 
		? emeditor->translation1DDirection( activenode ).normalize()
		: geeditor->translation1DDirection( activenode.subID() )
								.normalize();
	    const float angle = acos( defori.dot(desori) );
	    const Coord3 axis = defori.cross(desori);
	    translationdragger->setRotation( axis, angle );
	}

	visBase::ShapeScale* shapescale = visBase::ShapeScale::create();
	shapescale->ref();
	shapescale->restoreProportions(true);
	shapescale->setScreenSize(20);
	shapescale->setMinScale( 10 );
	shapescale->setMaxScale( 150 );
	shapescale->setShape( translationdragger->getShape("translator") );
	translationdragger->setOwnShape( shapescale, "translator" );
	shapescale->unRef();

	shapescale = visBase::ShapeScale::create();
	shapescale->ref();
	shapescale->restoreProportions(true);
	shapescale->setScreenSize(20);
	shapescale->setMinScale( 10 );
	shapescale->setMaxScale( 150 );
	shapescale->setShape( translationdragger->getShape("translatorActive"));
	translationdragger->setOwnShape( shapescale, "translatorActive" );
	shapescale->unRef();
    }
    else if ( translationdragger && translationdragger->isOn() )
	translationdragger->turnOn(false);
}


}; //namespce
