#ifndef vislight_h
#define vislight_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vislight.h,v 1.1 2002-02-09 13:38:42 kristofer Exp $
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
    virtual		~Light();

    void		turnOn(bool);
    bool		isOn() const;

    void		setIntensity(float);
    			/*!< 0 = nada, 1 = full light */
    float		intensity() const;

    SoNode*		getData();
protected:
    			Light(SoLight* light_);
    SoLight*		light;	
};

/*!\brief
A point that illuminates light

*/

class PointLight : public Light
{
public:
    			PointLight();
    void		setPosition(float,float,float);
    float		position(int dim) const;
};


/*!\brief
A light in a certain direction from a position at an infinite distance

*/


class DirectionalLight : public Light
{
public:
    			DirectionalLight();
    void		setDirection(float,float,float);
    float		direction(int dim) const;
};

/*!\brief


*/


class SpotLight : public Light
{
public:
    			SpotLight();

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
};

};


#endif
