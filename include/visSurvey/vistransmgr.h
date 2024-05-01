#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "gendefs.h"
#include "vissurvscene.h"
#include "vistransform.h"

class TrcKeySampling;
namespace Survey { class Geometry3D; }


namespace visSurvey
{

mExpClass(visSurvey) SceneTransformManager : public CallBacker
{
public:
			SceneTransformManager();
			~SceneTransformManager();

    static void		computeUTM2DisplayTransform(const Survey::Geometry3D&,
				    float zfactor,float zmidpt,mVisTrans* res);
			//!<Given to all objects in XY-space

    static void		computeICRotationTransform(const Survey::Geometry3D&,
						   float zfactor,float zmidpt,
						   mVisTrans* rotation,
						   mVisTrans* disptrans);

    void		setCurrentScene(Scene*);
    RefMan<Scene>	currentScene();
    ConstRefMan<Scene>	currentScene() const;

    Notifier<SceneTransformManager> mouseCursorCall;

private:

    WeakPtr<Scene>	scene_;
};

mGlobal(visSurvey) SceneTransformManager& STM();

} // namespace visSurvey
