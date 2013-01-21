#ifndef uivisemobj_h
#define uivisemobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2004
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uivismod.h"
#include "callback.h"
#include "emposid.h"
#include "menuhandler.h"

namespace EM { class EdgeLineSet; class EdgeLineSegment; }
namespace visSurvey { class EMObjectDisplay; }

class uiParent;
class uiMenuHandler;
class uiVisPartServer;
class MultiID;


mExpClass(uiVis) uiVisEMObject : public CallBacker
{
public:
    			uiVisEMObject(uiParent*,int displayid,
				      uiVisPartServer* );
    			uiVisEMObject(uiParent*,const EM::ObjectID&,int sceneid,
				      uiVisPartServer*);
			~uiVisEMObject();
    bool		isOK() const;

    static const char*	getObjectType(int displayid);
    int			id() const { return displayid_; }
    EM::ObjectID	getObjectID() const;

    float		getShift() const;
    void		setDepthAsAttrib(int attrib);
    void		setOnlyAtSectionsDisplay(bool);
    uiMenuHandler&	getNodeMenu() { return nodemenu_; }

    int			nrSections() const;
    EM::SectionID	getSectionID(int idx) const;
    EM::SectionID	getSectionID(const TypeSet<int>* pickedpath) const;

    void		checkTrackingStatus();
    			/*!< Checks if there is a tracker for this object and
			     turns on wireframe, singlecolor and full res if
			     a tracker if found. */

    static const char*	trackingmenutxt();

protected:
    void		setUpConnections();
    void		connectEditor();
    void		addToToolBarCB(CallBacker*);
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    void		interactionLineRightClick(CallBacker*);
    void		createInteractionLineMenuCB(CallBacker*) {}
    void		handleInteractionLineMenuCB(CallBacker*) {}

    void		edgeLineRightClick(CallBacker*);
    void		createEdgeLineMenuCB(CallBacker*);
    void		handleEdgeLineMenuCB(CallBacker*);

    void		nodeRightClick(CallBacker*);
    void		createNodeMenuCB(CallBacker*);
    void		handleNodeMenuCB(CallBacker*);

    visSurvey::EMObjectDisplay*		getDisplay();
    const visSurvey::EMObjectDisplay*	getDisplay() const;


    uiParent*		uiparent_;
    uiVisPartServer*	visserv_;

    uiMenuHandler&	nodemenu_;
    uiMenuHandler&	edgelinemenu_;
    uiMenuHandler&	interactionlinemenu_;
    
    int			displayid_;

    MenuItem		singlecolmnuitem_;
    MenuItem		wireframemnuitem_;
    MenuItem		editmnuitem_;
    MenuItem		removesectionmnuitem_;
    MenuItem		seedsmenuitem_;
    MenuItem		showseedsmnuitem_;
    MenuItem		seedpropmnuitem_;
    MenuItem		lockseedsmnuitem_;

    MenuItem		displaymnuitem_;
    MenuItem		showonlyatsectionsmnuitem_;
    MenuItem		showfullmnuitem_;
    MenuItem		showbothmnuitem_;
    MenuItem		changesectionnamemnuitem_;
    bool		showedtexture_;

    MenuItem		makepermnodemnuitem_;
    MenuItem		removecontrolnodemnuitem_;
};

#endif

