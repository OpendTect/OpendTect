#ifndef mpeengine_h
#define mpeengine_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: mpeengine.h,v 1.16 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "callback.h"
#include "color.h"
#include "cubesampling.h"
#include "emposid.h"
#include "trackplane.h"

class BufferStringSet;
class CubeSampling;
class MultiID;

namespace Attrib { class SelSpec; class SliceSet; }
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
    friend Engine&		engine();

public:
    				Engine();
    virtual			~Engine();

    const CubeSampling&		activeVolume() const;
    void			setActiveVolume(const CubeSampling&);
    static CubeSampling		getDefaultActiveVolume();
    Notifier<Engine>		activevolumechange;

    const TrackPlane&		trackPlane() const;
    bool			setTrackPlane(const TrackPlane&,bool track);
    void			setTrackMode(TrackPlane::TrackMode);
    TrackPlane::TrackMode	getTrackMode()
    				{ return trackplane.getTrackMode(); }
    Notifier<Engine>		trackplanechange;

    ObjectSet<Geometry::Element> interactionseeds;
    Color			seedcolor;
    int				seedsize;
    int				seedlinewidth;
    Notifier<Engine>		seedpropertychange;

    bool			trackAtCurrentPlane();
    bool			trackInVolume();
    void			setNewSeeds();

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
				    ObjectSet<const Attrib::SelSpec>&) const;
    CubeSampling		getAttribCube(const Attrib::SelSpec&) const;
    const Attrib::SliceSet*	getAttribCache(const Attrib::SelSpec&) const;
    bool			setAttribData(const Attrib::SelSpec&,
						Attrib::SliceSet*);

    				/*Editors */
    ObjectEditor*		getEditor(const EM::ObjectID&,bool create);
    void			removeEditor(const EM::ObjectID&);

    				/*Factories */
    void			addTrackerFactory(TrackerFactory*);
    void			addEditorFactory(EditorFactory*);

    const char*			errMsg() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    int				getFreeID();

    BufferString		errmsg;
    CubeSampling		activevolume;
    TrackPlane			trackplane;

    ObjectSet<EMTracker>	trackers;
    ObjectSet<ObjectEditor>	editors;

    ObjectSet<Attrib::SliceSet>	attribcache;
    ObjectSet<Attrib::SelSpec>	attribcachespecs;

    ObjectSet<TrackerFactory>	trackerfactories;
    ObjectSet<EditorFactory>	editorfactories;

    static const char*		nrtrackersStr()	{ return "Nr Trackers"; }
    static const char*		objectidStr()	{ return "ObjectID"; }
    static const char*		enabledStr()	{ return "Is enabled"; }
};


void initStandardClasses();


};

#endif

