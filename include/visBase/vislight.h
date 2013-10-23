#ifndef vislight_h
#define vislight_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visdata.h"
#include "visosg.h"


class SoLight;
class SoLightModel;


namespace osg
{
    class Light;
}

namespace visBase
{

/*!\brief
Class for all lights. More options are available in osg, but only what we
 currently need is implemented.

*/

mExpClass(visBase) Light
{ mRefCountImplNoDestructor(Light)
public:
			Light();
    void		setLightNum(int);
    int			getLightNum() const;

    void		turnOn(bool n)	{ ison_ = n; updateLights(); }
    bool		isOn() const	{ return ison_; }

    void		setAmbient(float);
    			/*!< 0 = nada, 1 = full light */
    float		getAmbient() const;

    void		setDiffuse(float);
			/*!< 0 = nada, 1 = full light */
    float		getDiffuse() const;

    void		setDirection(float,float,float);
    float		direction(int dim) const;

    void		fillPar( IOPar& ) const;
    bool		usePar( const IOPar& );

    osg::Light*		osgLight()	{ return light_; }

protected:

    void			updateLights();

    bool			ison_;
    float			ambient_;
    float			diffuse_;
    osg::Light*			light_;

    static const char*	sKeyIsOn();
    static const char*	sKeyAmbient();
    static const char*	sKeyDiffuse();
    static const char*	sKeyLightNum();
    static const char*	sKeyDirection();
};


} //visBase

#endif

