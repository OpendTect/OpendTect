/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.4 2002-03-04 14:21:10 kristofer Exp $";

#include "vissurvpickset.h"
#include "vissceneobjgroup.h"
#include "vissurvscene.h"
#include "position.h"
#include "viscube.h"
#include "geompos.h"
#include "color.h"

visSurvey::PickSet::PickSet( visSurvey::Scene& scene_ )
    : group( new visBase::SceneObjectGroup( true, true ) )
    , scene( scene_ )
    , inlsz( 5 )
    , crlsz( 5 )
    , tsz( 0.05 )
    , color( *new Color )
{
    color.set( 0, 255, 0 );
}


visSurvey::PickSet::~PickSet()
{
    delete &color;
}


Geometry::Pos visSurvey::PickSet::getPick( int idx ) const
{

    mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
    if ( cube )
    {
	return cube->centerPos();
    }

    Geometry::Pos res;

    return res;
}


int visSurvey::PickSet::addPick( const Geometry::Pos& pos )
{
    visBase::Cube* cube = new visBase::Cube( scene );
    cube->setCenterPos( pos );
    cube->setWidth( Geometry::Pos( inlsz, crlsz, tsz) );
    cube->setColor( color );

    return group->addObject( cube );
}


void visSurvey::PickSet::setSize( float inl, float crl, float t )
{
    inlsz = inl; crlsz = crl; tsz = t;

    Geometry::Pos nsz( inl, crl, t );

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube,
			group->getObject( group->getId(idx) ) );
	if ( !cube ) continue;

	cube->setWidth( nsz );
    }
}


void visSurvey::PickSet::turnOn( bool ns )
{
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube,
			group->getObject( group->getId(idx) ) );
	if ( !cube ) continue;

	cube->turnOn( ns );
    }
}


bool visSurvey::PickSet::isOn() const
{
    if ( group->size() )
    {
	mDynamicCastGet(visBase::Cube*, cube,
		group->getObject(group->getId(0)));
	return cube->isOn();
    }

    return true;
}


void visSurvey::PickSet::setColor( const Color& c )
{
    color = c;

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube,
			group->getObject( group->getId(idx) ) );
	if ( !cube ) continue;

	cube->setColor( c );
    }
}
