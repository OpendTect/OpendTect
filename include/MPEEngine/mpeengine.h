#ifndef mpeengine_h
#define mpeengine_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: mpeengine.h,v 1.55 2012-08-01 10:13:28 cvsmahant Exp $
________________________________________________________________________

-*/

/*!\mainpage
MPE stands for Model, Predict, Edit and is where all tracking and editing
functions are located. The functionality is managed by the MPEEngine, and
a static inistanciation of that can be retrieved by MPE::engine().

*/

#include "attribdataholder.h"
#include "attribdatacubes.h"
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
class TaskRunner;
class MultiID;

namespace Attrib { class SelSpec; }
namespace EM { class EMObject; }
namespace Geometry { class Element; };
template <class T> class Selector;

namespace MPE
{

class EMTracker;
class TrackPlane;
class ObjectEditor;

mClass AbstDataHolder : public CallBacker
{
mRefCountImplNoDestructor(AbstDataHolder);
public:
			AbstDataHolder(){}
};

mClass DataHolder : public AbstDataHolder
{
public:
				DataHolder()
				    : AbstDataHolder()
				    , is2d_( false )
				    , dcdata_(0)
				    , d2dhdata_(0)
				{ cs_.setEmpty(); }
	    
    bool			is2D() const		{ return is2d_; }
    void			setCubeSampling(const CubeSampling cs)
				{ cs_ = cs; }
    CubeSampling		getCubeSampling() const
				{ return cs_; }
    void			set3DData(const Attrib::DataCubes* dc)
				{ is2d_ = false; dcdata_ = dc; }
    const Attrib::DataCubes*	get3DData() const	{ return dcdata_; }
    void			set2DData(const Attrib::Data2DArray* d2h)
				{ is2d_ = true; d2dhdata_ = d2h; }
    const Attrib::Data2DArray*	get2DData() const	{ return d2dhdata_; }
    int			nrCubes() const
				{
				    if ( !dcdata_ && !d2dhdata_ )
					return 0;
				    if ( !is2d_ && dcdata_ )
					return dcdata_->nrCubes();
				    if ( is2d_ && d2dhdata_ )
					return d2dhdata_->nrTraces();

				    return 0;
				}

protected:
    CubeSampling		cs_;
    const Attrib::DataCubes*	dcdata_;
    const Attrib::Data2DArray*	d2dhdata_;
    bool			is2d_;

private:
    void		refNotify() const
    			{
			    if ( !is2d_ && dcdata_ )
				dcdata_->ref();
			    if ( is2d_ && d2dhdata_ )
				d2dhdata_->ref();
			}
    void		unRefNotify() const
			{
			    if ( !is2d_ && dcdata_ )
				dcdata_->unRef();
			    if ( is2d_ && d2dhdata_ )
				d2dhdata_->unRef();
			}
};

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
    void		setActiveVolShown(bool bn)	
    			{ isactivevolshown_ = bn; }
    bool		isActiveVolShown()	{ return isactivevolshown_; }
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
    void		removeSelectionInPolygon(const Selector<Coord3>&,
	    					 TaskRunner*);

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
    const DataHolder*
			getAttribCache(const Attrib::SelSpec&);
    bool		setAttribData( const Attrib::SelSpec&,
	    			       DataPack::ID);
    bool		setAttribData( const Attrib::SelSpec&,
				       const DataHolder*);
    bool		cacheIncludes(const Attrib::SelSpec&,
				      const CubeSampling&);
    void		swapCacheAndItsBackup();

    bool		isSelSpecSame(const Attrib::SelSpec& setupss,
	    			      const Attrib::SelSpec& clickedss) const;

    void		updateFlatCubesContainer(const CubeSampling& cs,
	    					 const int idx,bool);
			/*!< add = true, remove = false. */
    ObjectSet<CubeSampling>* getTrackedFlatCubes(const int idx) const;

    			/*Editors */
    ObjectEditor*	getEditor(const EM::ObjectID&,bool create);
    void		removeEditor(const EM::ObjectID&);

    			/*Fault(StickSet)s workaround, untill
			  2DViewer tree is not in place */
    Notifier<Engine>    activefaultchanged_;
    void		setActiveFaultObjID(EM::ObjectID objid)
			{ 
			    activefaultid_ = objid;
			    activefaultchanged_.trigger();
			}
    EM::ObjectID	getActiveFaultObjID()	{ return activefaultid_; }
    Notifier<Engine>    activefsschanged_;
    void		setActiveFSSObjID(EM::ObjectID objid)
			{
			    activefssid_ = objid;
			    activefsschanged_.trigger();
			}
    EM::ObjectID	getActiveFSSObjID()	{ return activefssid_; }

    const char*		errMsg() const;

    BufferString	setupFileName( const MultiID& ) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    int				getFreeID();
    const DataHolder*    	getAttribCache(DataPack::ID);

    BufferString		errmsg_;
    CubeSampling		activevolume_;
    TrackPlane			trackplane_;
    bool			isactivevolshown_;

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
    ObjectSet<const DataHolder>	attribcache_;
    ObjectSet<CacheSpecs>		attribcachespecs_;
    TypeSet<DataPack::ID>               attribbkpcachedatapackids_;
    ObjectSet<const DataHolder>	attribbackupcache_;
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

    				/*Fault(StickSet)s workaround, untill
				  2DViewer tree is not in place */
    EM::ObjectID		activefaultid_;
    EM::ObjectID		activefssid_;

    static const char*		sKeyNrTrackers(){ return "Nr Trackers"; }
    static const char*		sKeyObjectID()	{ return "ObjectID"; }
    static const char*		sKeyEnabled()	{ return "Is enabled"; }
    static const char*		sKeyTrackPlane(){ return "Track Plane"; }
    static const char*		sKeySeedConMode(){ return "Seed Connect Mode"; }
};


mGlobal Engine&	engine();


};

#endif

