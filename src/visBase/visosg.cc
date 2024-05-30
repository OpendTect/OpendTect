/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visosg.h"

#include "osgver.h"
#include "perthreadrepos.h"

#include <osg/Referenced>
#include <osg/Version>


void visBase::refOsgPtr( const osg::Referenced* ptr )
{
    if ( ptr )
	ptr->ref();
}


void visBase::unRefOsgPtr( osg::Referenced* ptr )
{
    if ( ptr )
	ptr->unref();
}


void visBase::unRefAndZeroOsgPtr( osg::Referenced* ptr )
{
    unRefAndNullOsgPtr( ptr );
}



// OneFrameCullDisabler

OneFrameCullDisabler::OneFrameCullDisabler( osg::Node* node )
{
    node->setCullingActive( false );
    node->setCullCallback( this );
    ref();
}


OneFrameCullDisabler::~OneFrameCullDisabler()
{}


void OneFrameCullDisabler::operator()( osg::Node* node, osg::NodeVisitor* nv )
{
    node->setCullingActive( true );
    node->removeCullCallback( this );
    unref();
}



const char* GetOSGVersion()
{
    mDeclStaticString( ret );
    if ( !ret.isEmpty() )
	return ret.buf();

    ret.set( OPENSCENEGRAPH_MAJOR_VERSION ).add( "." )
       .add( OPENSCENEGRAPH_MINOR_VERSION ).add( "." )
       .add( OPENSCENEGRAPH_PATCH_VERSION );
    return ret.buf();
}
