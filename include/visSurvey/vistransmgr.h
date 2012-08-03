#ifndef vistransmgr_h
#define vistransmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2005
 RCS:           $Id: vistransmgr.h,v 1.9 2012-08-03 13:01:29 cvskris Exp $
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "commondefs.h"
#include "vistransform.h"

class HorSampling;

namespace visSurvey
{

class Scene;

mClass(visSurvey) SceneTransformManager
{
public:
    			SceneTransformManager()
			    : scene_(0)	{}

    mVisTrans*		createZScaleTransform() const;
    mVisTrans*		createUTM2DisplayTransform(const HorSampling&) const;
    mVisTrans*		createIC2DisplayTransform(const HorSampling&) const;

    void		setIC2DispayTransform(const HorSampling&,
					      mVisTrans*) const;

    void		setZScale(mVisTrans*,float) const;
    float		defZStretch() const	{ return 2; }
    const char*		zStretchStr() const	{ return "Z Stretch"; }
    const char*		zOldStretchStr() const	{ return "Z Scale"; }

    void		setCurrentScene( Scene* scn ) { scene_ = scn; }
    Scene*		currentScene() const	{ return scene_; }

protected:

    Scene*		scene_;
};


mGlobal(visSurvey) SceneTransformManager& STM();

} // namespace visSurvey

#endif

