#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visdata.h"
#include "visnodestate.h"
#include "visosg.h"


namespace osg { class Light; }

namespace visBase
{

/*!\brief
Class for all lights. More options are available in osg, but only what we
 currently need is implemented.

*/
mExpClass( visBase ) Light : public NodeState
{
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

    void		setDirection(float x,float y,float z);
			/*!< the direction is specified towards light source*/
    float		direction(int dim) const;
			/*!< the direction is specified towards light source*/

    void		fillPar( IOPar& ) const;
    bool		usePar( const IOPar& );


protected:
				~Light();
    void			updateLights();
    void			initLight();
    bool			ison_;
    float			ambient_;
    float			diffuse_;
    osg::Light*			light_;

    static const char*	sKeyIsOn();
    static const char*	sKeyAmbient();
    static const char*	sKeyDiffuse();
    static const char*	sKeyLightNum();
    static const char*	sKeyDirection();
private:
    void			applyAttribute(osg::StateSet*,
					       osg::StateAttribute*) override;
};

} // namespace visBase
