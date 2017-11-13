#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"
#include "uiodattribtreeitem.h"
#include "uiodprobeparenttreeitem.h"

class Probe;
class IOObj;
class uiRandomLinePolyLineDlg;
namespace Geometry { class RandomLineSet; }


mExpClass(uiODMain) uiODRandomLineParentTreeItem
				: public uiODSceneProbeParentTreeItem
{
    mODTextTranslationClass(uiODRandomLineParentTreeItem);
    mDefineItemMembers(RandomLineParent,SceneProbeParentTreeItem,SceneTreeTop);
    mMenuOnAnyButton;

    virtual void		addMenuItems();
    const char*			childObjTypeKey() const;
    virtual Probe*		createNewProbe() const;
    uiPresManagedTreeItem*	addChildItem(const Presentation::ObjInfo&);

    void			removeChild(uiTreeItem*);
    uiRandomLinePolyLineDlg*	rdlpolylinedlg_;

protected:

    int				rdltobeaddedid_;
    int				visrdltobeaddedid_;
    Probe*			rdlprobetobeadded_;

    bool			setProbeToBeAddedParams(int mnuid);
    bool			setSelRDLID();
    bool			setRDLIDFromContours();
    bool			setRDLIDFromExisting();
    bool			setRDLIDFromPolygon();
    bool			setRDLFromTable();
    bool			setRDLID(int opt);
    void			setRDLFromPicks();
    void			setRDLFromWell();

    void			loadRandLineFromWellCB(CallBacker*);
    void			rdlPolyLineDlgCloseCB(CallBacker*);
};


namespace visSurvey { class RandomTrackDisplay; }


mExpClass(uiODMain) uiODRandomLineTreeItemFactory
    : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(uiODRandomLineTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODRandomLineParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODRandomLineTreeItem : public uiODSceneProbeTreeItem
{ mODTextTranslationClass(uiODRandomLineTreeItem)
public:
    enum Type		{ Empty, Select, Default, RGBA };

			uiODRandomLineTreeItem(Probe&,int displayid=-1);

    bool		init();

protected:

    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		changeColTabCB(CallBacker*);
    void		remove2DViewerCB(CallBacker*);
    void		rdlGeomChanged(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODRandomLineParentTreeItem).name(); }

    void		editNodes();

    MenuItem		editnodesmnuitem_;
    MenuItem		insertnodemnuitem_;
    MenuItem		usewellsmnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		saveas2dmnuitem_;
    MenuItem		create2dgridmnuitem_;
};

mExpClass(uiODMain) uiODRandomLineAttribTreeItem : public uiODAttribTreeItem
{
public:
				uiODRandomLineAttribTreeItem(const char*);
    static void			initClass();
    static uiODDataTreeItem*	create(ProbeLayer&);
};
