/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emeditor.cc,v 1.4 2005-01-10 12:37:01 kristofer Exp $";

#include "emeditor.h"

#include "emhistory.h"
#include "emmanager.h"
#include "emobject.h"
#include "geeditor.h"

namespace MPE 
{


ObjectEditor::ObjectEditor( EM::EMObject& emobj_ )
    : emobject( emobj_ )
{}


void ObjectEditor::startEdit()
{ changedpids.erase(); }


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
    const Geometry::Element* ge =
	const_cast<const EM::EMObject*>(&emobject)->getElement( sid );
    if ( !ge ) return 0;

    for ( int idx=0; idx<geeditors.size(); idx++ )
    {
	if ( &geeditors[idx]->getElement()==ge )
	    return geeditors[idx];
    }

    Geometry::ElementEditor* geeditor = createEditor( sid );
    if ( geeditor ) geeditors += geeditor;

    return geeditor;
}


const Geometry::ElementEditor* ObjectEditor::getEditor(
	const EM::SectionID& sid ) const
{
    return const_cast<ObjectEditor*>(this)->getEditor(sid);
}


EditorFactory::EditorFactory( const char* emtype,
			      EMEditorCreationFunc cf )
    : createfunc( cf )
    , type( emtype )
{}


const char* EditorFactory::emObjectType() const { return type; }


ObjectEditor* EditorFactory::create( EM::EMObject& emobj ) const
{ return createfunc( emobj ); }



}; //Namespace
