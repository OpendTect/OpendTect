#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"

#include "callback.h"
#include "emposid.h"
#include "menuhandler.h"
#include "uisettings.h"

namespace EM { class EdgeLineSet; class EdgeLineSegment; class Horizon3D;}
namespace visSurvey { class EMObjectDisplay; }

class uiColorTableGroup;
class uiMenuHandler;
class uiParent;
class uiVisPartServer;


mExpClass(uiVis) uiVisEMObject : public CallBacker
{ mODTextTranslationClass(uiVisEMObject)
public:
			uiVisEMObject(uiParent*,VisID displayid,
				      uiVisPartServer* );
			uiVisEMObject(uiParent*,const EM::ObjectID&,
				      SceneID sceneid,
				      uiVisPartServer*);
			~uiVisEMObject();
    bool		isOK() const;

    static const char*	getObjectType(VisID displayid);
    VisID		id() const { return displayid_; }
    EM::ObjectID	getObjectID() const;

    float		getShift() const;
    void		setDepthAsAttrib(int attrib);
    void		setOnlyAtSectionsDisplay(bool);
    bool		isOnlyAtSections() const;

    int			nrSections() const;
    EM::SectionID	getSectionID(int idx) const;
    EM::SectionID	getSectionID(const TypeSet<VisID>* pickedpath) const;

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

    VisID		displayid_;

    MenuItem		singlecolmnuitem_;
    MenuItem		seedsmenuitem_;
    MenuItem		showseedsmnuitem_;
    MenuItem		seedpropmnuitem_;
    MenuItem		lockseedsmnuitem_;
    MenuItem		ctrlpointsmenuitem_;
    MenuItem		showctrlpointsmnuitem_;
    MenuItem		ctrlpointspropmnuitem_;

    MenuItem		displaymnuitem_;
    MenuItem		showonlyatsectionsmnuitem_;
    MenuItem		showfullmnuitem_;
    MenuItem		showbothmnuitem_;
    MenuItem		showsurfacegridmnuitem_;
};



mExpClass(uiVis) uiHorizonSettings : public uiSettingsGroup
{ mODTextTranslationClass(uiHorizonSettings)
public:
			mDefaultFactoryInstantiation2Param(
				uiSettingsGroup,
				uiHorizonSettings,
				uiParent*,Settings&,
				"Horizons",
				toUiString(sFactoryKeyword()))

			uiHorizonSettings(uiParent*,Settings&);
    bool		acceptOK() override;
    HelpKey		helpKey() const override;

protected:
			~uiHorizonSettings();

    uiGenInput*		resolutionfld_;
    uiColorTableGroup*	coltabfld_;

    int			resolution_;
    BufferString	coltabnm_;
};
