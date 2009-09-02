#ifndef bouncydata_h
#define bouncydata_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
 * ID       : $Id: beachballdata.h,v 1.1 2009-09-02 15:46:30 cvskarthika Exp $
-*/

#include "color.h"
#include "namedobj.h"
#include "position.h"

namespace visBeachBall 
{ 
// Classes for communicating properties between plugin and visBase.

// Class for the basic properties of the ball
class BallProperties: public ::NamedObject
{
public:

    				BallProperties( const char* nm=0,
						float r=500, 
						Color c1=Color(255,0,0),
						Color c2=Color::White(),
						Coord3 pos=Coord3(0, 0, 0),
						float el=0.5) 
					: ::NamedObject(nm)
					, radius_(r) 
					, color1_(c1), color2_(c2)
					,  pos_(pos), elasticity_(el)	{}

    BallProperties		get() const;
    void			set(const BallProperties&);
    
    float			getRadius() const;
    void			setRadius(float);

    Color			getColor1() const;
    void			setColor1(Color);

    Color			getColor2() const;
    void			setColor2(Color);

    Coord3			getPos() const;
    void			setPos(Coord3);

    float			getElasticity() const;
    void			setElasticity(float);

protected:

    float			radius_;
    Color			color1_;
    Color			color2_;
    Coord3			pos_;
    float			elasticity_; // range is 0.0 to 1.0

};


// class for the dynamic properties of the ball
class BallDynamics: public ::NamedObject
{
public:
			
    				BallDynamics(const char* nm=0,
					     float sp=1.0,
					     Coord3 dirvec=Coord3(1, 1, 1))
				    : ::NamedObject(nm) 
				    , speed_(sp)
				    , directionvec_(dirvec)	{}

    BallDynamics		get() const;
    void			set(const BallDynamics&);
    
    void			getVelocity(float* speed, 
	    				    Coord3* directionvec) const;
    void			setVelocity(float speed, 
	    				    Coord3 directionvec);

protected:
    float			speed_;
    Coord3			directionvec_;

};

};


#endif
