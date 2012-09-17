#ifndef uimpeman_h
#define uimpeman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.h,v 1.64 2012/06/27 15:23:39 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "trackplane.h"

namespace EM { class EMObject; };
namespace MPE { class EMTracker; };
namespace visSurvey { class MPEDisplay; class MPEClickCatcher; }

class uiPropertiesDialog;
class uiComboBox;
class uiSpinBox;
class uiToolBar;
class uiVisPartServer;


/*! \brief Dialog for tracking properties
*/
mClass uiMPEMan : public CallBacker
{
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

protected:
    void			addButtons();
    visSurvey::MPEDisplay*	getDisplay(int sceneid,bool create=false);

    uiPropertiesDialog*		propdlg_;
    bool			showtexture_;

    uiToolBar*			toolbar;
    
    uiVisPartServer*		visserv;

    visSurvey::MPEClickCatcher*	clickcatcher;
    int				clickablesceneid;

    uiComboBox*			seedconmodefld;
    uiSpinBox*			nrstepsbox;

    void			boxDraggerStatusChangeCB(CallBacker*);
    void			showCubeCB(CallBacker*);

    void			attribSel(CallBacker*);

    void			undoPush(CallBacker*);
    void			redoPush(CallBacker*);
    void			savePush(CallBacker*);
public:
    void			updateButtonSensitivity(CallBacker* = 0);
protected:
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

    int				seedidx;
    int				clrtabidx;
    int				moveplaneidx;
    int				showcubeidx, displayidx;
    int				mouseeraseridx;
    int				undoidx, redoidx;
    int				trackforwardidx, trackbackwardidx;
    int				trackwithseedonlyidx;
    int				trackinvolidx;
    int 			polyselectidx;
    int				removeinpolygon;
    int				displayatsectionidx;
    int				retrackallinx;
    bool			trackerwasonbeforemouseerase;

    bool			seedpickwason;
    bool			polyselstoppedseedpick;
    bool			mpeintropending;

    MPE::TrackPlane		oldtrackplane;
    CubeSampling		oldactivevol;

    static const char*		sKeyNoAttrib() { return "No attribute"; }
};

#endif
