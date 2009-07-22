#ifndef mpeengine_h
#define mpeengine_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: mpeengine.h,v 1.46 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

/*!\mainpage
MPE stands for Model, Predict, Edit and is where all tracking and editing
functions are located. The functionality is managed by the MPEEngine, and
a static inistanciation of that can be retrieved by MPE::engine().

*/

#include "attribsel.h"
#include "bufstring.h"
#include "callback.h"
#include "color.h"
#include "cubesampling.h"
#include "datapack.h"
#include "emposid.h"
#include "trackplane.h"

class BufferStringSet;
class Executor;
class CubeSampling;
class MultiID;

namespace Attrib { class SelSpec; class DataCubes; }
namespace EM { class EMObject; };
namespace Geometry { class Element; };
template <class T> class Selector;

namespace MPE
{

class EMTracker;
class TrackPlane;
class ObjectEditor;

mClass Engine : public CallBacker
{
    mGlobal friend Engine&		engine();

public:
    				Engine();
    virtual			~Engine();

    void		init();

    const CubeSampling&	activeVolume() const;
    void		setActiveVolume(const CubeSampling&);
    static CubeSampling	getDefaultActiveVolume();
    Notifier<Engine>	activevolumechange;

    void		setActive2DLine(const MultiID& linesetid,
	    				const char* linename);
    const MultiID&	active2DLineSetID() const;
    const BufferString&	active2DLineName() const;
    
    const TrackPlane&	trackPlane() const;
    bool		setTrackPlane(const TrackPlane&,bool track);
    void		setTrackMode(TrackPlane::TrackMode);
    TrackPlane::TrackMode getTrackMode() { return trackplane_.getTrackMode(); }
    Notifier<Engine>	trackplanechange;
    Notifier<Engine>	trackplanetrack;

    Notifier<Engine>	loadEMObject;
    MultiID		midtoload;

    bool		trackAtCurrentPlane();
    void		updateSeedOnlyPropagation(bool);
    Executor*		trackInVolume();
    void		removeSelectionInPolygon(const Selector<Coord3>&);

    void		getAvailableTrackerTypes(BufferStringSet&)const;

    int			nrTrackersAlive() const;
    int			highestTrackerID() const;
    const EMTracker*	getTracker(int idx) const;
    EMTracker*		getTracker(int idx);
    int			getTrackerByObject(const EM::ObjectID&) const;
    int			getTrackerByObject(const char*) const;
    int			addTracker(EM::EMObject*);
    void		removeTracker(int idx);
    Notifier<Engine>	trackeraddremove;
    void		setActiveTracker(const EM::ObjectID&);
    void		setActiveTracker(EMTracker*);
    EMTracker*		getActiveTracker();


    			/*Attribute stuff */
    void 		setOneActiveTracker(const EMTracker*);
    void 		unsetOneActiveTracker();
    void		getNeededAttribs(
			    ObjectSet<const Attrib::SelSpec>&) const;
    CubeSampling	getAttribCube(const Attrib::SelSpec&) const;
    			/*!< Returns the cube that is needed for
			     this attrib, given that the activearea
			     should be tracked. */
    int			getCacheIndexOf(const Attrib::SelSpec&) const;
    DataPack::ID	getAttribCacheID(const Attrib::SelSpec&) const;
    const Attrib::DataCubes*
			getAttribCache(const Attrib::SelSpec&);
    bool		setAttribData( const Attrib::SelSpec&,
	    			       DataPack::ID);
    bool		setAttribData( const Attrib::SelSpec&,
				       const Attrib::DataCubes*);
    bool		cacheIncludes(const Attrib::SelSpec&,
				      const CubeSampling&);
    void		swapCacheAndItsBackup();

    void		updateFlatCubesContainer(const CubeSampling& cs,
	    					 const int idx,bool);
			/*!< add = true, remove = false. */
    ObjectSet<CubeSampling>* getTrackedFlatCubes(const int idx) const;

    			/*Editors */
    ObjectEditor*	getEditor(const EM::ObjectID&,bool create);
    void		removeEditor(const EM::ObjectID&);

    const char*		errMsg() const;

    BufferString	setupFileName( const MultiID& ) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    int				getFreeID();
    const Attrib::DataCubes*    getAttribCache(DataPack::ID);

    BufferString		errmsg_;
    CubeSampling		activevolume_;
    TrackPlane			trackplane_;

    MultiID			active2dlinesetid_;
    BufferString		active2dlinename_;

    ObjectSet<EMTracker>	trackers_;
    ObjectSet<ObjectEditor>	editors_;

    const EMTracker*		oneactivetracker_;
    EMTracker*			activetracker_;

    struct CacheSpecs
    {
				CacheSpecs(const Attrib::SelSpec& as,
					   const MultiID& id=MultiID(-1),
					   const char* nm=0)
				    : attrsel_(as),linesetid_(id),linename_(nm)
				{}
				
	Attrib::SelSpec		attrsel_;
	MultiID			linesetid_;
	BufferString		linename_;
    };

    TypeSet<DataPack::ID>  		attribcachedatapackids_;   
    ObjectSet<const Attrib::DataCubes>	attribcache_;
    ObjectSet<CacheSpecs>		attribcachespecs_;
    TypeSet<DataPack::ID>               attribbkpcachedatapackids_;
    ObjectSet<const Attrib::DataCubes>	attribbackupcache_;
    ObjectSet<CacheSpecs>		attribbackupcachespecs_;

    mStruct FlatCubeInfo
    {
				FlatCubeInfo()
				:nrseeds_(1)
				{
				    flatcs_.setEmpty();
				}
	CubeSampling		flatcs_;
	int			nrseeds_;	
    };

    ObjectSet<ObjectSet<FlatCubeInfo> >	flatcubescontainer_;

    static const char*		sKeyNrTrackers(){ return "Nr Trackers"; }
    static const char*		sKeyObjectID()	{ return "ObjectID"; }
    static const char*		sKeyEnabled()	{ return "Is enabled"; }
    static const char*		sKeyTrackPlane(){ return "Track Plane"; }
    static const char*		sKeySeedConMode(){ return "Seed Connect Mode"; }
};


mGlobal Engine&	engine();


};

#endif

