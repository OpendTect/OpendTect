#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uiparent.h"
#include "trckeyzsampling.h"
#include "trckeyvalue.h"
#include "emposid.h"

namespace EM { class EMObject; class Horizon; class Horizon2D; class Horizon3D;}
namespace MPE { class EMTracker; }
namespace visSurvey
{
    class EMObjectDisplay;
    class Horizon2DDisplay;
    class HorizonDisplay;
    class MPEClickCatcher;
}

class uiPropertiesDialog;
class uiVisPartServer;
class LockedDisplayTimer;


/*! \brief Dialog for tracking properties
*/
mExpClass(uiVis) uiMPEMan : public CallBacker
{ mODTextTranslationClass(uiMPEMan)
public:
    friend class uiPropertiesDialog;
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
    SceneID			clickablesceneid_;

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
    void			undo();
    void			redo();
    void			changePolySelectionMode();
    void			clearSelection();
    void			deleteSelection();
    void			removeInPolygon();
    void			showParentsPath();
    void			showSetupDlg();
    void			restrictCurrentHorizon();
    void			changeMode(int);

    void			trackFromSeedsOnly();
    void			trackFromSeedsAndEdges();
    void			treeItemSelCB(CallBacker*);
    void			workAreaChgCB(CallBacker*);

    void			updateSeedPickState();
    void			mouseCursorCallCB(CallBacker*);
    void			trackerAddedRemovedCB(CallBacker*);

    bool			isPickingWhileSetupUp() const;

    MPE::EMTracker*		getSelectedTracker();
    visSurvey::Horizon2DDisplay* getSelected2DDisplay();
    visSurvey::HorizonDisplay*	getSelectedDisplay();
    visSurvey::EMObjectDisplay* getSelectedEMDisplay();

    EM::Horizon*		getSelectedHorizon();
    EM::Horizon2D*		getSelectedHorizon2D();
    EM::Horizon3D*		getSelectedHorizon3D();

    int				cureventnr_;
    void			beginSeedClickEvent(EM::EMObject*);
    void			endSeedClickEvent(EM::EMObject*);
    mDeprecated("Use other setUndoLevel")
    void			setUndoLevel(int);

    void			setUndoLevel(const EM::ObjectID&,
					     int preveventnr);

    void			seedClick(CallBacker*);
    void			updateClickCatcher(bool create=true);
    void			cleanPatchDisplay();
    void			sowingFinishedCB(CallBacker*);
    void			sowingModeCB(CallBacker*);
    void			updatePatchDisplay();
    void			lockAll();

    bool			seedpickwason_;
    bool			sowingmode_;
    TrcKeyZSampling		oldactivevol_;
    LockedDisplayTimer*		lockeddisplaytimer_;
};
