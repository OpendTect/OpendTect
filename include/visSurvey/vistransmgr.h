#ifndef vistransmgr_h
#define vistransmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "gendefs.h"
#include "vistransform.h"

class TrcKeySampling;
namespace Survey { class Geometry3D; }

namespace visSurvey
{

class Scene;

mExpClass(visSurvey) SceneTransformManager : public CallBacker
{
public:
    			SceneTransformManager()
			    : scene_(0)
			    , mouseCursorCall(this)
    			{}

    static void		computeUTM2DisplayTransform(const Survey::Geometry3D&,
				    float zfactor, float zmidpt,mVisTrans* res);
    			//!<Given to all objects in XY-space

    static void		computeICRotationTransform(const Survey::Geometry3D&,
						   float zfactor, float zmidpt,
					      	   mVisTrans* rotation,
						   mVisTrans* disptrans );

    void		setCurrentScene( Scene* scn ) { scene_ = scn; }
    Scene*		currentScene() const	{ return scene_; }

    Notifier<SceneTransformManager> mouseCursorCall;

protected:

    Scene*		scene_;

};


mGlobal(visSurvey) SceneTransformManager& STM();

} // namespace visSurvey


#endif

