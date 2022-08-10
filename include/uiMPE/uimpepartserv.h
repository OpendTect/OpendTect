#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2004
________________________________________________________________________

-*/

#include "uimpemod.h"
#include "attribsel.h"
#include "uiapplserv.h"
#include "multiid.h"
#include "trckeyzsampling.h"
#include "datapack.h"
#include "emposid.h"
#include "emtracker.h"

class BufferStringSet;

namespace Geometry { class Element; }
namespace MPE { class uiSetupGroup; class DataHolder; }
namespace Attrib { class DescSet; class DataCubes; class Data2DArray; }


/*! \brief Implementation of Tracking part server interface */

mExpClass(uiMPE) uiMPEPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiMPEPartServer);
public:
				uiMPEPartServer(uiApplService&);
				~uiMPEPartServer();
    void			setCurrentAttribDescSet(const Attrib::DescSet*);
    const Attrib::DescSet* 	getCurAttrDescSet(bool is2d) const;

    const char*			name() const override		{ return "MPE";}

    int				getTrackerID(const EM::ObjectID&) const;
    int				getTrackerID(const char* name) const;
    void			getTrackerTypes(BufferStringSet&) const;
    bool			addTracker(const char* trackertype,
					   SceneID sceneid);
    int				addTracker(const EM::ObjectID&,
					   const Coord3& pos);
				/*!<Creates a new tracker for the object and
				    returns the trackerid of it or -1 if it
				    failed.
				    \param pos should contain the clicked
					   position. If the activevolume is not
					   set before, it will be centered
					   pos, otherwise, it will be expanded
					   to include pos. */
    EM::ObjectID		getEMObjectID(int trackerid) const;
    SceneID			getCurSceneID() const { return cursceneid_; }

    bool			canAddSeed(int trackerid) const;

    void			enableTracking(int trackerid,bool yn);
    bool			isTrackingEnabled(int trackerid) const;

    bool			showSetupDlg(const EM::ObjectID&,
					     const EM::SectionID&);
				/*!<\returns false if cancel was pressed. */
    bool			showSetupGroupOnTop(const EM::ObjectID&,
						    const char* grpnm);
    void			useSavedSetupDlg(const EM::ObjectID&,
						 const EM::SectionID&);
    MPE::uiSetupGroup*		getSetupGroup()	{ return setupgrp_; }

    int				activeTrackerID() const;
				/*!< returns the trackerid of the last event */

    static int			evGetAttribData();
    bool			is2D() const;
				/*!<If attrib is 2D, check for a selspec. If
				    selspec is returned, calculate the attrib.
				    If no selspec is present, use getLineSet,
				    getLineName & getAttribName. */
    TrcKeyZSampling		getAttribVolume(const Attrib::SelSpec&) const;
				/*!<\returns the volume needed of an
					     attrib if tracking should
					     be possible in the activeVolume. */
    const Attrib::SelSpec*	getAttribSelSpec() const;
    void			setAttribData(const Attrib::SelSpec&,
					      DataPack::ID);

    static int			evCreate2DSelSpec();
    Pos::GeomID 		getGeomID() const;
    const char*			get2DLineName() const;
    const char*			get2DAttribName() const;
    void			set2DSelSpec(const Attrib::SelSpec&);

    static int			evStartSeedPick();
    static int			evEndSeedPick();

    static int			evAddTreeObject();
				/*!<Get trackerid via activeTrackerID */
    static int			evRemoveTreeObject();
				/*!<Get trackerid via activeTrackerID */
    static int			evUpdateTrees();
    static int			evUpdateSeedConMode();
    static int			evStoreEMObject();
    static int			evSetupLaunched();
    static int			evSetupClosed();
    static int			evInitFromSession();
    static int			evHorizonTracking();

    void			loadTrackSetupCB(CallBacker*);
    bool 			prepareSaveSetupAs(const MultiID&);
    bool 			saveSetupAs(const MultiID&);
    bool 			saveSetup(const MultiID&);
    bool 			readSetup(const MultiID&);

    bool			sendMPEEvent(int);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    void			activeVolumeChange(CallBacker*);
    void			loadEMObjectCB(CallBacker*);
    void			mergeAttribSets(const Attrib::DescSet& newads,
						MPE::EMTracker&);
    bool			initSetupDlg(EM::EMObject*& emobj,
					     MPE::EMTracker*& tracker,
					     const EM::SectionID& sid,
					     bool freshdlg=false);

    const Attrib::DescSet*	attrset3d_;
    const Attrib::DescSet*	attrset2d_;

				//Interaction variables
    const Attrib::SelSpec*	eventattrselspec_;
    int				activetrackerid_;
    int				temptrackerid_;
    SceneID			cursceneid_;

				//2D interaction
    Pos::GeomID 		geomid_;
    Attrib::SelSpec		lineselspec_;

    void			aboutToAddRemoveSeed(CallBacker*);
    void			seedAddedCB(CallBacker*);
    EM::ObjectID		trackercurrentobject_;
    void			trackerWinClosedCB(CallBacker*);

    int				initialundoid_;
    bool			seedhasbeenpicked_;
    bool			setupbeingupdated_;
    bool			seedswithoutattribsel_;

    void			modeChangedCB(CallBacker*);
    void			eventChangedCB(CallBacker*);
    void			propertyChangedCB(CallBacker*);
    void			correlationChangedCB(CallBacker*);
    void			settingsChangedCB(CallBacker*);

    void			nrHorChangeCB(CallBacker*);

    void			noTrackingRemoval();
    void			cleanSetupDependents();

    MPE::uiSetupGroup*		setupgrp_;

private:
    static uiString		sYesAskGoOnStr();
    static uiString		sNoAskGoOnStr();

public:
    void			fillTrackerSettings(int trackerid);
};

