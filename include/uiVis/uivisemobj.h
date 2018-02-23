#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2004
________________________________________________________________________


-*/

#include "uivismod.h"

#include "callback.h"
#include "emposid.h"
#include "menuhandler.h"
#include "uisettings.h"

namespace EM { class EdgeLineSet; class EdgeLineSegment; class Horizon3D; }
namespace visSurvey { class EMObjectDisplay; }

class uiColSeqSel;
class uiMenuHandler;
class uiParent;
class uiVisPartServer;


mExpClass(uiVis) uiVisEMObject : public CallBacker
{ mODTextTranslationClass(uiVisEMObject)
public:
			uiVisEMObject(uiParent*,int displayid,
				      uiVisPartServer* );
			uiVisEMObject(uiParent*,const DBKey&,int sceneid,
				      uiVisPartServer*);
			~uiVisEMObject();
    bool		isOK() const;

    static BufferString	getObjectType(int displayid);
    int			id() const { return displayid_; }
    DBKey		getObjectID() const;

    float		getShift() const;
    void		setDepthAsAttrib(int attrib);
    void		setOnlyAtSectionsDisplay(bool);
    bool		isOnlyAtSections() const;

    int			nrSections() const;
    EM::SectionID	getSectionID(int idx) const;
    EM::SectionID	getSectionID(const TypeSet<int>* pickedpath) const;

    void		checkTrackingStatus();
			/*!< Checks if there is a tracker for this object and
			     turns on singlecolor and full res if
			     a tracker if found. */

protected:

    void		setUpConnections();
    void		addToToolBarCB(CallBacker*);
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		keyEventCB(CallBacker*);
    void		checkHorizonSize(const EM::Horizon3D*);

    visSurvey::EMObjectDisplay*		getDisplay();
    const visSurvey::EMObjectDisplay*	getDisplay() const;


    uiParent*		uiparent_;
    uiVisPartServer*	visserv_;

    int			displayid_;

    MenuItem		singlecolmnuitem_;
    MenuItem		seedsmenuitem_;
    MenuItem		showseedsmnuitem_;
    MenuItem		seedpropmnuitem_;
    MenuItem		lockseedsmnuitem_;

    MenuItem		displaymnuitem_;
    MenuItem		showonlyatsectionsmnuitem_;
    MenuItem		showfullmnuitem_;
    MenuItem		showbothmnuitem_;
    MenuItem		showsurfacegridmnuitem_;
};
