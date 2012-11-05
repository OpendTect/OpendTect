#ifndef treecommands_h
#define treecommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"

namespace CmdDrive
{

mStartDeclCmdClassNoActNoEntry( uiCmdDriver,Tree, UiObjectCmd )
protected:

    bool		parTreeSelPre(const uiTreeView&,
	    			FileMultiString& treepath,
				ObjectSet<const uiTreeViewItem>& nodesfound,
				FileMultiString& curpath,bool multisel) const;

    void		unfoldTreeNodes(ObjectSet<const uiTreeViewItem>&) const;
    int			countTreeItems(const uiTreeView&,const uiTreeViewItem*,
				       bool countkids=true) const;

    const uiTreeViewItem* singleSelected(const uiTreeView& uilview) const;

mEndDeclCmdClass

mClass(uiCmdDriver) TreeActivator: public Activator
{
public:
			TreeActivator(const uiTreeView&,const uiTreeViewItem&,
				      const BufferStringSet& clicktags,
				      int column=0);
    void		actCB(CallBacker*);
protected:
    uiTreeView&		actlview_;
    uiTreeViewItem&	actitem_;
    int			actcolumn_;

    BufferStringSet	actclicktags_;
};


mStartDeclCmdClass( uiCmdDriver, TreeClick, TreeCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, TreeMenu, TreeCmd )		mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, TreeButton, TreeCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, TreeExpand, TreeCmd )		mEndDeclCmdClass

mStartDeclCmdClassNoActNoEntry( uiCmdDriver,TreeQuestion, TreeCmd )
    virtual bool	isUiObjChangeCommand() const	{ return false; }
    virtual bool	isVisualCommand() const		{ return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, NrTreeItems, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, NrTreeCols, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTreeItemOn, TreeQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTreeItemExpanded, TreeQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTreeButtonOn,TreeQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTreePath, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTreeCol, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTreeItem, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTreeCol, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTreeItem, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTreePath, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver,NrTreeMenuItems,TreeQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass(uiCmdDriver,IsTreeMenuItemOn,TreeQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver,GetTreeMenuItem,TreeQuestionCmd)mEndDeclCmdClass


mStartDeclComposerClassWithInit( uiCmdDriver, Tree, CmdComposer, uiTreeView )
public:
    virtual void		updateInternalState();

protected:
    void			reInit();	
    void			storeTreeState();
    void			addToTreeState(const uiTreeView&,
                                               const uiTreeViewItem* root=0);
    void			labelStoredStateOld();
    void			labelStoredStateNew();

    void			writeTreeClick();
    void			writeTreeButton();
    void			writeTreeExpand();
    void			writeTreeMenu(const CmdRecEvent& menuevent);

    int				stagenr_;
    const uiTreeViewItem*	clickeditem_;
    int				clickedcol_;
    bool			leftclicked_;
    bool			ctrlclicked_;
    bool			selchanged_;
    bool			expanded_;
    bool			treecmdsflushed_;

    ObjectSet<const uiTreeViewItem> checkeditems_;
    ObjectSet<const uiTreeViewItem> wascheckeditems_;
    ObjectSet<const uiTreeViewItem> ischeckeditems_;

mEndDeclComposerClass

}; // namespace CmdDrive

#endif

