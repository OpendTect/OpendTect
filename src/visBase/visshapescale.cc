/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visshapescale.cc,v 1.6 2004-01-05 09:43:23 kristofer Exp $";

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
    shapescalekit->dorotate.setValue( false );
}


visBase::ShapeScale::~ShapeScale()
{
    if ( shape ) shape->unRef();
    shapescalekit->unref();
}


void visBase::ShapeScale::setShape( DataObject* no )
{
    if ( shape ) shape->unRef();
    shape = no;
    if ( no ) no->ref();
    shapescalekit->setPart("shape", no ? no->getInventorNode() : 0 );
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
    shapescalekit->doscale = yn;
}


bool visBase::ShapeScale::isFrozen() const
{
    return shapescalekit->doscale.getValue();
}


SoNode*  visBase::ShapeScale::getInventorNode() 
{
    return shapescalekit;
}


int visBase::ShapeScale::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
    if ( res!= 1 ) return res;

    int shapeid;
    if ( !iopar.get( shapeidstr, shapeid )) return -1;
    if ( shapeid==-1 ) return 1;

    DataObject* dataobj = DM().getObj( shapeid );
    if ( !dataobj ) { setShape( (SoNode*) 0 ); return 0; }
    mDynamicCastGet( DataObject*, sceneobj, dataobj );
    if ( !sceneobj ) return -1;

    setShape( sceneobj );
    return 1;
}


void visBase::ShapeScale::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );

    int shapeid = shape ? shape->id() : -1;
    iopar.set( shapeidstr, shapeid );
    if ( saveids.indexOf( shapeid )==-1 ) saveids += shapeid;
}
