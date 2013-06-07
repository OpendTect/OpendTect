
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id$";

#include "beachballdata.h"

namespace visBeachBall
{

BallProperties BallProperties::get()const
{
    //BallProperties bp( *this );
    BallProperties bp;
    bp.setName( name_->buf() );
    bp.setRadius( radius_ );
    bp.setColor1( color1_ );
    bp.setColor2( color2_ );
    bp.setPos( pos_ );
    bp.setElasticity( elasticity_ );
    return bp;
}


void BallProperties::set( const BallProperties& bp )
{
    setName( bp.name() );    
    radius_ = bp.radius();
    color1_ = bp.color1();
    color2_ = bp.color2();
    pos_ = bp.pos();
    elasticity_ = bp.elasticity();
}


float BallProperties::radius() const
{
    return radius_;
}


void BallProperties::setRadius( float r)
{
    radius_ = r;
}


Color BallProperties::color1() const
{
    return color1_;
}


void BallProperties::setColor1( Color c )
{
    color1_ = c;
}


Color BallProperties::color2() const
{
    return color2_;
}


void BallProperties::setColor2( Color c )
{
    color2_ = c;
}

    
Coord3 BallProperties::pos() const
{
    return pos_;
}


void BallProperties::setPos( const Coord3& p )
{
    pos_ = p;
}


float BallProperties::elasticity() const
{
    return elasticity_;
}


void BallProperties::setElasticity( float el )
{
    elasticity_ = el;
}


BallProperties& BallProperties::operator = ( const BallProperties& bp )
{
    if ( this == &bp )
	return *this;

//    this->NamedObject::operator = ( bp );
    // Fix this! Crashes! Properly copy base class data! 
    setName( bp.name() );
    radius_ = bp.radius();
    color1_ = bp.color1();
    color2_ = bp.color2();
    pos_ = bp.pos();
    elasticity_ = bp.elasticity();
    return *this;
}


bool BallProperties::operator == ( const BallProperties& bp ) const
{
    if ( radius_ == bp.radius() && color1_ == bp.color1() 
	 && color2_ == bp.color2() && pos_ == bp.pos()
	 && elasticity_ == bp.elasticity() )
	return NamedObject::operator ==( bp );
    else return false;
}


bool BallProperties::operator != ( const BallProperties& bp ) const
{
    return ! (*this == bp);
}


BallDynamics BallDynamics::get() const
{
    //BallDynamics bd( *this );
    BallDynamics bd;
    bd.setName( name_->buf() );
    bd.setSpeed( speed_ );
    bd.setDirectionVector( directionvec_ );
    return bd;
}


void BallDynamics::set( const BallDynamics&bd )
{
    setName( bd.name() );    
    speed_ = bd.speed();
    directionvec_ = bd.directionvector();
}


float BallDynamics::speed() const
{
    return speed_;
}


void BallDynamics::setSpeed( const float& sp )
{
   speed_ = sp;
}

    
Coord3 BallDynamics::directionvector() const
{
    return directionvec_;
}


void BallDynamics::setDirectionVector( const Coord3& dv )
{
    directionvec_ = dv;
}


void BallDynamics::velocity( float* sp, Coord3* directionvec ) const
{
    if ( !sp )
	sp = new float;
    *sp = speed_;

    if ( !directionvec )
	directionvec = new Coord3;
    *directionvec = directionvec_;
}


void BallDynamics::setVelocity( float sp, const Coord3& directionvec )
{
    speed_ = sp;
    directionvec_ = directionvec;
}


BallDynamics& BallDynamics::operator = (const BallDynamics& bd)
{
    if ( this == &bd )
	return *this;

//    this->NamedObject::operator = ( bd );
    setName( bd.name() );
    speed_ = bd.speed();
    directionvec_ = bd.directionvector();
    return *this;
}


bool BallDynamics::operator == (const BallDynamics& bd) const
{
    if ( speed_ == bd.speed() && directionvec_ == bd.directionvector() )
	return NamedObject::operator ==( bd );
    else return false;
}


bool BallDynamics::operator != (const BallDynamics& bd) const
{
    return ! (*this == bd);
}

};

