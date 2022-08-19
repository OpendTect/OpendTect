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

void visBase::unRefAndZeroOsgPtr( osg::Referenced* ptr )
{
    if ( !ptr ) return;
    ptr->unref();
    ptr = 0;
}


void visBase::unRefOsgPtr( osg::Referenced* ptr )
{
    if ( !ptr ) return;
    ptr->unref();
}


void visBase::refOsgPtr( const osg::Referenced* ptr )
{
    if ( !ptr ) return;
    ptr->ref();
}


OneFrameCullDisabler::OneFrameCullDisabler( osg::Node* node )
{
    node->setCullingActive( false );
    node->setCullCallback( this );
    ref();
}


void OneFrameCullDisabler::operator()( osg::Node* node, osg::NodeVisitor* nv )
{
    node->setCullingActive( true );
    node->removeCullCallback( this );
    unref();
}



const char* GetOSGVersion()
{
    mDeclStaticString( ret );
    if ( !ret.isEmpty() ) return ret.buf();

    ret.set( OPENSCENEGRAPH_MAJOR_VERSION ).add( "." )
       .add( OPENSCENEGRAPH_MINOR_VERSION ).add( "." )
       .add( OPENSCENEGRAPH_PATCH_VERSION );
    return ret.buf();
}
