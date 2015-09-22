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
class uiVisPartServer;


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
    int				clickablesceneid_;

    void			mouseEventCB(CallBacker*);
    void			keyEventCB(CallBacker*);
    void			mpeActionCB(CallBacker*);
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

    void			trackFromSeedsOnly();
    void			trackFromSeedsAndEdges();
    void			treeItemSelCB(CallBacker*);
    void			workAreaChgCB(CallBacker*);

    void			updateSeedPickState();
    void			mouseCursorCallCB(CallBacker*);
    void			trackerAddedRemovedCB(CallBacker*);

    bool			isPickingWhileSetupUp() const;

    MPE::EMTracker*		getSelectedTracker();
    visSurvey::HorizonDisplay*	getSelectedDisplay();
    EM::Horizon3D*		getSelectedHorizon3D();

    int				cureventnr_;
    void			beginSeedClickEvent(EM::EMObject*);
    void			endSeedClickEvent(EM::EMObject*);
    void			setUndoLevel(int);

    void			seedClick(CallBacker*);
    void			updateClickCatcher();

    bool			seedpickwason_;
    TrcKeyZSampling		oldactivevol_;
};

#endif

