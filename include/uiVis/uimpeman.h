#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uiparent.h"
#include "trckeyzsampling.h"

namespace EM { class Object; class Horizon; class Horizon2D; class Horizon3D;}
namespace MPE { class EMTracker; }
namespace visSurvey
{
    class EMObjectDisplay;
    class Horizon2DDisplay;
    class HorizonDisplay;
    class MPEClickCatcher;
}

class uiVisPartServer;
class LockedDisplayTimer;


/*! \brief Dialog for tracking properties
*/
mExpClass(uiVis) uiMPEMan : public CallBacker
{ mODTextTranslationClass(uiMPEMan)
public:
				uiMPEMan(uiParent*,uiVisPartServer*);
				~uiMPEMan();

    void			deleteVisObjects();
    void			validateSeedConMode();
    void			initFromDisplay();

    void			turnSeedPickingOn(bool);
    bool			isSeedPickingOn() const;

    void			visObjectLockedCB(CallBacker*);

protected:

    uiParent*			parent_;
    uiVisPartServer*		visserv_;

    visSurvey::MPEClickCatcher*	clickcatcher_;
    int				clickablesceneid_;

    void			mouseEventCB(CallBacker*);
    void			keyEventCB(CallBacker*);
    void			mpeActionCalledCB(CallBacker*);
    void			mpeActionFinishedCB(CallBacker*);
    void			planeChangedCB(CallBacker*);
    int				popupMenu();
    void			handleAction(int);

    void			startTracking();
    void			startRetrack();
    void			stopTracking();
    void			changePolySelectionMode();
    void			showParentsPath();
    void			lockAll();
    void			clearSelection();
    void			deleteSelection();
    void			undo();
    void			redo();
    void			removeInPolygon();
    void			showSetupDlg();
    void			restrictCurrentHorizon();
    void			changeMode(int);

    void			trackFromSeedsOnly();
    void			trackFromSeedsAndEdges();
    void			treeItemSelCB(CallBacker*);
    void			workAreaChgCB(CallBacker*);
    void			survChgCB(CallBacker*);

    void			updateSeedPickState();
    void			mouseCursorCallCB(CallBacker*);
    void			trackerAddedRemovedCB(CallBacker*);

    bool			isPickingWhileSetupUp() const;

    MPE::EMTracker*		getSelectedTracker();
    EM::Horizon*		getSelectedHorizon();
    EM::Horizon2D*		getSelectedHorizon2D();
    EM::Horizon3D*		getSelectedHorizon3D();

    visSurvey::EMObjectDisplay* getSelectedDisplay();
    visSurvey::Horizon2DDisplay* getSelectedDisplay2D();
    visSurvey::HorizonDisplay*	getSelectedDisplay3D();

    int				cureventnr_;
    void			beginSeedClickEvent(EM::Object*);
    void			endSeedClickEvent(EM::Object*);
    void			setUndoLevel(const DBKey&,int);

    void			seedClick(CallBacker*);
    void			updateClickCatcher(bool create=true);
    void			cleanPatchDisplay();
    void			sowingFinishedCB(CallBacker*);
    void			sowingModeCB(CallBacker*);
    void			updatePatchDisplay();

    bool			seedpickwason_;
    bool			sowingmode_;
    TrcKeyZSampling		oldactivevol_;
    LockedDisplayTimer*		lockeddisplaytimer_;
};
