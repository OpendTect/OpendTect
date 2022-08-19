#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

mExpClass(uiCmdDriver) TreeActivator: public Activator
{
public:
			TreeActivator(const uiTreeView&,const uiTreeViewItem&,
				      const BufferStringSet& clicktags,
				      int column=0);
    void		actCB(CallBacker*) override;
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
    bool	isUiObjChangeCommand() const override	{ return false; }
    bool	isVisualCommand() const override	{ return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, NrTreeItems, TreeQuestionCmd ) mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, NrTreeCols, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTreeItemOn, TreeQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTreeItemExpanded, TreeQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTreeButtonOn,TreeQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTreePath, TreeQuestionCmd ) mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTreeCol, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTreeItem, TreeQuestionCmd ) mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTreeCol, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTreeItem, TreeQuestionCmd ) mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTreePath, TreeQuestionCmd ) mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver,NrTreeMenuItems,TreeQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass(uiCmdDriver,IsTreeMenuItemOn,TreeQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver,GetTreeMenuItem,TreeQuestionCmd)mEndDeclCmdClass


mStartDeclComposerClassWithInit( uiCmdDriver, Tree, CmdComposer, uiTreeView )
public:
    void			updateInternalState() override;

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

} // namespace CmdDrive
