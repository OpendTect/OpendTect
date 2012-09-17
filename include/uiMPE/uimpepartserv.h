#ifndef uimpepartserv_h
#define uimpepartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2004
 RCS:           $Id: uimpepartserv.h,v 1.52 2011/09/19 12:23:22 cvskris Exp $
________________________________________________________________________

-*/

#include "attribsel.h"
#include "uiapplserv.h"
#include "multiid.h"
#include "cubesampling.h"
#include "datapack.h"
#include "emposid.h"
#include "emtracker.h"

class BufferStringSet;
class uiDialog;

namespace Geometry { class Element; }
namespace MPE { class uiSetupGroup; class DataHolder; }
namespace Attrib { class DescSet; class DataCubes; class Data2DArray; }


/*! \brief Implementation of Tracking part server interface */

mClass uiMPEPartServer : public uiApplPartServer
{
public:
				uiMPEPartServer(uiApplService&);
				~uiMPEPartServer();
    void			setCurrentAttribDescSet(const Attrib::DescSet*);
    const Attrib::DescSet* 	getCurAttrDescSet(bool is2d) const;

    const char*			name() const		{ return "MPE";}

    int				getTrackerID(const EM::ObjectID&) const;
    int				getTrackerID(const char* name) const;
    void			getTrackerTypes(BufferStringSet&) const;
    bool			addTracker(const char* trackertype,int sceneid);
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
    int				getCurSceneID() const { return cursceneid_; }

    bool			canAddSeed(int trackerid) const;

    void			enableTracking(int trackerid,bool yn);
    bool			isTrackingEnabled(int trackerid) const;

    bool			showSetupDlg( const EM::ObjectID&,
	    				      const EM::SectionID&);
    				/*!<\returns false if cancel was pressed. */
    void			useSavedSetupDlg(const EM::ObjectID&,
	    					 const EM::SectionID&);

    int				activeTrackerID() const;
    				/*!< returns the trackerid of the last event */

    static const int		evGetAttribData();
    bool			is2D() const;
    				/*!<If attrib is 2D, check for a selspec. If
				    selspec is returned, calculate the attrib.
				    If no selspec is present, use getLineSet,
				    getLineName & getAttribName. */
    CubeSampling		getAttribVolume(const Attrib::SelSpec&) const;
    				/*!<\returns the volume needed of an
				 	     attrib if tracking should
					     be possible in the activeVolume. */
    const Attrib::SelSpec*	getAttribSelSpec() const;
    DataPack::ID		getAttribCacheID(const Attrib::SelSpec&) const;
    const MPE::DataHolder*	getAttribCache(const Attrib::SelSpec&) const;
    void			setAttribData(const Attrib::SelSpec&,
	    				      DataPack::ID);
    void			setAttribData(const Attrib::SelSpec&,
					      const Attrib::DataCubes*);
    void			setAttribData(const Attrib::SelSpec&,
					      const Attrib::Data2DArray*);

    static const int		evCreate2DSelSpec();
    const MultiID&		get2DLineSet() const;
    const char*			get2DLineName() const;
    const char*			get2DAttribName() const;
    void			set2DSelSpec(const Attrib::SelSpec&);

    static const int		evStartSeedPick();
    static const int		evEndSeedPick();

    static const int		evAddTreeObject();
    				/*!<Get trackerid via activeTrackerID */
    static const int		evRemoveTreeObject();
    				/*!<Get trackerid via activeTrackerID */
    static const int		evUpdateTrees();
    static const int		evUpdateSeedConMode();
    static const int		evShowToolbar();
    static const int		evMPEDispIntro();
    static const int		evMPEStoreEMObject();
    static const int		evSetupLaunched();
    static const int		evSetupClosed();
    static const int		evInitFromSession();
    static const int		evHideToolBar();
    static const int		evSaveUnsavedEMObject();
    static const int		evRemoveUnsavedEMObject();
    static const int		evRetrackInVolume();

    bool			isDataLoadingBlocked() const;
    void			blockDataLoading(bool);
    void			postponeLoadingCurVol();
    void			loadPostponedVolume();

    void			loadTrackSetupCB(CallBacker*);
    bool 			prepareSaveSetupAs(const MultiID&);
    bool 			saveSetupAs(const MultiID&);
    bool 			saveSetup(const MultiID&);
    bool 			readSetup(const MultiID&);
    bool                        fireAddTreeObjectEvent();

    void                        saveUnsaveEMObject();
	
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    void			fireLAttribData()	{ loadAttribData(); }

protected:
    bool			activeVolumeIsDefault() const;
    void			expandActiveVolume(const CubeSampling&);
    void			activeVolumeChange(CallBacker*);
    void			loadAttribData();
    void			loadEMObjectCB(CallBacker*);
    void			mergeAttribSets(const Attrib::DescSet& newads,
						MPE::EMTracker&);
    bool			initSetupDlg(EM::EMObject*& emobj,
	    				     MPE::EMTracker*& tracker,
	    				     const EM::SectionID& sid,
					     bool freshdlg=false);

    const Attrib::DescSet*	attrset3d_;
    const Attrib::DescSet*	attrset2d_;
    bool			blockdataloading_;
    				/*!<Is checked when cb is issued from the
				    MPE::Engine about changed active volume */
    CubeSampling		postponedcs_;

				//Interaction variables
    const Attrib::SelSpec*	eventattrselspec_;
    int				activetrackerid_;
    int				temptrackerid_;
    int				cursceneid_;

    				//2D interaction
    BufferString		linename_;
    BufferString		attribname_;
    MultiID			linesetid_;
    Attrib::SelSpec		lineselspec_;
    
    void			aboutToAddRemoveSeed(CallBacker*);
    EM::ObjectID        	trackercurrentobject_;
    void			trackerWinClosedCB(CallBacker*);

    CubeSampling		trackerseedbox_;
    int				initialundoid_;
    bool			seedhasbeenpicked_;
    bool			setupbeingupdated_;
    bool			seedswithoutattribsel_;

    void			modeChangedCB(CallBacker*);
    void			eventorsimimlartyChangedCB(CallBacker*);
    void			propertyChangedCB(CallBacker*);
    void			retrackCB(CallBacker*);

    void			nrHorChangeCB(CallBacker*);

    void			adjustSeedBox();
    void			noTrackingRemoval();
    void			retrack( const EM::ObjectID& );

    void			cleanSetupDependents();

    MPE::uiSetupGroup*          setupgrp_;
};


#endif
