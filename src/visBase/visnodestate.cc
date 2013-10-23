/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visnodestate.h"
#include "vispolygonoffset.h"

#include <osg/StateSet>
#include <osg/PolygonOffset>

using namespace visBase;


NodeState::NodeState()
{}

NodeState::~NodeState()
{
    while ( statesets_.size() )
	detachStateSet( statesets_[0]);
}


void NodeState::attachStateSet( osg::StateSet* ns )
{
    if ( statesets_.isPresent( ns ) )
    {
	pErrMsg("Stateset is already attached");
	return;
    }
    
    statesets_ += ns;
    ns->ref();
    
    for ( int idx=0; idx<attributes_.size(); idx++ )
	applyAttribute( ns, attributes_[idx] );
}


void NodeState::detachStateSet( osg::StateSet* ns )
{
    if ( !statesets_.isPresent( ns ) )
	return;
    
    statesets_ -= ns;
    
    for ( int idx=0; idx<attributes_.size(); idx++ )
	ns->removeAttribute( attributes_[idx] );
    
    ns->unref();
}



void NodeState::doAdd( osg::StateAttribute* as)
{
    if ( attributes_.isPresent(as) )
    {
	pErrMsg("Attribute is present");
	return;
    }

    for ( int idx=0; idx<statesets_.size(); idx++ )
	statesets_[idx]->setAttribute( as );
    
    attributes_ += as;
    as->ref();
}


void NodeState::doRemove( osg::StateAttribute* as)
{
    if ( !attributes_.isPresent(as) )
	return;

    for ( int idx=0; idx<statesets_.size(); idx++ )
	statesets_[idx]->removeAttribute( as );
    
    attributes_ -= as;
    as->unref();
}


void NodeState::applyAttribute( osg::StateSet* ns, osg::StateAttribute* attr)
{
    if ( ns )
	ns->setAttribute( attr );
}
