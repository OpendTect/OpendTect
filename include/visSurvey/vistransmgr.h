#ifndef vistransmgr_h
#define vistransmgr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2005
 RCS:           $Id: vistransmgr.h,v 1.3 2008-12-10 18:05:30 cvskris Exp $
________________________________________________________________________

-*/


class HorSampling;
namespace visBase { class Transformation; }

namespace visSurvey
{

class Scene;

class SceneTransformManager
{
public:
    				SceneTransformManager()
				    : scene_(0)	{}

    visBase::Transformation*	createZScaleTransform();
    visBase::Transformation*	createUTM2DisplayTransform(const HorSampling&);
    visBase::Transformation*	createIC2DisplayTransform(const HorSampling&);

    void			setZScale(visBase::Transformation*,float);
    float			defZStretch() const	{ return 2; }
    const char*			zStretchStr() const	{ return "Z Stretch"; }
    const char*			zOldStretchStr() const	{ return "Z Scale"; }

    void			setCurrentScene( Scene* scn ) { scene_ = scn; }
    Scene*			currentScene() const	{ return scene_; }

protected:

    Scene*			scene_;
};

SceneTransformManager& STM();

} // namespace visSurvey

#endif
