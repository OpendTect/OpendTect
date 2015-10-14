#ifndef uiodrandlinetreeitem_h
#define uiodrandlinetreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"

class IOObj;
class uiRandomLinePolyLineDlg;
namespace Geometry { class RandomLineSet; }

mDefineItem( RandomLineParent, TreeItem, TreeTop, mShowMenu \
    bool load(const IOObj&,int); \
    bool addStored(int); \
    void genRandLine(int); \
    void genFromContours(); \
    void genFromExisting(); \
    void genFromPolygon(); \
    void genFromTable(); \
    void genFromWell(); \
    void loadRandLineFromWell(CallBacker*); \
    void genFromPicks(); \
    void rdlPolyLineDlgCloseCB(CallBacker*); \
    void removeChild(uiTreeItem*);\
    uiRandomLinePolyLineDlg* rdlpolylinedlg_;
    mMenuOnAnyButton
)

namespace visSurvey { class RandomTrackDisplay; }


mExpClass(uiODMain) uiODRandomLineTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODRandomLineTreeItemFactory)
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODRandomLineParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODRandomLineTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODRandomLineTreeItem)
public:
    enum Type		{ Empty, Select, Default, RGBA };

			uiODRandomLineTreeItem(int displayid=-1,Type tp=Empty,
					       int rlid_=-1);

    bool		init();
    bool		displayDefaultData();
    void		setRandomLineID(int id);

protected:

    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		changeColTabCB(CallBacker*);
    void		remove2DViewerCB(CallBacker*);
    const char*		parentType() const
			{ return typeid(uiODRandomLineParentTreeItem).name(); }

    void		editNodes();

    MenuItem		editnodesmnuitem_;
    MenuItem		insertnodemnuitem_;
    MenuItem		usewellsmnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		saveas2dmnuitem_;
    MenuItem		create2dgridmnuitem_;
    Type		type_;
    int			rlid_;
};


#endif

