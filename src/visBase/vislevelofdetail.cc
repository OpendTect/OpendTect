/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vislevelofdetail.cc,v 1.4 2003-11-07 12:22:02 bert Exp $";


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
