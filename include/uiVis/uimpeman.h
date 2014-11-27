#ifndef uimpeman_h
#define uimpeman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uiparent.h"
#include "trackplane.h"

namespace EM { class EMObject; }
namespace MPE { class EMTracker; }
namespace visSurvey { class MPEDisplay; class MPEClickCatcher; }

class uiPropertiesDialog;
class uiComboBox;
class uiSpinBox;
class uiToolBar;
class uiVisPartServer;


/*! \brief Dialog for tracking properties
*/
mExpClass(uiVis) uiMPEMan : public CallBacker
{ mODTextTranslationClass(uiMPEMan)
public:
    friend class uiPropertiesDialog;
				uiMPEMan(uiParent*,uiVisPartServer*);
				~uiMPEMan();

    uiToolBar*			getToolBar() const;

    void			deleteVisObjects();
    void			validateSeedConMode();
    void			introduceMPEDisplay();
    void			updateSeedModeSel();
    void			initFromDisplay();
    void			trackInVolume();

    void			updateOldActiveVol();
    void			restoreActiveVolume();

    void			turnSeedPickingOn(bool);
    bool			isSeedPickingOn() const;
    void			turnQCPlaneOff();

    void                        visObjectLockedCB(CallBacker*);
    void			updateButtonSensitivity(CallBacker* = 0);

protected:
    void			addButtons();
    visSurvey::MPEDisplay*	getDisplay(int sceneid,bool create=false);

    uiPropertiesDialog*		propdlg_;
    bool			showtexture_;

    uiToolBar*			toolbar_;

    uiVisPartServer*		visserv_;

    visSurvey::MPEClickCatcher*	clickcatcher_;
    int				clickablesceneid_;

    uiComboBox*			seedconmodefld_;
    uiSpinBox*			nrstepsbox_;

    void			boxDraggerStatusChangeCB(CallBacker*);
    void			showCubeCB(CallBacker*);
    void			setCubePosCB(CallBacker*);

    void			attribSel(CallBacker*);

    void			undoPush(CallBacker*);
    void			redoPush(CallBacker*);
    void			savePush(CallBacker*);
    void			moveBackward(CallBacker*);
    void			moveForward(CallBacker*);
    void			trackFromSeedsOnly(CallBacker*);
    void			trackFromSeedsAndEdges(CallBacker*);
    void			trackInVolume(CallBacker*);
    void			selectionMode(CallBacker*);
    void			handleToolClick(CallBacker*);
    void 			removeInPolygon(CallBacker*);
    void			treeItemSelCB(CallBacker*);
    void			workAreaChgCB(CallBacker*);
    void			showSettingsCB(CallBacker*);
    void			displayAtSectionCB(CallBacker*);
    void			retrackAllCB(CallBacker*);

    void			updateSeedPickState();
    void			trackPlaneTrackCB(CallBacker*);
    void			trackerAddedRemovedCB(CallBacker*);
    void			addSeedCB(CallBacker*);
    void			seedConnectModeSel(CallBacker*);
    void			setColorbarCB(CallBacker*);
    void			movePlaneCB(CallBacker*);
    void			handleOrientationClick(CallBacker*);
    void			planeOrientationChangedCB(CallBacker*);
    void			mouseEraseModeCB(CallBacker*);
    void			showTracker(bool yn);
    void			changeTrackerOrientation(int orient);

    void                        onColTabClosing(CallBacker*);

    bool			isPickingWhileSetupUp() const;
    void			restoreActiveVol();

    MPE::EMTracker*		getSelectedTracker();

    void 			finishMPEDispIntro(CallBacker*);
    void			loadPostponedData();

    int				cureventnr_;
    void			beginSeedClickEvent(EM::EMObject*);
    void			endSeedClickEvent(EM::EMObject*);
    void			setUndoLevel(int);

    void			seedClick(CallBacker*);
    void			updateClickCatcher();

    int				seedidx_;
    int				clrtabidx_;
    int				moveplaneidx_;
    int				showcubeidx_, displayidx_;
    int				mouseeraseridx_;
    int				undoidx_, redoidx_;
    int				trackforwardidx_, trackbackwardidx_;
    int				trackwithseedonlyidx_;
    int				trackinvolidx_;
    int 			polyselectidx_;
    int				removeinpolygonidx_;
    int				displayatsectionidx_;
    int				retrackallidx_;
    int				saveidx_;
    bool			trackerwasonbeforemouseerase_;

    bool			seedpickwason_;
    bool			polyselstoppedseedpick_;
    bool			mpeintropending_;

    MPE::TrackPlane		oldtrackplane_;
    TrcKeyZSampling		oldactivevol_;

    static const char*		sKeyNoAttrib() { return "No attribute"; }
};

#endif

