/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vislevelofdetail.cc,v 1.5 2004-01-05 09:43:23 kristofer Exp $";


#include "vislevelofdetail.h"


#include "Inventor/nodes/SoLevelOfDetail.h"

mCreateFactoryEntry( visBase::LevelOfDetail );


visBase::LevelOfDetail::LevelOfDetail()
    : lod( new SoLevelOfDetail )
{
    lod->ref();
}


visBase::LevelOfDetail::~LevelOfDetail()
{
    lod->removeAllChildren();
    for ( int idx=0; idx<children.size(); idx++ )
	children[idx]->unRef();

    lod->unref();
}


void visBase::LevelOfDetail::addChild( DataObject* obj, float m )
{
    int nrkids = lod->getNumChildren();
    if ( nrkids )
	lod->screenArea.set1Value( nrkids-1, m );
    lod->addChild( obj->getInventorNode() );
    children += obj;
    obj->ref();
}


SoNode* visBase::LevelOfDetail::getInventorNode()
{ return lod; }
