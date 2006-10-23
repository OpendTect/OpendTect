#ifndef uivisemobj_h
#define uivisemobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2004
 RCS:		$Id: uivisemobj.h,v 1.22 2006-10-23 09:13:46 cvsjaap Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "emposid.h"
#include "menuhandler.h"

namespace EM { class EdgeLineSet; class EdgeLineSegment; }
namespace visSurvey { class EMObjectDisplay; }

class uiParent;
class uiMenuHandler;
class uiVisPartServer;
class MultiID;


class uiVisEMObject : public CallBacker
{
public:
    			uiVisEMObject(uiParent*,int displayid,
				      uiVisPartServer* );
    			uiVisEMObject(uiParent*,const EM::ObjectID&,int sceneid,
				      uiVisPartServer*);
			~uiVisEMObject();
    bool		isOK() const;

    static const char*	getObjectType(int displayid);
    int			id() const { return displayid; }
    EM::ObjectID	getObjectID() const;

    float		getShift() const;
    void		setDepthAsAttrib(int attrib);
    void		setOnlyAtSectionsDisplay(bool);
    uiMenuHandler&	getNodeMenu() { return nodemenu; }

    int			nrSections() const;
    EM::SectionID	getSectionID(int idx) const;
    EM::SectionID	getSectionID(const TypeSet<int>* pickedpath) const;

    void		checkTrackingStatus();
    			/*!< Checks if there is a tracker for this object and
			     turns on wireframe, singlecolor and full res if
			     a tracker if found. */

    static const char*	trackingmenutxt;

protected:
    void		setUpConnections();
    void		connectEditor();
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



    uiParent*		uiparent;
    uiVisPartServer*	visserv;

    uiMenuHandler&	nodemenu;
    uiMenuHandler&	edgelinemenu;
    uiMenuHandler&	interactionlinemenu;
    
    int			displayid;

    MenuItem		singlecolmnuitem;
    MenuItem		wireframemnuitem;
    MenuItem		trackmenuitem;
    MenuItem		editmnuitem;
    MenuItem		shiftmnuitem;
    MenuItem		removesectionmnuitem;
    MenuItem		seedsmenuitem;
    MenuItem		showseedsmnuitem;
    MenuItem		seedpropmnuitem;
    MenuItem		lockseedsmnuitem;

    MenuItem		showonlyatsectionsmnuitem;
    MenuItem		changesectionnamemnuitem;
    bool		showedtexture;
//
    MenuItem		makepermnodemnuitem;
    MenuItem		removecontrolnodemnuitem;
    //int			removenodenodemnuitem;
    //int			tooglesnappingnodemnuitem;
////
    //int			cutsmalllinemnuitem;
    //int			cutlargelinemnuitem;
    //int			splitlinemnuitem;
    //int			mkstoplinemnuitem;
//
    //int			removetermedgelinemnuitem;
    //int			removeconnedgelinemnuitem;
    //int			joinedgelinemnuitem;
};

#endif
