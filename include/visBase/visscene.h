#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "sets.h"
#include "visdatagroup.h"

/*!\brief Open Scene Graph*/

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
    static Scene*	create()
			mCreateDataObj(Scene);

    void		addObject(DataObject*);

    void		setAmbientLight(float);
    float		ambientLight() const;

    Light*		getLight() const;

    PolygonOffset*	getPolygonOffset()	{ return polygonoffset_; }
    bool		saveCurrentOffsetAsDefault() const;

    bool		blockMouseSelection(bool yn);
    			/*!<\returns previous status. */

    Camera*		getCamera() 			{ return camera_; }
    const Camera*	getCamera() const 		{ return camera_; }
    virtual void	setCamera(Camera*);

    EventCatcher&	eventCatcher();

    void		setName(const char*);

    Notifier<Scene>	nameChanged;

    static const char*	sKeyOffset()	{ return "Polygon offset"; }
    static const char*	sKeyFactor()	{ return "Factor"; }
    static const char*	sKeyUnits()	{ return "Units"; }
    static const char*	sKeyLight()	{ return "Light"; }


protected:
    virtual		~Scene();
    EventCatcher&	events_;

    void		fillOffsetPar( IOPar& ) const;

private:

    int			mousedownid_;

    void		mousePickCB(CallBacker*);

    PolygonOffset*	polygonoffset_;
    Light*		light_;

    bool		blockmousesel_;
    osg::Group*		osgsceneroot_;

    Camera*		camera_;
};

}

#endif

