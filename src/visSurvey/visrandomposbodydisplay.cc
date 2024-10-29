/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visrandomposbodydisplay.h"

#include "color.h"
#include "emmanager.h"
#include "executor.h"
#include "iopar.h"
#include "randcolor.h"
#include "vismaterial.h"


namespace visSurvey
{

RandomPosBodyDisplay::RandomPosBodyDisplay()
    : visBase::VisualObjectImpl(true)
{
    ref();
    getMaterial()->setAmbience( 0.5 );
    setColor( OD::getRandomColor(false) );
    unRefNoDelete();
}


RandomPosBodyDisplay::~RandomPosBodyDisplay()
{
    if ( displaybody_ )
	removeChild( displaybody_->osgNode() );
}


bool RandomPosBodyDisplay::setVisBody( visBase::RandomPos2Body* body )
{
    if ( !body )
	return false;

    if ( displaybody_ )
	removeChild( displaybody_->osgNode() );

    displaybody_ = nullptr;
    embody_ = new EM::RandomPosBody( EM::EMM() );
    if ( !embody_ || !embody_->isOK() )
    {
	embody_ = nullptr;
	return false;
    }

    embody_->setPositions( body->getPoints() );
    EM::EMM().addObject( embody_.ptr() );

    displaybody_ = body;
    displaybody_->setDisplayTransformation( transform_.ptr() );
    displaybody_->setSelectable( false );
    addChild( displaybody_->osgNode() );

    if ( displaybody_->getMaterial() )
	getMaterial()->setFrom( *displaybody_->getMaterial() );

    displaybody_->setMaterial( nullptr );
    embody_->setPreferredColor( getMaterial()->getColor() );

    return true;
}


EM::ObjectID RandomPosBodyDisplay::getEMID() const
{
    return embody_ ? embody_->id() : EM::ObjectID::udf();
}


bool RandomPosBodyDisplay::setEMID( const EM::ObjectID& emid )
{
    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::RandomPosBody*, embody, emobject.ptr() );
    if ( !embody )
    {
	if ( displaybody_ )
	    displaybody_->turnOn( false );

	return false;
    }

    embody_ = embody;
    return updateVisFromEM();
}


bool RandomPosBodyDisplay::updateVisFromEM()
{
    if ( !embody_ )
	return false;

    getMaterial()->setColor( embody_->preferredColor() );
    setName( embody_->name().isEmpty() ? "<New Geobody>"
				       : embody_->name().str() );

    if ( !displaybody_ )
    {
	displaybody_ = visBase::RandomPos2Body::create();
	displaybody_->setDisplayTransformation( transform_.ptr() );
	displaybody_->setMaterial( nullptr );
	displaybody_->setSelectable( false );
	addChild( displaybody_->osgNode() );
    }

    if ( !displaybody_->setPoints(embody_->getPositions()) )
    {
	removeChild( displaybody_->osgNode() );
	displaybody_ = nullptr;
	return false;
    }

    displaybody_->turnOn( true );
    return true;
}


MultiID RandomPosBodyDisplay::getMultiID() const
{
    return embody_ ? embody_->multiID() : MultiID();
}


void RandomPosBodyDisplay::setColor( OD::Color nc )
{
    if ( embody_ )
	embody_->setPreferredColor(nc);

    getMaterial()->setColor( nc );
}


OD::Color RandomPosBodyDisplay::getColor() const
{
    return getMaterial()->getColor();
}


void RandomPosBodyDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    SurveyObject::fillPar( par );
    par.set( sKeyPSEarthModelID(), getMultiID() );
}


bool RandomPosBodyDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar(par) || !SurveyObject::usePar(par) )
	return false;

    MultiID newmid;
    if ( !par.get(sKeyPSEarthModelID(),newmid) )
	return false;

    EM::ObjectID emid = EM::EMM().getObjectID( newmid );
    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    if ( !emobject )
    {
	PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	if ( loader )
	    loader->execute();

	emid = EM::EMM().getObjectID( newmid );
	emobject = EM::EMM().getObject( emid );
    }

    if ( emobject )
	setEMID( emobject->id() );

    return true;
}


void RandomPosBodyDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    transform_ = nt;
    if ( displaybody_ )
	displaybody_->setDisplayTransformation( nt );
}


const mVisTrans* RandomPosBodyDisplay::getDisplayTransformation() const
{
    return transform_.ptr();
}


} // namespace visSurvey
