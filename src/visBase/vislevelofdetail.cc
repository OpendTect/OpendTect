/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "vislevelofdetail.h"

#include <Inventor/nodes/SoLevelOfDetail.h>

mCreateFactoryEntry( visBase::LevelOfDetail );

namespace visBase
{

LevelOfDetail::LevelOfDetail()
    : lod(new SoLevelOfDetail)
{
    lod->ref();
}


LevelOfDetail::~LevelOfDetail()
{
    lod->removeAllChildren();
    for ( int idx=0; idx<children.size(); idx++ )
	children[idx]->unRef();

    lod->unref();
}


void LevelOfDetail::addChild( DataObject* obj, float m )
{
    int nrkids = lod->getNumChildren();
    if ( nrkids )
	lod->screenArea.set1Value( nrkids-1, m );
    lod->addChild( obj->getInventorNode() );
    children += obj;
    obj->ref();
}


SoNode* LevelOfDetail::gtInvntrNode()
{ return lod; }

}; // namespace visBase
