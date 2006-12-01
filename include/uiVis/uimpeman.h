#ifndef uimpeman_h
#define uimpeman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.h,v 1.41 2006-12-01 16:31:43 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "cubesampling.h"
#include  "trackplane.h"

namespace MPE { class EMTracker; };
namespace visSurvey { class MPEDisplay; class MPEClickCatcher; }

class uiColorBarDialog;
class uiComboBox;
class uiSpinBox;
class uiSlider;
class uiToolBar;
class uiVisPartServer;


/*! \brief Dialog for tracking properties
*/
class uiMPEMan : public CallBacker
{
public:		
				uiMPEMan(uiParent*,uiVisPartServer*);
				~uiMPEMan();

    uiToolBar*			getToolBar() const;

    void			deleteVisObjects();
    void			updateAttribNames();
    void			validateSeedConMode();
    void			introduceMPEDisplay();
    void			initFromDisplay();

    void			turnSeedPickingOn(bool);
    bool			isSeedPickingOn() const;
    void                        visObjectLockedCB();

protected:
    void			addButtons();
    visSurvey::MPEDisplay*	getDisplay(int sceneid,bool create=false);
    
    uiColorBarDialog*		colbardlg;

    uiToolBar*			toolbar;
    
    uiVisPartServer*		visserv;

    visSurvey::MPEClickCatcher*	clickcatcher;

    uiComboBox*			seedconmodefld;
    uiComboBox*			attribfld;
    uiSlider*			transfld;
    uiSpinBox*			nrstepsbox;

    void			boxDraggerStatusChangeCB(CallBacker*);
    void			showCubeCB(CallBacker*);

    void			attribSel(CallBacker*);

    void			transpChg(CallBacker*);

    void			undoPush(CallBacker*);
    void			redoPush(CallBacker*);
    void			updateButtonSensitivity(CallBacker* = 0);
    void			updateSelectedAttrib();
    void			trackBackward(CallBacker*);
    void			trackForward(CallBacker*);
    void			trackInVolume(CallBacker*);
    void			treeItemSelCB(CallBacker*);
    void			workAreaChgCB(CallBacker*);

    void			updateSeedPickState();
    void			trackerAddedRemovedCB(CallBacker*);
    void			addSeedCB(CallBacker*);
    void			seedConnectModeSel(CallBacker*);
    void			setColorbarCB(CallBacker*);
    void                        onColTabClosing(CallBacker*);
    void			movePlaneCB(CallBacker*);
    void			extendModeCB(CallBacker*);
    void			retrackModeCB(CallBacker*);
    void			eraseModeCB(CallBacker*);
    void			mouseEraseModeCB(CallBacker*);
    void			showTracker(bool yn);

    bool			isPickingInWizard() const;
    void			restoreActiveVol();
    
    MPE::EMTracker*		getSelectedTracker(); 

    void			setHistoryLevel(int);

    void			seedClick(CallBacker*);

    int				seedidx;
    int				clrtabidx;
    int				moveplaneidx, extendidx, retrackidx, eraseidx;
    int				showcubeidx, displayidx;
    int				mouseeraseridx;
    int				undoidx, redoidx;
    int				trackforwardidx, trackbackwardidx;
    int				trackinvolidx;
    bool			trackerwasonbeforemouseerase;

    bool			seedpickwason;

    MPE::TrackPlane		oldtrackplane;
    CubeSampling		oldactivevol;

    static const char*		sKeyNoAttrib() { return "No attribute"; }
};

#endif
