#ifndef vislight_h
#define vislight_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vislight.h,v 1.6 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"


class SoLight;

namespace visBase
{

/*!\brief
Base class for all lights

*/

class Light : public SceneObject
{
public:

    void		turnOn(bool);
    bool		isOn() const;

    void		setIntensity(float);
    			/*!< 0 = nada, 1 = full light */
    float		intensity() const;

    SoNode*		getData();

    virtual void	fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int		usePar( const IOPar& );
protected:
    			Light(SoLight* light_);
    virtual		~Light();

    SoLight*		light;	

    static const char*	isonstr;
    static const char*	intensitystr;
};

/*!\brief
A point that illuminates light

*/

class PointLight : public Light
{
public:
    static PointLight*	create() mCreateDataObj( PointLight );

    void		setPosition(float,float,float);
    float		position(int dim) const;

    void		fillPar( IOPar&, TypeSet<int>& ) const;
    int			usePar( const IOPar& );

protected:
    static const char*	positionstr;

};


/*!\brief
A light in a certain direction from a position at an infinite distance

*/


class DirectionalLight : public Light
{
public:
    static DirectionalLight*	create() mCreateDataObj( DirectionalLight );

    void		setDirection(float,float,float);
    float		direction(int dim) const;

    void		fillPar( IOPar&, TypeSet<int>& ) const;
    int			usePar( const IOPar& );

protected:
    static const char*	directionstr;

};

/*!\brief


*/


class SpotLight : public Light
{
public:
    static SpotLight*	create() mCreateDataObj( SpotLight );

    void		setDirection(float,float,float);
    float		direction(int dim) const;
    
    void		setPosition(float,float,float);
    float		position(int dim) const;

    void		setConeAngle(float);
    			//!< In radians, from one side of the cone to the other
    float		coneAngle() const;

    void		setDropOffRate(float);
    			// 0=smooth, 1=sharp
    float		dropOffRate() const;

    void		fillPar( IOPar&, TypeSet<int>& ) const;
    int			usePar( const IOPar& );

protected:
    static const char*	directionstr;
    static const char*	positionstr;
    static const char*	coneanglestr;
    static const char*	dropoffratestr;
};

};


#endif
