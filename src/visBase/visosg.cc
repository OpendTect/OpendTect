/*
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Apr 2013
*/

const char* rcsID mUsedVar = "$Id$";

#include "visosg.h"

#include <osg/Referenced>


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
