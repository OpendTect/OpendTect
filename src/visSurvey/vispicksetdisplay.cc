/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.9 2002-03-20 08:28:03 nanne Exp $";

#include "vissurvpickset.h"
#include "vissceneobjgroup.h"
#include "vissurvscene.h"
#include "position.h"
#include "viscube.h"
#include "geompos.h"
#include "color.h"

visSurvey::PickSet::PickSet()
    : group( visBase::SceneObjectGroup::create(true) )
    , inlsz( 5 )
    , crlsz( 5 )
    , tsz( 0.05 )
    , addedpoint( this )
    , removedpoint( this )
{
    group->ref();
    addChild( group->getData() );
}


visSurvey::PickSet::~PickSet()
{
    removeChild( group->getData() );
    group->unRef();
}


int visSurvey::PickSet::nrPicks() const
{
    return group->size();
}


Geometry::Pos visSurvey::PickSet::getPick( int idx ) const
{

    mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
    if ( cube )
    {
	return cube->centerPos();
    }

    Geometry::Pos res(mUndefValue,mUndefValue,mUndefValue);

    return res;
}


void visSurvey::PickSet::addPick( const Geometry::Pos& pos )
{
    visBase::Cube* cube = visBase::Cube::create();
    cube->setCenterPos( pos );
    cube->setWidth( Geometry::Pos( inlsz, crlsz, tsz) );
    cube->setMaterial( 0 );

    group->addObject( cube );
}


void visSurvey::PickSet::setSize( float inl, float crl, float t )
{
    inlsz = inl; crlsz = crl; tsz = t;

    Geometry::Pos nsz( inl, crl, t );

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
	if ( !cube ) continue;

	cube->setWidth( nsz );
    }
}


void visSurvey::PickSet::removePick( const Geometry::Pos& pos )
{
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
	if ( !cube ) continue;

//	TODO: remove cube nearest to pos.
	if ( cube->centerPos() == pos )
	{
	    cube->remove();
	    return;
	}
    }

}


void visSurvey::PickSet::removeAll()
{
    group->removeAll();
}
