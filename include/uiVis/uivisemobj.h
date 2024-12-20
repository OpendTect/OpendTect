#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"

#include "menuhandler.h"
#include "uisettings.h"
#include "visemobjdisplay.h"
#include "vishorizondisplay.h"
#include "vishorizon2ddisplay.h"

namespace EM
{
    class EdgeLineSet;
    class EdgeLineSegment;
    class Horizon3D;
}

class uiColorTableGroup;
class uiMenuHandler;
class uiParent;
class uiVisPartServer;


mExpClass(uiVis) uiVisEMObject : public CallBacker
{ mODTextTranslationClass(uiVisEMObject)
public:
			uiVisEMObject(uiParent*,const EM::ObjectID&,
				      const SceneID&,uiVisPartServer*);
			uiVisEMObject(uiParent*,const VisID&,
				      uiVisPartServer*);
			~uiVisEMObject();

    bool		isOK() const;

    VisID		id() const { return displayid_; }
    EM::ObjectID	getObjectID() const;

    ConstRefMan<visSurvey::HorizonDisplay> getHorizon3DDisplay() const;
    ConstRefMan<visSurvey::Horizon2DDisplay> getHorizon2DDisplay() const;
    RefMan<visSurvey::HorizonDisplay> getHorizon3DDisplay();
    RefMan<visSurvey::Horizon2DDisplay> getHorizon2DDisplay();

    float		getShift() const;
    void		setDepthAsAttrib(int attrib);
    void		setOnlyAtSectionsDisplay(bool);
    bool		isOnlyAtSections() const;

    int			nrSections() const;
    EM::SectionID	getSectionID(int idx) const;
    EM::SectionID	getSectionID(const TypeSet<VisID>* pickedpath) const;

    bool		activateTracker();
    void		checkTrackingStatus();
			/*!< Checks if there is a tracker for this object and
			     turns on singlecolor and full res if
			     a tracker if found. */

private:

    void		setUpConnections();
    void		addToToolBarCB(CallBacker*);
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		keyEventCB(CallBacker*);
    void		checkHorizonSize(const EM::Horizon3D*);

    ConstRefMan<visSurvey::EMObjectDisplay> getDisplay() const;
    RefMan<visSurvey::EMObjectDisplay> getDisplay();

    uiParent*		uiparent_;
    uiVisPartServer*	visserv_;

    VisID		displayid_;
    SceneID		sceneid_;
    WeakPtr<visSurvey::EMObjectDisplay> emobjdisplay_;

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
