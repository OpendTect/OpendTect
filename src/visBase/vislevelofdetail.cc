/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vislevelofdetail.cc,v 1.1 2002-06-24 13:20:00 kristofer Exp $";


#include "vislevelofdetail.h"


#include "Inventor/nodes/SoLevelOfDetail.h"


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


void visBase::LevelOfDetail::addChild( SceneObject* obj, float m )
{
    int nrkids = lod->getNumChildren();
    if ( nrkids )
	lod->screenArea.set1Value( nrkids-1, m );
    lod->addChild( obj->getData() );
    children += obj;
    obj->ref();
}


SoNode* visBase::LevelOfDetail::getData()
{ return lod; }
