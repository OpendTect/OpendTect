/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C.Glas
 * DATE     : Feburary 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: stickseteditor.cc,v 1.3 2010/06/17 19:00:58 cvskris Exp $";

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
    const int stick = RowCol(gpid).row;
    return fss.getEditPlaneNormal( stick );
}


void StickSetEditor::addedKnots(CallBacker*)
{ editpositionchange.trigger(); }
    

    
}; //Namespace
