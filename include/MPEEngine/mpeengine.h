#ifndef mpeengine_h
#define mpeengine_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: mpeengine.h,v 1.1 2005-01-06 09:25:55 kristofer Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "callback.h"
#include "color.h"
#include "cubesampling.h"
#include "emposid.h"
#include "sets.h"
#include "trackplane.h"

class AttribSelSpec;
class AttribSliceSet;
class CubeSampling;
class MultiID;
class BufferStringSet;

namespace EM { class EMObject; };
namespace Geometry { class Element; };

namespace MPE
{

class EMTracker;
class TrackerFactory;
class TrackPlane;

class Engine : public CallBackClass
{
public:
    			Engine();
    virtual		~Engine();

    const CubeSampling&	activeVolume() const;
    void		setActiveVolume(const CubeSampling&);

    const TrackPlane&	trackPlane() const;
    bool		setTrackPlane( const TrackPlane&, bool track );

    ObjectSet<Geometry::Element>	interactionseeds;
    Color				seedcolor;
    int					seedsize;
    Notifier<Engine>			seedpropertychange;

    bool		trackAtCurrentPlane();

    void		getAvaliableTrackerTypes(BufferStringSet&)const;

    int			highestTrackerID() const;
    const EMTracker*	getTracker( int idx ) const;
    EMTracker*		getTracker( int idx );
    int			getTrackerByObject( const EM::ObjectID& ) const;
    int			getTrackerByObject( const char* ) const;
    int			addTracker( EM::EMObject* );
    int			addTracker( const char* objectname,
				    const char* trackername);
    void		removeTracker( int idx );

    			/*Attribute stuff */
    void		getNeededAttribs(ObjectSet<const AttribSelSpec>&) const;
    CubeSampling	getAttribCube(const AttribSelSpec&) const;
    const AttribSliceSet* getAttribCache( const AttribSelSpec& ) const;
    bool		setAttribData(const AttribSelSpec&, AttribSliceSet*);

    void		addTrackerFactory( TrackerFactory* );

    const char*		errMsg() const;

    void		fillPar( IOPar& ) const {}
    bool		usePar( const IOPar& ) { return true; }

protected:
    int			getFreeID();
    BufferString	errmsg;

    CubeSampling	activevolume;
    TrackPlane		trackplane;
    ObjectSet<EMTracker> trackers;

    ObjectSet<AttribSliceSet>	attribcache;
    ObjectSet<AttribSelSpec>	attribcachespecs;

    ObjectSet<TrackerFactory>	trackerfactories;
};


inline Engine& engine()
{
    static MPE::Engine engine_inst;
    return engine_inst;
}


void initStandardClasses();


};

#endif

