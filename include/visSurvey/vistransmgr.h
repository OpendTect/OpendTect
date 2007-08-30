#ifndef vistransmgr_h
#define vistransmgr_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2005
 RCS:           $Id: vistransmgr.h,v 1.2 2007-08-30 14:31:36 cvskris Exp $
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
    float			defZScale() const	{ return 2; }
    const char*			zScaleStr() const	{ return "Z Scale"; }

    void			setCurrentScene( Scene* scn ) { scene_ = scn; }
    Scene*			currentScene() const	{ return scene_; }

protected:

    Scene*			scene_;
};

SceneTransformManager& STM();

} // namespace visSurvey

#endif
