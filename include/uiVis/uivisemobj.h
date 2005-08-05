#ifndef uivisemobj_h
#define uivisemobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2004
 RCS:		$Id: uivisemobj.h,v 1.9 2005-08-05 18:24:21 cvskris Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "emposid.h"
#include "menuhandler.h"

namespace EM { class EdgeLineSet; class EdgeLineSegment; };
class uiParent;
class uiMenuHandler;
class uiVisPartServer;
class MultiID;


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

    float		getShift() const;
    void		setDepthAsAttrib();
    uiMenuHandler&	getNodeMenu() { return nodemenu; }

    void		readAuxData();
    int			nrSections() const;
    EM::SectionID	getSectionID(int idx) const;
    EM::SectionID	getSectionID(const TypeSet<int>* pickedpath) const;

    static const char*	trackingmenutxt;

protected:
    void		setUpConnections();
    void		connectEditor();
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
    MenuItem		showseedsmnuitem;

    MenuItem		showonlyatsectionsmnuitem;
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
