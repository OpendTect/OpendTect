#ifndef mpeengine_h
#define mpeengine_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: mpeengine.h,v 1.5 2005-03-09 16:39:13 cvsnanne Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "callback.h"
#include "color.h"
#include "cubesampling.h"
#include "emposid.h"
#include "trackplane.h"

class AttribSelSpec;
class AttribSliceSet;
class BufferStringSet;
class CubeSampling;
class MultiID;

namespace EM { class EMObject; };
namespace Geometry { class Element; };

namespace MPE
{

class EMTracker;
class TrackerFactory;
class EditorFactory;
class TrackPlane;
class ObjectEditor;

class Engine : public CallBackClass
{
public:
    				Engine();
    virtual			~Engine();

    const CubeSampling&		activeVolume() const;
    void			setActiveVolume(const CubeSampling&);
    static CubeSampling		getDefaultActiveVolume();
    Notifier<Engine>		activevolumechange;

    const TrackPlane&		trackPlane() const;
    bool			setTrackPlane(const TrackPlane&,bool track);
    Notifier<Engine>		trackplanechange;

    ObjectSet<Geometry::Element> interactionseeds;
    Color			seedcolor;
    int				seedsize;
    Notifier<Engine>		seedpropertychange;

    bool			trackAtCurrentPlane();

    void			getAvaliableTrackerTypes(BufferStringSet&)const;

    int				highestTrackerID() const;
    const EMTracker*		getTracker(int idx) const;
    EMTracker*			getTracker(int idx);
    int				getTrackerByObject(const EM::ObjectID&) const;
    int				getTrackerByObject(const char*) const;
    int				addTracker(EM::EMObject*);
    int				addTracker(const char* objectname,
					   const char* trackername);
    void			removeTracker(int idx);

    				/*Attribute stuff */
    void			getNeededAttribs(
	    				ObjectSet<const AttribSelSpec>&) const;
    CubeSampling		getAttribCube(const AttribSelSpec&) const;
    const AttribSliceSet*	getAttribCache(const AttribSelSpec&) const;
    bool			setAttribData(const AttribSelSpec&,
	    				      AttribSliceSet*);

    				/*Editors */
    ObjectEditor*		getEditor(const EM::ObjectID&,bool create);

    				/*Factories */
    void			addTrackerFactory(TrackerFactory*);
    void			addEditorFactory(EditorFactory*);

    const char*			errMsg() const;

    void			fillPar(IOPar&) const		{}
    bool			usePar(const IOPar&)		{ return true; }

protected:
    int				getFreeID();
    BufferString		errmsg;

    CubeSampling		activevolume;
    TrackPlane			trackplane;

    ObjectSet<EMTracker>	trackers;
    ObjectSet<ObjectEditor>	editors;

    ObjectSet<AttribSliceSet>	attribcache;
    ObjectSet<AttribSelSpec>	attribcachespecs;

    ObjectSet<TrackerFactory>	trackerfactories;
    ObjectSet<EditorFactory>	editorfactories;
};


inline Engine& engine()
{
    static MPE::Engine engine_inst;
    return engine_inst;
}


void initStandardClasses();


};

#endif

