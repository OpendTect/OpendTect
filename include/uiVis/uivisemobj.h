#ifndef uivisemobj_h
#define uivisemobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2004
 RCS:		$Id: uivisemobj.h,v 1.1 2005-01-17 08:36:09 kristofer Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "emposid.h"

namespace EM { class EdgeLineSet; class EdgeLineSegment; };
class uiParent;
class uiVisMenu;
class uiVisPartServer;
class MultiID;


#define mGetTrackingBoolean(param) \
Settings::common().getYN(IOPar::compKey("dTect","Enable Tracking"),param)


class uiVisEMObject : public CallBackClass
{
public:
    			uiVisEMObject(uiParent*,int displayid,
				      uiVisPartServer* );
    			uiVisEMObject(uiParent*,const MultiID&,int sceneid,
				      uiVisPartServer*);
			~uiVisEMObject();
    bool		isOK() const;
    void		prepareForShutdown();

    static const char*	getObjectType(int displayid);
    static bool		canHandle(int displayid);
    int			id() const { return displayid; }

    void		setDepthAsAttrib();
    uiVisMenu&		getNodeMenu() { return nodemenu; }

    //void		updateTexture();
    //int			nrSections() const;
    //EM::SectionID	getSectionID(int idx) const;
    //EM::SectionID	getSectionID(const TypeSet<int>* pickedpath) const;

    //static const char*	trackingmenutxt;

protected:
    void		setUpConnections();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    void		interactionLineRightClick(CallBacker*);
    void		createInteractionLineMenuCB(CallBacker*);
    void		handleInteractionLineMenuCB(CallBacker*);

    void		edgeLineRightClick(CallBacker*);
    void		createEdgeLineMenuCB(CallBacker*);
    void		handleEdgeLineMenuCB(CallBacker*);

    void		nodeRightClick(CallBacker*);
    void		createNodeMenuCB(CallBacker*);
    void		handleNodeMenuCB(CallBacker*);

    uiParent*		uiparent;
    uiVisPartServer*	visserv;

    uiVisMenu&		nodemenu;
    uiVisMenu&		edgelinemenu;
    uiVisMenu&		interactionlinemenu;
    
    int			displayid;

    int			singlecolmnuid;
    int			wireframemnuid;
    int			editmnuid;
    int			shiftmnuid;
    int			removesectionmnuid;
//
    //int			makepermnodemnusel;
    //int			removecontrolnodemnusel;
    //int			removenodenodemnusel;
    //int			tooglesnappingnodemnusel;
////
    //int			cutsmalllinemnusel;
    //int			cutlargelinemnusel;
    //int			splitlinemnusel;
    //int			mkstoplinemnusel;
//
    //int			removetermedgelinemnusel;
    //int			removeconnedgelinemnusel;
    //int			joinedgelinemnusel;
};

#endif
