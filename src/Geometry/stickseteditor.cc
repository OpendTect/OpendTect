/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C.Glas
 * DATE     : Feburary 2008
___________________________________________________________________

-*/


#include "stickseteditor.h"

#include "faultstickset.h"

namespace Geometry 
{


StickSetEditor::StickSetEditor( Geometry::FaultStickSet& fss )
    : ElementEditor( fss )
{
    mAttachCB(  fss.nrpositionnotifier, StickSetEditor::addedKnots );
}


StickSetEditor::~StickSetEditor()
{
    detachAllNotifiers();
}


bool StickSetEditor::mayTranslate2D( GeomPosID gpid ) const
{ return translation2DNormal( gpid ).isDefined(); }


Coord3 StickSetEditor::translation2DNormal( GeomPosID gpid ) const
{
    const FaultStickSet& fss = reinterpret_cast<const FaultStickSet&>(element);
    const int stick = RowCol::fromInt64(gpid).row();
    return fss.getEditPlaneNormal( stick );
}


void StickSetEditor::addedKnots(CallBacker*)
{ editpositionchange.trigger(); }
    

    
}; //Namespace
