#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "attribsel.h"
#include "emtracker.h"
#include "seisdatapack.h"
#include "survgeom.h"

class BufferStringSet;

template <class T> class Selector;

namespace MPE
{

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

    const TrcKeyZSampling&	activeVolume() const;
    void			setActiveVolume(const TrcKeyZSampling&);
    Notifier<Engine>		activevolumechange;

    void			setActive2DLine(const Pos::GeomID&);
    Pos::GeomID			activeGeomID() const;

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
					const Selector<Coord3>&,TaskRunner*);

				//!< Trackers
    bool			hasTracker() const;
    bool			hasTracker(const EM::ObjectID&) const;
    bool			isActiveTracker(const EM::ObjectID&) const;
    void			setActiveTracker(EMTracker*);
    void			setValidator(TrackSettingsValidator*);
    ConstRefMan<EMTracker>	getActiveTracker() const;
    RefMan<EMTracker>		getActiveTracker();
    ConstRefMan<EMTracker>	getTrackerByID(const EM::ObjectID&) const;
    RefMan<EMTracker>		getTrackerByID(const EM::ObjectID&);
    void			getTrackerIDsByType(const char* typestr,
						TypeSet<EM::ObjectID>&) const;

    CNotifier<Engine,const EM::ObjectID&> trackeradded;

				//!< Editors
    bool			hasEditor(const EM::ObjectID&) const;
    ConstRefMan<ObjectEditor>	getEditorByID(const EM::ObjectID&) const;
    RefMan<ObjectEditor>	getEditorByID(const EM::ObjectID&);

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
    ConstRefMan<SeisDataPack>	getAttribCacheDP(const Attrib::SelSpec&) const;
    bool			hasAttribCache(const Attrib::SelSpec&) const;
    bool			setAttribData(const Attrib::SelSpec&,
					      const FlatDataPack&);
    bool			setAttribData(const Attrib::SelSpec&,
					      const RegularSeisDataPack&);
    bool			cacheIncludes(const Attrib::SelSpec&,
					      const TrcKeyZSampling&);
    void			swapCacheAndItsBackup();

    bool			pickingOnSameData(const Attrib::SelSpec& oldss,
						  const Attrib::SelSpec& newss,
						  uiString& error) const;
    bool			isSelSpecSame(const Attrib::SelSpec& setupss,
					const Attrib::SelSpec& clickedss) const;

    RefMan<FlatDataPack>	getSeedPosDataPack(const TrcKey&,float z,
					int nrtrcs,const ZSampling&) const;

    const char*			errMsg() const;

    BufferString		setupFileName(const MultiID&) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    bool			needRestoredTracker(const MultiID&) const;
    bool			restoreTracker(const EM::ObjectID&);

    const TrcKeyPath*		activePath() const;
    void			setActivePath(const TrcKeyPath*);
    RandomLineID		activeRandomLineID() const;
    void			setActiveRandomLineID(const RandomLineID&);

    Notifier<Engine>		settingsChanged;

private:

    void			cleanup();
    bool			prepareForTrackInVolume(uiString&);
    bool			prepareForRetrack();
    bool			trackInVolume();
    bool			trackFromEdges();
    void			surveyChangedCB(CallBacker*);
    void			trackingFinishedCB(CallBacker*);
    ConstRefMan<EMTracker>	getOneActiveTracker() const;
    RefMan<EM::EMObject>	getCurrentEMObject() const;
    bool			setAttribData_(const Attrib::SelSpec&,
					       const SeisDataPack&);

    BufferString		errmsg_;
    TrcKeyZSampling		activevolume_;
    Pos::GeomID			activegeomid_;

    TrackState			state_				= Stopped;
    mutable WeakPtrSet<EMTracker> trackers_;
    mutable WeakPtrSet<ObjectEditor> editors_;

    WeakPtr<EMTracker>		oneactivetracker_;
    WeakPtr<EMTracker>		activetracker_;
    TrackSettingsValidator*	validator_			= nullptr;
    int				undoeventid_			= -1;
    const TrcKeyPath*		rdmlinetkpath_			= nullptr;
    RandomLineID		rdlid_;

    mClass(MPEEngine) CacheSpecs
    {
    public:
				CacheSpecs(const Attrib::SelSpec&,
					   const Pos::GeomID&);
				~CacheSpecs();

	Attrib::SelSpec		attrsel_;
	WeakPtr<SeisDataPack>	seisdp_;
	Pos::GeomID		geomid_;
    };

    ObjectSet<CacheSpecs>	attribcachespecs_;
    ObjectSet<CacheSpecs>	attribbackupcachespecs_;
    ObjectSet<IOPar>		trackerpars_;
    TypeSet<MultiID>		trackermids_;

    static const char*		sKeyNrTrackers(){ return "Nr Trackers"; }
    static const char*		sKeyObjectID()	{ return "ObjectID"; }
    static const char*		sKeyEnabled()	{ return "Is enabled"; }
    static const char*		sKeyTrackPlane(){ return "Track Plane"; }
    static const char*		sKeySeedConMode(){ return "Seed Connect Mode"; }

public:
				mDeprecatedObs
    void			init()			{ cleanup(); }


};

mGlobal(MPEEngine) Engine&	engine();

} // namespace MPE
