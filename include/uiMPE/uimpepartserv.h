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
#include "dbkey.h"
#include "trckeyzsampling.h"
#include "datapack.h"
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

    const char*			name() const		{ return "MPE";}

    int				getTrackerID(const DBKey&) const;
    int				getTrackerID(const char* name) const;
    void			getTrackerTypes(BufferStringSet&) const;
    bool			addTracker(const char* trackertype,int sceneid);
    bool			addTracker(const DBKey&,int sceneid);
    int				addTracker(const DBKey&);
				/*!<Creates a new tracker for the object and
				    returns the trackerid of it or -1 if it
				    failed.*/
    DBKey			getEMObjectID(int trackerid) const;
    int				getCurSceneID() const { return cursceneid_; }

    bool			canAddSeed(int trackerid) const;

    void			enableTracking(int trackerid,bool yn);
    bool			isTrackingEnabled(int trackerid) const;

    bool			showSetupDlg(const DBKey&);
				/*!<\returns false if cancel was pressed. */
    bool			showSetupGroupOnTop(const DBKey&,
						    const char* grpnm);
    void			useSavedSetupDlg(const DBKey&);
    MPE::uiSetupGroup*		getSetupGroup()	{ return setupgrp_; }
    void			fillTrackerSettings(int trackerid);

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
    Pos::GeomID		getGeomID() const;
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

    bool			prepareSaveSetupAs(const DBKey&);
    bool			saveSetupAs(const DBKey&);
    bool			saveSetup(const DBKey&);
    bool			readSetup(const DBKey&);

    bool			sendMPEEvent(int);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    void			activeVolumeChange(CallBacker*);
    void			loadEMObjectCB(CallBacker*);
    void			mergeAttribSets(const Attrib::DescSet& newads,
						MPE::EMTracker&);
    bool			initSetupDlg(EM::Object*& emobj,
					     MPE::EMTracker*& tracker,
					     bool freshdlg=false);

				//Interaction variables
    const Attrib::SelSpec*	eventattrselspec_;
    int				activetrackerid_;
    int				temptrackerid_;
    int				cursceneid_;

				//2D interaction
    Pos::GeomID		geomid_;
    Attrib::SelSpec		lineselspec_;

    void			trackerAddRemoveCB(CallBacker*);
    void			trackerToBeRemovedCB(CallBacker*);
    void			aboutToAddRemoveSeed(CallBacker*);
    void			seedAddedCB(CallBacker*);
    DBKey			trackercurrentobject_;
    void			trackerWinClosedCB(CallBacker*);

    int				initialundoid_;

    void			modeChangedCB(CallBacker*);
    void			eventChangedCB(CallBacker*);
    void			propertyChangedCB(CallBacker*);
    void			correlationChangedCB(CallBacker*);

    void			nrHorChangeCB(CallBacker*);

    void			cleanSetupDependents();

    MPE::uiSetupGroup*		setupgrp_;
};
