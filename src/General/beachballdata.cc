
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID = "$Id: beachballdata.cc,v 1.1 2009-09-02 15:45:13 cvskarthika Exp $";

#include "beachballdata.h"

namespace visBeachBall
{

BallProperties BallProperties::get()const
{
    BallProperties bp(*this);
    return bp;
}


void BallProperties::set( const BallProperties& bp )
{
    *this = bp;
}


float BallProperties::getRadius() const
{
    return radius_;
}


void BallProperties::setRadius( float r)
{
    radius_ = r;
}


Color BallProperties::getColor1() const
{
    return color1_;
}


void BallProperties::setColor1( Color c )
{
    color1_ = c;
}


Color BallProperties::getColor2() const
{
    return color2_;
}


void BallProperties::setColor2( Color c )
{
    color2_ = c;
}

    
Coord3 BallProperties::getPos() const
{
    return pos_;
}


void BallProperties::setPos( Coord3 pos )
{
    pos_ = pos;
}


float BallProperties::getElasticity() const
{
    return elasticity_;
}


void BallProperties::setElasticity( float el )
{
    elasticity_ = el;
}

};

