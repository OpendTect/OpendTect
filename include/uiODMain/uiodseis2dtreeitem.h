#ifndef uiodseis2dtreeitem_h
#define uiodseis2dtreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"

#include "uimenuhandler.h"
#include "multiid.h"
#include "ranges.h"

class uiTaskRunner;

mDefineItem( Seis2DParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );

mClass(uiODMain) Seis2DTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODSeis2DParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mClass(uiODMain) uiOD2DLineSetTreeItem : public uiODTreeItem
{
public:
    			uiOD2DLineSetTreeItem(const MultiID&);
    void		selectAddLines();
    bool		showSubMenu();

    const MultiID&	lineSetID() const { return setid_; }
    int			selectionKey() const;

protected:
    			~uiOD2DLineSetTreeItem();
    bool		init();
    int			uiTreeViewItemType() const;

    void		checkCB(CallBacker*);
    virtual void	createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    void		createAttrMenu(MenuHandler*);
    void		selectNewAttribute(const char*);
    bool		isExpandable() const		{ return true; }
    const char*		parentType() const;

    Interval<float>	curzrg_;
    MultiID		setid_;
    RefMan<uiMenuHandler> menuhandler_;

    MenuItem		addlinesitm_;
    MenuItem		zrgitm_;
    MenuItem		addattritm_;
    MenuItem		removeattritm_;
    MenuItem		editcoltabitm_;
    MenuItem		editattritm_;
    MenuItem		showitm_;
    MenuItem		hideitm_;
    MenuItem		showlineitm_;
    MenuItem		hidelineitm_;
    MenuItem		showlblitm_;
    MenuItem		hidelblitm_;
    MenuItem		removeitm_;
    MenuItem		steeringitm_;
    MenuItem		storeditm_;
    MenuItem		coltabselitm_;
    MenuItem		showattritm_;
    MenuItem		hideattritm_;
    MenuItem		expanditm_;
    MenuItem		collapseitm_;
};


mClass(uiODMain) uiOD2DLineSetSubItem : public uiODDisplayTreeItem
{
public:
			uiOD2DLineSetSubItem(const char* nm,int displayid=-1);

    bool		addStoredData(const char*,int component,uiTaskRunner&);
    void		addAttrib(const Attrib::SelSpec&,uiTaskRunner&);
    void		showLineName(bool);
    void		setZRange(const Interval<float>);
    void		removeAttrib(const char*);

protected:
			~uiOD2DLineSetSubItem();
    bool		init();
    const char*		parentType() const;
    BufferString	createDisplayName() const;

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		getNewData(CallBacker*);

private:
    MenuItem		linenmitm_;
    MenuItem		positionitm_;
};


mClass(uiODMain) uiOD2DLineSetAttribItem : public uiODAttribTreeItem
{
public:
				uiOD2DLineSetAttribItem(const char* parenttype);
    bool			displayStoredData(const char*,int component,
						  uiTaskRunner&);
    void			setAttrib(const Attrib::SelSpec&,
					  uiTaskRunner&);
    void			clearAttrib();

protected:
    void			createMenu(MenuHandler*,bool istb);
    void			handleMenuCB(CallBacker*);

    MenuItem			storeditm_;
    MenuItem			steeringitm_;
    MenuItem			zattritm_;
    MenuItem			attrnoneitm_;
};


#endif

