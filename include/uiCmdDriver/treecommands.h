#ifndef treecommands_h
#define treecommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id: treecommands.h,v 1.1 2012-09-17 12:38:33 cvsjaap Exp $
 ________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"

namespace CmdDrive
{

class CmdDriver;


mStartDeclCmdClassNoAct( Tree, UiObjectCmd )
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

mClass(CmdDriver) TreeActivator: public Activator
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


mStartDeclCmdClass( TreeClick, TreeCmd )		mEndDeclCmdClass
mStartDeclCmdClass( TreeMenu, TreeCmd )			mEndDeclCmdClass

mStartDeclCmdClass( TreeButton, TreeCmd )		mEndDeclCmdClass
mStartDeclCmdClass( TreeExpand, TreeCmd )		mEndDeclCmdClass

mStartDeclCmdClassNoAct( TreeQuestion, TreeCmd )
    virtual bool	isUiObjChangeCommand() const	{ return false; }
    virtual bool	isVisualCommand() const		{ return false; }
mEndDeclCmdClass

mStartDeclCmdClass( NrTreeItems, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( NrTreeCols, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( IsTreeItemOn, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( IsTreeItemExpanded, TreeQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( IsTreeButtonOn, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( CurTreePath, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( CurTreeCol, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( CurTreeItem, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( GetTreeCol, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( GetTreeItem, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( GetTreePath, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( NrTreeMenuItems, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( IsTreeMenuItemOn, TreeQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( GetTreeMenuItem, TreeQuestionCmd )		mEndDeclCmdClass


mStartDeclComposerClassWithInit( Tree, CmdComposer )
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

