/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visshapescale.cc,v 1.2 2002-07-22 09:35:06 kristofer Exp $";

#include "visshapescale.h"
#include "iopar.h"
#include "visdataman.h"

#include "SoShapeScale.h"


mCreateFactoryEntry( visBase::ShapeScale );

const char* visBase::ShapeScale::shapeidstr = "Shape ID";


visBase::ShapeScale::ShapeScale()
    : shapescalekit( new SoShapeScale )
    , shape( 0 )
{
    shapescalekit->ref();
    shapescalekit->projectedSize.setValue( 5 );
}


visBase::ShapeScale::~ShapeScale()
{
    if ( shape ) shape->unRef();
    shapescalekit->unref();
}


void visBase::ShapeScale::setShape( SceneObject* no )
{
    if ( shape ) shape->unRef();
    shape = no;
    if ( no ) no->ref();
    shapescalekit->setPart("shape", no ? no->getData() : 0 );
}


void visBase::ShapeScale::setShape( SoNode* node )
{
    if ( shape ) shape->unRef();
    shape = 0;
    shapescalekit->setPart("shape", node );
}


void visBase::ShapeScale::setSize( float nz )
{
    shapescalekit->projectedSize.setValue( nz );
}


float visBase::ShapeScale::getSize() const
{
    return shapescalekit->projectedSize.getValue();
}


void visBase::ShapeScale::freeze(bool yn)
{
    shapescalekit->active = yn;
}


bool visBase::ShapeScale::isFrozen() const
{
    return shapescalekit->active.getValue();
}


SoNode*  visBase::ShapeScale::getData() 
{
    return shapescalekit;
}


int visBase::ShapeScale::usePar( const IOPar& iopar )
{
    int res = SceneObject::usePar( iopar );
    if ( res!= 1 ) return res;

    int shapeid;
    if ( !iopar.get( shapeidstr, shapeid )) return -1;
    if ( shapeid==-1 ) return 1;

    DataObject* dataobj = DM().getObj( shapeid );
    if ( !dataobj ) { setShape( (SoNode*) 0 ); return 0; }
    mDynamicCastGet( SceneObject*, sceneobj, dataobj );
    if ( !sceneobj ) return -1;

    setShape( sceneobj );
    return 1;
}


void visBase::ShapeScale::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    SceneObject::fillPar( iopar, saveids );

    int shapeid = shape ? shape->id() : -1;
    iopar.set( shapeidstr, shapeid );
    if ( saveids.indexOf( shapeid )==-1 ) saveids += shapeid;
}
