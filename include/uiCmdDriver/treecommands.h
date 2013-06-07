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

#include "command.h"
#include "cmdcomposer.h"

namespace CmdDrive
{

mStartDeclCmdClassNoActNoEntry( Tree, UiObjectCmd )
protected:

    bool		parTreeSelPre(const uiListView&,
	    			FileMultiString& treepath,
				ObjectSet<const uiListViewItem>& nodesfound,
				FileMultiString& curpath,bool multisel) const;

    void		unfoldTreeNodes(ObjectSet<const uiListViewItem>&) const;
    int			countTreeItems(const uiListView&,const uiListViewItem*,
				       bool countkids=true) const;

    const uiListViewItem* singleSelected(const uiListView& uilview) const;

mEndDeclCmdClass

mClass TreeActivator: public Activator
{
public:
			TreeActivator(const uiListView&,const uiListViewItem&,
				      const BufferStringSet& clicktags,
				      int column=0);
    void		actCB(CallBacker*);
protected:
    uiListView&		actlview_;
    uiListViewItem&	actitem_;
    int			actcolumn_;

    BufferStringSet	actclicktags_;
};


mStartDeclCmdClass( TreeClick, TreeCmd )		mEndDeclCmdClass
mStartDeclCmdClass( TreeMenu, TreeCmd )			mEndDeclCmdClass

mStartDeclCmdClass( TreeButton, TreeCmd )		mEndDeclCmdClass
mStartDeclCmdClass( TreeExpand, TreeCmd )		mEndDeclCmdClass

mStartDeclCmdClassNoActNoEntry( TreeQuestion, TreeCmd )
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


mStartDeclComposerClassWithInit( Tree, CmdComposer, uiListView )
public:
    virtual void		updateInternalState();

protected:
    void			reInit();	
    void			storeTreeState();
    void			addToTreeState(const uiListView&,
                                               const uiListViewItem* root=0);
    void			labelStoredStateOld();
    void			labelStoredStateNew();

    void			writeTreeClick();
    void			writeTreeButton();
    void			writeTreeExpand();
    void			writeTreeMenu(const CmdRecEvent& menuevent);

    int				stagenr_;
    const uiListViewItem*	clickeditem_;
    int				clickedcol_;
    bool			leftclicked_;
    bool			ctrlclicked_;
    bool			selchanged_;
    bool			expanded_;
    bool			treecmdsflushed_;

    ObjectSet<const uiListViewItem> checkeditems_;
    ObjectSet<const uiListViewItem> wascheckeditems_;
    ObjectSet<const uiListViewItem> ischeckeditems_;

mEndDeclComposerClass

}; // namespace CmdDrive

#endif

