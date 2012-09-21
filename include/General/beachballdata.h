#ifndef beachballdata_h
#define beachballdata_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
 * ID       : $Id$
-*/

#include "generalmod.h"
#include "color.h"
#include "namedobj.h"
#include "position.h"

namespace visBeachBall 
{ 

//!\brief basic properties of the ball

mClass(General) BallProperties: public ::NamedObject
{
public:

    				BallProperties( const char* nm=0,
						float r=500, 
						Color c1=Color(255,0,0),
						Color c2=Color::White(),
						Coord3 p=Coord3(0, 0, 0),
						float el=0.5) 
					: ::NamedObject(nm)
					, radius_(r) 
					, color1_(c1), color2_(c2)
					,  pos_(p), elasticity_(el)	{}

    BallProperties		get() const;
    void			set(const BallProperties&);
    
    float			radius() const;
    void			setRadius(float);

    Color			color1() const;
    void			setColor1(Color);

    Color			color2() const;
    void			setColor2(Color);

    Coord3			pos() const;
    void			setPos(const Coord3&);

    float			elasticity() const;
    void			setElasticity(float);

    BallProperties&		operator = (const BallProperties& bp);
    bool			operator == (const BallProperties& bp) const;
    bool			operator != (const BallProperties& bp) const;

protected:

    float			radius_;
    Color			color1_;
    Color			color2_;
    Coord3			pos_;
    float			elasticity_; // range is 0.0 to 1.0

};


//!\brief dynamic properties of the ball

mClass(General) BallDynamics: public ::NamedObject
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
    
    float			speed() const;
    void			setSpeed(const float&); 
    
    Coord3			directionvector() const;
    void			setDirectionVector(const Coord3&);

    void			velocity(float*, Coord3*) const;
    void			setVelocity(float, const Coord3&); 

    BallDynamics&		operator = (const BallDynamics& bd);
    bool			operator == (const BallDynamics& bd) const;
    bool			operator != (const BallDynamics& bd) const;

protected:

    float			speed_;
    Coord3			directionvec_;	//!< movement dir

};

};


#endif

