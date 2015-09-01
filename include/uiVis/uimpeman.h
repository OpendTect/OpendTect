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
#include "trckeyzsampling.h"

namespace EM { class EMObject; class Horizon3D; }
namespace MPE { class EMTracker; }
namespace visSurvey { class HorizonDisplay; class MPEClickCatcher; }

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
    void			initFromDisplay();

    void			turnSeedPickingOn(bool);
    bool			isSeedPickingOn() const;

    void			visObjectLockedCB(CallBacker*);
    void			updateButtonSensitivity(CallBacker* = 0);

protected:
    void			addButtons();

    uiToolBar*			toolbar_;

    uiVisPartServer*		visserv_;

    visSurvey::MPEClickCatcher*	clickcatcher_;
    int				clickablesceneid_;

    void			mouseEventCB(CallBacker*);
    void			keyEventCB(CallBacker*);
    int				popupMenu();
    void			handleAction(int);

    void			undo();
    void			redo();
    void			startPolySelection();
    void			clearSelection();
    void			deleteSelection();
    void			removeInPolygon();
    void			showParentsPath();
    void			showSetupDlg();

    void			trackFromSeedsOnly();
    void			trackFromSeedsAndEdges();
    void			trackInVolume();
    void			treeItemSelCB(CallBacker*);
    void			workAreaChgCB(CallBacker*);
    void			retrackAll();

    void			updateSeedPickState();
    void			trackerAddedRemovedCB(CallBacker*);
    void			addSeedCB(CallBacker*);

    bool			isPickingWhileSetupUp() const;

    MPE::EMTracker*		getSelectedTracker();
    visSurvey::HorizonDisplay*	getSelectedDisplay();
    EM::Horizon3D*		getSelectedHorizon3D();

    void			finishMPEDispIntro(CallBacker*);

    int				cureventnr_;
    void			beginSeedClickEvent(EM::EMObject*);
    void			endSeedClickEvent(EM::EMObject*);
    void			setUndoLevel(int);

    void			seedClick(CallBacker*);
    void			updateClickCatcher();

    int				seedidx_;

    bool			seedpickwason_;
    bool			polyselstoppedseedpick_;
    bool			mpeintropending_;

    TrcKeyZSampling		oldactivevol_;
};

#endif

