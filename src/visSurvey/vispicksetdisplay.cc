/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.1 2002-02-27 14:42:15 kristofer Exp $";

#include "vissurvpickset.h"
#include "vissceneobjgroup.h"
#include "vissurvscene.h"
#include "position.h"
#include "viscube.h"
#include "color.h"

visSurvey::PickSet::PickSet( visSurvey::Scene& scene_ )
    : group( new visBase::SceneObjectGroup( true, true ) )
    , scene( scene_ )
    , inlsz( 11 )
    , crlsz( 11 )
    , tsz( 0.05 )
    , color( *new Color )
{
    color.set( 0, 255, 0 );
    groupid = scene.addInlCrlTObject( group );
}


visSurvey::PickSet::~PickSet()
{
    scene.removeObject( groupid );
    delete &color;
}


BinIDValue visSurvey::PickSet::getPick( int idx ) const
{
    BinIDValue res;

    mDynamicCastGet(visBase::Cube*, cube, group->getObject( idx ) );
    if ( cube )
    {
	res.binid.inl = mNINT(cube->centerPos( 0 ));
	res.binid.crl = mNINT(cube->centerPos( 1 ));
	res.value = cube->centerPos( 2 );
    }

    return res;
}


int visSurvey::PickSet::addPick( const BinIDValue& bidv )
{
    visBase::Cube* cube = new visBase::Cube( scene );
    cube->setCenterPos( bidv.binid.inl, bidv.binid.crl, bidv.value );
    cube->setWidth( inlsz, crlsz, tsz );
    cube->setColor( color );

    return group->addObject( cube );
}


void visSurvey::PickSet::setSize( float inl, float crl, float t )
{
    inlsz = inl; crlsz = crl; tsz = t;

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Cube*, cube,
			group->getObject( group->getId(idx) ) );
	if ( !cube ) continue;

	cube->setWidth( inlsz, crlsz, tsz );
    }
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
