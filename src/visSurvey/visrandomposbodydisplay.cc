/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : January 2009
-*/


#include "visrandomposbodydisplay.h"

#include "emmanager.h"
#include "emrandomposbody.h"
#include "executor.h"
#include "iopar.h"
#include "randcolor.h"
#include "vismaterial.h"
#include "visrandompos2body.h"
#include "vistransform.h"


namespace visSurvey
{

RandomPosBodyDisplay::RandomPosBodyDisplay()
    : VisualObjectImpl(true)
    , embody_( 0 )
    , displaybody_( 0 )
    , transform_( 0 )
{
    getMaterial()->setAmbience( 0.5 );
    setColor( getRandomColor( false ) );
}


RandomPosBodyDisplay::~RandomPosBodyDisplay()
{
    if ( transform_ ) transform_->unRef();
    if ( embody_ ) embody_->unRef();
    if ( displaybody_ )
    {
	removeChild( displaybody_->osgNode() );
	displaybody_->unRef();
    }
}


bool RandomPosBodyDisplay::setVisBody( visBase::RandomPos2Body* body )
{
    if ( !body ) return false;

    if ( displaybody_ )
    {
	removeChild( displaybody_->osgNode() );
	displaybody_->unRef();
	displaybody_ = 0;
    }

    if ( embody_ )
    {
	embody_->unRef();
	embody_ = 0;
    }

    mTryAlloc( embody_, EM::RandomPosBody(EM::EMM()) );
    if ( !embody_ ) return false;

    embody_->ref();
    if ( !embody_->isOK() )
    {
	embody_->unRef();
	embody_ = 0;
	return false;
    }

    embody_->setPositions( body->getPoints() );
    EM::EMM().addObject( embody_ );

    displaybody_ = body;
    displaybody_->ref();
    displaybody_->setDisplayTransformation( transform_ );
    displaybody_->setSelectable( false );
    addChild( displaybody_->osgNode() );

    if ( displaybody_->getMaterial() )
	getMaterial()->setFrom( *displaybody_->getMaterial() );

    displaybody_->setMaterial( 0 );
    embody_->setPreferredColor( getMaterial()->getColor() );

    return true;
}


EM::ObjectID RandomPosBodyDisplay::getEMID() const
{ return embody_ ? embody_->id() : -1; }


bool RandomPosBodyDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( embody_ )
    {
	embody_->unRef();
	embody_ = 0;
    }

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::RandomPosBody*, embody, emobject.ptr() );
    if ( !embody )
    {
	if ( displaybody_ ) displaybody_->turnOn( false );
	return false;
    }

    embody_ = embody;
    embody_->ref();

    return updateVisFromEM();
}


bool RandomPosBodyDisplay::updateVisFromEM()
{
    if ( !embody_ ) return false;
    getMaterial()->setColor( embody_->preferredColor() );
    setName( embody_->name().isEmpty() ? "<New body>" : embody_->name().str() );

    if ( !displaybody_ )
    {
	displaybody_ = visBase::RandomPos2Body::create();
	displaybody_->ref();
	displaybody_->setDisplayTransformation( transform_ );
	displaybody_->setMaterial( 0 );
	displaybody_->setSelectable( false );
	addChild( displaybody_->osgNode() );
    }

    if ( !displaybody_->setPoints( embody_->getPositions() ) )
    {
	removeChild( displaybody_->osgNode() );
	displaybody_->unRef();
	displaybody_ = 0;
	return false;
    }

    displaybody_->turnOn( true );
    return true;
}


MultiID RandomPosBodyDisplay::getMultiID() const
{
    return embody_ ? embody_->multiID() : MultiID();
}


void RandomPosBodyDisplay::setColor( Color nc )
{
    if ( embody_ ) embody_->setPreferredColor(nc);
    getMaterial()->setColor( nc );
}


Color RandomPosBodyDisplay::getColor() const
{ return getMaterial()->getColor(); }


NotifierAccess* RandomPosBodyDisplay::materialChange()
{ return &getMaterial()->change; }


void RandomPosBodyDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );
    par.set( sKeyPSEarthModelID(), getMultiID() );
}


bool RandomPosBodyDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	return false;

    MultiID newmid;
    if ( par.get(sKeyPSEarthModelID(),newmid) )
    {
	EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader ) loader->execute();
	    emid = EM::EMM().getObjectID( newmid );
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject ) setEMID( emobject->id() );
    }
    else
	return false;

    return true;
}


void RandomPosBodyDisplay::setDisplayTransformation(const mVisTrans* nt)
{
    if ( transform_ ) transform_->unRef();
    transform_ = nt;
    transform_->ref();

    if ( displaybody_ ) displaybody_->setDisplayTransformation( nt );
}


const mVisTrans* RandomPosBodyDisplay::getDisplayTransformation() const
{ return transform_; }


}; // namespace visSurvey
