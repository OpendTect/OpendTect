/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vislevelofdetail.cc,v 1.6 2005-02-04 14:31:34 kristofer Exp $";


#include "vislevelofdetail.h"


#include "Inventor/nodes/SoLevelOfDetail.h"

namespace visBase
{

mCreateFactoryEntry( LevelOfDetail );


LevelOfDetail::LevelOfDetail()
    : lod( new SoLevelOfDetail )
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


SoNode* LevelOfDetail::getInventorNode()
{ return lod; }

}; // namespace visBase
