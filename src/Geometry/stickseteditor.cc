/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C.Glas
 * DATE     : Feburary 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: stickseteditor.cc,v 1.1 2008-11-18 13:28:53 cvsjaap Exp $";

#include "stickseteditor.h"

#include "faultstickset.h"

namespace Geometry 
{


StickSetEditor::StickSetEditor( Geometry::FaultStickSet& fss )
    : ElementEditor( fss )
{
    fss.nrpositionnotifier.notify( mCB(this,StickSetEditor,addedKnots) );
}


StickSetEditor::~StickSetEditor()
{
    FaultStickSet& fss = reinterpret_cast<FaultStickSet&>(element);
    fss.nrpositionnotifier.remove( mCB(this,StickSetEditor,addedKnots) );
}


bool StickSetEditor::mayTranslate2D( GeomPosID gpid ) const
{ return translation2DNormal( gpid ).isDefined(); }


Coord3 StickSetEditor::translation2DNormal( GeomPosID gpid ) const
{
    const FaultStickSet& fss = reinterpret_cast<const FaultStickSet&>(element);
    const int stick = RowCol(gpid).r();
    return fss.getEditPlaneNormal( stick );
}


void StickSetEditor::addedKnots(CallBacker*)
{ editpositionchange.trigger(); }
    

    
}; //Namespace
