#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "attribsel.h"
#include "callback.h"
#include "datapack.h"
#include "emposid.h"
#include "integerid.h"
#include "survgeom.h"

class BufferStringSet;
class Executor;
class FlatDataPack;

namespace EM { class EMObject; }
namespace Geometry { class Element; }
template <class T> class Selector;

namespace MPE
{

class EMTracker;
class HorizonTrackerMgr;
class ObjectEditor;

mExpClass(MPEEngine) TrackSettingsValidator
{
public:
    virtual		~TrackSettingsValidator();

    virtual bool	checkInVolumeTrackMode() const			= 0;
    virtual bool	checkActiveTracker() const			= 0;
    virtual bool	checkStoredData(Attrib::SelSpec&,MultiID&) const = 0;
    virtual bool	checkPreloadedData(const MultiID&) const	= 0;

protected:
			TrackSettingsValidator();
};


/*!
\brief Main engine for tracking EM objects like horizons, faults etc.,
*/

mExpClass(MPEEngine) Engine : public CallBacker
{ mODTextTranslationClass(Engine)
    mGlobal(MPEEngine) friend Engine&		engine();

public:
				Engine();
    virtual			~Engine();

    void			init();

    const TrcKeyZSampling&	activeVolume() const;
    void			setActiveVolume(const TrcKeyZSampling&);
    Notifier<Engine>		activevolumechange;

    void			setActive2DLine(Pos::GeomID);
    Pos::GeomID			activeGeomID() const;

    Notifier<Engine>		loadEMObject;
    MultiID			midtoload;

    void			updateSeedOnlyPropagation(bool);

    enum TrackState		{ Started, Paused, Stopped };
    TrackState			getState() const	{ return state_; }
    bool			startTracking(uiString&);
    bool			startRetrack(uiString&);
    bool			startFromEdges(uiString&);
    void			stopTracking();
    bool			trackingInProgress() const;
    void			undo(uiString& errmsg);
    void			redo(uiString& errmsg);
    bool			canUnDo();
    bool			canReDo();
    void			enableTracking(bool yn);
    Notifier<Engine>		actionCalled;
    Notifier<Engine>		actionFinished;

    void			removeSelectionInPolygon(
					const Selector<Coord3>&,
					TaskRunner*);
    void			getAvailableTrackerTypes(BufferStringSet&)const;

    int				nrTrackersAlive() const;
    int				highestTrackerID() const;
    const EMTracker*		getTracker(int idx) const;
    EMTracker*			getTracker(int idx);
    int				getTrackerByObject(const EM::ObjectID&) const;
    int				getTrackerByObject(const char*) const;
    int				addTracker(EM::EMObject*);
    void			removeTracker(int idx);
    Notifier<Engine>		trackeraddremove;
    void			setActiveTracker(const EM::ObjectID&);
    void			setActiveTracker(EMTracker*);
    EMTracker*			getActiveTracker();

				/*Attribute stuff */
    void			setOneActiveTracker(const EMTracker*);
    void			unsetOneActiveTracker();
    void			getNeededAttribs(
				TypeSet<Attrib::SelSpec>&) const;
    TrcKeyZSampling		getAttribCube(const Attrib::SelSpec&) const;
				/*!< Returns the cube that is needed for
				     this attrib, given that the activearea
				     should be tracked. */
    int				getCacheIndexOf(const Attrib::SelSpec&) const;
    DataPackID		getAttribCacheID(const Attrib::SelSpec&) const;
    bool			hasAttribCache(const Attrib::SelSpec&) const;
    bool			setAttribData( const Attrib::SelSpec&,
					       DataPackID);
    bool			cacheIncludes(const Attrib::SelSpec&,
					      const TrcKeyZSampling&);
    void			swapCacheAndItsBackup();

    bool			pickingOnSameData(const Attrib::SelSpec& oldss,
						  const Attrib::SelSpec& newss,
						  uiString& error) const;
    bool			isSelSpecSame(const Attrib::SelSpec& setupss,
					const Attrib::SelSpec& clickedss) const;

    void			updateFlatCubesContainer(const TrcKeyZSampling&,
							 int idx,bool);
				/*!< add = true, remove = false. */
    ObjectSet<TrcKeyZSampling>* getTrackedFlatCubes(const int idx) const;
    RefMan<FlatDataPack>	getSeedPosDataPackRM(const TrcKey&,float z,
					int nrtrcs,
					const StepInterval<float>& zrg) const;
    DataPackID		getSeedPosDataPack(const TrcKey&,float z,
					int nrtrcs,
					const StepInterval<float>& zrg) const;

				/*Editors */
    ObjectEditor*		getEditor(const EM::ObjectID&,bool create);
    void			removeEditor(const EM::ObjectID&);

    void			setValidator(TrackSettingsValidator*);
    const char*			errMsg() const;

    BufferString		setupFileName(const MultiID&) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    const TrcKeyPath*		activePath() const;
    void			setActivePath(const TrcKeyPath*);
    RandomLineID		activeRandomLineID() const;
    void			setActiveRandomLineID(RandomLineID);
    void			refTracker(EM::ObjectID);
    void			unRefTracker(EM::ObjectID,bool nodel=false);
    bool			hasTracker(EM::ObjectID) const;

    Notifier<Engine>		settingsChanged;

protected:

    BufferString		errmsg_;
    TrcKeyZSampling		activevolume_;

    Pos::GeomID			activegeomid_;

    TrackState			state_;
    ObjectSet<HorizonTrackerMgr> trackermgrs_;
    ObjectSet<EMTracker>	trackers_;
    ObjectSet<ObjectEditor>	editors_;

    const EMTracker*		oneactivetracker_		= nullptr;
    EMTracker*			activetracker_			= nullptr;
    int				undoeventid_			= -1;
    DataPackMgr&		dpm_;
    const TrcKeyPath*		rdmlinetkpath_			= nullptr;
    RandomLineID		rdlid_;
    TrackSettingsValidator*	validator_			= nullptr;

    bool			prepareForTrackInVolume(uiString&);
    bool			prepareForRetrack();
    bool			trackInVolume();
    bool			trackFromEdges();
    void			trackingFinishedCB(CallBacker*);
    EM::EMObject*		getCurrentEMObject() const;


    struct CacheSpecs
    {
				CacheSpecs(const Attrib::SelSpec& as,
					Pos::GeomID geomid=
					Survey::GeometryManager::cUndefGeomID())
				    : attrsel_(as),geomid_(geomid)
				{}

	Attrib::SelSpec		attrsel_;
	Pos::GeomID		geomid_;
    };

    TypeSet<DataPackID>		attribcachedatapackids_;
    ObjectSet<CacheSpecs>	attribcachespecs_;
    TypeSet<DataPackID>		attribbkpcachedatapackids_;
    ObjectSet<CacheSpecs>	attribbackupcachespecs_;

    mStruct(MPEEngine) FlatCubeInfo
    {
				FlatCubeInfo()
				:nrseeds_(1)
				{
				    flatcs_.setEmpty();
				}
	TrcKeyZSampling		flatcs_;
	int			nrseeds_;
    };

    ObjectSet<ObjectSet<FlatCubeInfo> >	flatcubescontainer_;

    static const char*		sKeyNrTrackers(){ return "Nr Trackers"; }
    static const char*		sKeyObjectID()	{ return "ObjectID"; }
    static const char*		sKeyEnabled()	{ return "Is enabled"; }
    static const char*		sKeyTrackPlane(){ return "Track Plane"; }
    static const char*		sKeySeedConMode(){ return "Seed Connect Mode"; }

    void			applClosingCB(CallBacker*);
};

mGlobal(MPEEngine) Engine&	engine();

} // namespace MPE
