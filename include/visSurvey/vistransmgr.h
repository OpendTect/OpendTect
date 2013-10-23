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
#include "commondefs.h"
#include "vistransform.h"

class HorSampling;
class InlCrlSystem;

namespace visSurvey
{

class Scene;

mExpClass(visSurvey) SceneTransformManager
{
public:
    			SceneTransformManager()
			    : scene_(0)
    			{}

    static void		computeUTM2DisplayTransform(const InlCrlSystem&,
				    float zfactor, mVisTrans* res);
    			//!<Given to all objects in XY-space

    static void		computeICRotationTransform(const InlCrlSystem&,
						   float zfactor,
					      	   mVisTrans* rotation,
						   mVisTrans* disptrans );

    void		setCurrentScene( Scene* scn ) { scene_ = scn; }
    Scene*		currentScene() const	{ return scene_; }

protected:

    Scene*		scene_;
};


mGlobal(visSurvey) SceneTransformManager& STM();

} // namespace visSurvey

#endif

