/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emeditor.cc,v 1.10 2005-07-18 14:07:21 cvskris Exp $";

#include "emeditor.h"

#include "emhistory.h"
#include "emmanager.h"
#include "emobject.h"
#include "geeditor.h"

namespace MPE 
{


ObjectEditor::ObjectEditor( EM::EMObject& emobj_ )
    : emobject( emobj_ )
    , editpositionchange( this )
    , movingnode( -1,-1,-1 )
{
    emobject.ref();
}


ObjectEditor::~ObjectEditor()
{
    emobject.unRef();
    deepErase( geeditors );
    sections.erase();
}


void ObjectEditor::startEdit(const EM::PosID& pid)
{
    changedpids.erase();
    movingnode = pid;

    if ( pid.objectID()!=-1 )
    {
        startpos = getPosition(movingnode);
        if ( !startpos.isDefined() )
        {
    	    pErrMsg( "Editnode is undefined");
	    movingnode = EM::PosID(-1,-1,-1);
	}
    }

    getAlongMovingNodes( alongmovingnodes, &alongmovingnodesfactors );

    alongmovingnodesstart.erase();
    for ( int idx=0; idx<alongmovingnodes.size(); idx++ )
	alongmovingnodesstart += emobject.getPos(alongmovingnodes[idx]);
}


bool ObjectEditor::setPosition(const Coord3& np)
{
    if ( movingnode.objectID()==-1 )
    {
	pErrMsg("Moving unknown node");
	return false;
    }

    if ( !np.isDefined() )
    {
	pErrMsg("You cannot set the editnode to undefined");
	return false;
    }

    const Coord3 diff = np-startpos;
    if ( !setPosition( movingnode, np ) )
	return false;

    for ( int idx=0; idx<alongmovingnodes.size(); idx++ )
    {
	const Coord3 newpos = alongmovingnodesstart[idx] +
		    alongmovingnodesfactors[idx]*diff;
	if ( !setPosition( alongmovingnodes[idx], newpos ) )
	    return false;
    }

    return true;
}


void ObjectEditor::finishEdit()
{
    if ( !changedpids.size() )
	return;

     EM::History& history = EM::EMM().history();
     const int currentevent = history.currentEventNr();
     if ( currentevent==-1 ||
	     history.getLevel(currentevent)>=mEMHistoryUserInteractionLevel )
	 return;

     history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
}


const BufferStringSet* ObjectEditor::getVertMovingStyleNames() const
{ return 0; }


void ObjectEditor::setEditIDs( const TypeSet<EM::PosID>& ids )
{
    editids = ids;
    editpositionchange.trigger();
}


void ObjectEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    ids.erase();

    for ( int idx=0; idx<emobject.nrSections(); idx++ )
    {
	const EM::SectionID sectionid =  emobject.sectionID(idx);
	const Geometry::ElementEditor* ge = getEditor( sectionid );
	if ( !ge ) continue;

	TypeSet<GeomPosID> gepids;
	ge->getEditIDs( gepids );
	for ( int idy=0; idy<gepids.size(); idy++ )
	    ids += EM::PosID( emobject.id(), sectionid, gepids[idy] );
    }
}


Coord3 ObjectEditor::getPosition( const EM::PosID& pid ) const
{ return emobject.getPos( pid ); }


bool ObjectEditor::setPosition( const EM::PosID& pid,  const Coord3& np )
{
    const bool addtohistory = changedpids.indexOf(pid) == -1;
    if ( addtohistory )
	changedpids += pid;

    return emobject.setPos( pid, np, addtohistory );
}

#define mMayFunction( func ) \
bool ObjectEditor::func( const EM::PosID& pid ) const \
{ \
    const Geometry::ElementEditor* ge = getEditor( pid.sectionID() ); \
    if ( !ge ) return false; \
 \
    return ge->func( pid.subID() ); \
}


#define mGetFunction( func ) \
Coord3 ObjectEditor::func( const EM::PosID& pid ) const\
{\
    const Geometry::ElementEditor* ge = getEditor( pid.sectionID() );\
    if ( !ge ) return Coord3::udf();\
\
    return ge->func( pid.subID() );\
}



mMayFunction( mayTranslate1D );
mGetFunction( translation1DDirection );


mMayFunction( mayTranslate2D );
mGetFunction( translation2DNormal );


mMayFunction( mayTranslate3D );


mMayFunction( maySetNormal );
mGetFunction( getNormal );


mMayFunction( maySetDirection );
mGetFunction( getDirectionPlaneNormal );
mGetFunction( getDirection );


Geometry::ElementEditor* ObjectEditor::getEditor( const EM::SectionID& sid )
{
    const int idx = sections.indexOf(sid);
    if ( idx!=-1 ) return geeditors[idx];

    Geometry::ElementEditor* geeditor = createEditor( sid );
    if ( geeditor )
    {
	geeditors += geeditor;
	sections += sid;
	geeditor->editpositionchange.notify(
		mCB(this,ObjectEditor,editPosChangeTrigger));
    }

    return geeditor;
}


const Geometry::ElementEditor* ObjectEditor::getEditor(
	const EM::SectionID& sid ) const
{ return const_cast<ObjectEditor*>(this)->getEditor(sid); }



void ObjectEditor::editPosChangeTrigger(CallBacker*)
{ editpositionchange.trigger(); }


void ObjectEditor::emSectionChange(CallBacker* cb)
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event!=EM::EMObjectCallbackData::SectionChange )
	return;

    const EM::SectionID sectionid = cbdata.pid0.sectionID();
    const int editoridx = sections.indexOf(sectionid);

    const Geometry::Element* ge =
	const_cast<const EM::EMObject*>(&emobject)->getElement( sectionid );

    if ( !ge && editoridx!=-1 )
    {
	delete geeditors[editoridx];
	geeditors.remove(editoridx);
	editpositionchange.trigger();
    }
    else if ( ge && editoridx==-1 )
    {
	Geometry::ElementEditor* geeditor = createEditor( sectionid );
	if ( geeditor )
	{
	    geeditors += geeditor;
	    sections += sectionid;
	    geeditor->editpositionchange.notify(
		    mCB(this,ObjectEditor,editPosChangeTrigger));
	}
    }
}


void ObjectEditor::getAlongMovingNodes( TypeSet<EM::PosID>& nodes,
					TypeSet<float>* factors ) const
{
    nodes.erase();
    if ( factors ) factors->erase();
}


EditorFactory::EditorFactory( const char* emtype, EMEditorCreationFunc cf )
    : createfunc( cf )
    , type( emtype )
{}


const char* EditorFactory::emObjectType() const { return type; }


ObjectEditor* EditorFactory::create( EM::EMObject& emobj ) const
{ return createfunc( emobj ); }





}; //Namespace
