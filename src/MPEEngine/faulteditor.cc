/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: faulteditor.cc,v 1.4 2008-03-26 13:53:54 cvsjaap Exp $";

#include "faulteditor.h"

#include "emfault.h"
#include "faultsticksurfedit.h"
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
{ MPE::EditorFactory().addCreator( create, EM::Fault::typeStr() ); }


Geometry::ElementEditor* FaultEditor::createEditor( const EM::SectionID& sid )
{
    const Geometry::Element* ge = emObject().sectionGeometry( sid );
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::FaultStickSurface*,surface,ge);
    if ( !surface ) return 0;
    
    return new Geometry::FaultStickSurfEditor(
			*const_cast<Geometry::FaultStickSurface*>(surface) );
}


void FaultEditor::getEditIDs( TypeSet<EM::PosID>& pids ) const
{ pids = editpids_; }


bool FaultEditor::addEditID( const EM::PosID& pid )
{
    if ( editpids_.indexOf(pid) < 0 )
    {
	editpids_ += pid;
	editpositionchange.trigger();
    }

    return true;
}


bool FaultEditor::removeEditID( const EM::PosID& pid )
{
    const int idx = editpids_.indexOf( pid );
    if ( idx < 0 )
	return false;
    
    editpids_.removeFast( idx );
    editpositionchange.trigger();
    return true;
}


};  // namespace MPE
