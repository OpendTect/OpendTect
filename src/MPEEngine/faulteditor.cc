/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: faulteditor.cc,v 1.2 2006-04-27 15:53:13 cvskris Exp $";

#include "faulteditor.h"

#include "emfault.h"
#include "cubicbeziersurface.h"
#include "geeditorimpl.h"
#include "mpeengine.h"

namespace MPE
{

FaultEditor::FaultEditor( EM::Fault& fault )
    : ObjectEditor(fault)
{}


ObjectEditor* FaultEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::Fault*,fault,&emobj);
    if ( !fault ) return 0;
    return new FaultEditor(*fault);
}


void FaultEditor::initClass()
{
    MPE::engine().addEditorFactory(
	    new EditorFactory( EM::Fault::typeStr(), create ) );
}


Geometry::ElementEditor* FaultEditor::createEditor( const EM::SectionID& sid )
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::CubicBezierSurface*,surface,ge);
    if ( !surface ) return 0;
    
    return new Geometry::ElementEditorImpl(
	    *const_cast<Geometry::CubicBezierSurface*>(surface),
	    Coord3::udf(), Coord3(0,0,1), false );
}


void FaultEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    // TODO: create modes like: all, sparse, clicked pos
    ids.erase();
    ObjectEditor::getEditIDs( ids );
    
    // clickedpos
    // ids = editids;
}


};  // namespace MPE
