#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "color.h"
#include "sets.h"
#include "visdatagroup.h"

/*!\brief Open Scene Graph*/

namespace osg { class Group; }

namespace visBase
{
    class SelectionManager;
    class EventCatcher;
    class PolygonOffset;
    class Light;
    class Camera;

/*!\brief
    Scene manages all DataObjects and has some managing
    functions such as the selection management and variables that should
    be common for the whole scene.
*/

mExpClass(visBase) Scene : public DataObjectGroup
{
public:
    static RefMan<Scene> create();
			mCreateDataObj(Scene);

    void		addObject(DataObject*) override;

    void		setBackgroundColor(const OD::Color&);
    OD::Color		getBackgroundColor() const;
    void		setCameraAmbientLight(float);
    float		getCameraAmbientLight() const;
    void		setCameraLightIntensity( float );
    float		getCameraLightIntensity() const;

    Light*		getDirectionalLight();
    const Light*	getDirectionalLight() const;

    PolygonOffset*	getPolygonOffset()	{ return polygonoffset_; }
    bool		saveCurrentOffsetAsDefault() const;

    bool		blockMouseSelection(bool yn);
			/*!<\returns previous status. */

    Camera*		getCamera();
    const Camera*	getCamera() const;
    virtual void	setCamera(Camera*);

    EventCatcher&	eventCatcher();

    int			getUpdateQueueID() const	{return updatequeueid_;}
    int			fixedIdx() const		{ return fixedidx_; }

    float		getPolygonOffsetFactor() const;
    float		getPolygonOffsetUnits() const;

    void		setName(const char*) override;

    Notifier<Scene>	nameChanged;
    Notifier<Scene>	contextIsUp;
			//Trigges when we can ask quesitons about the context

    static const char*	sKeyOffset()	{ return "Polygon offset"; }
    static const char*	sKeyFactor()	{ return "Factor"; }
    static const char*	sKeyUnits()	{ return "Units"; }
    static const char*	sKeyLight()	{ return "Light"; }

protected:
			Scene( bool /*internal*/ )
			    : Scene()		{}
			~Scene();

    RefMan<EventCatcher> events_;

    virtual void	runUpdateQueueCB(CallBacker*);

    void		fillOffsetPar( IOPar& ) const;

private:

    VisID		mousedownid_;
    int			updatequeueid_;
    int			fixedidx_;

    void		mousePickCB(CallBacker*);

    RefMan<PolygonOffset> polygonoffset_;
    RefMan<Light>	light_;

    bool		blockmousesel_ = false;
    osg::Group*		osgsceneroot_ = nullptr;

    RefMan<Camera>	camera_;
};

} // namespace visBase
