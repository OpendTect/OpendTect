/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          February 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: treecommands.cc,v 1.1 2012-09-17 12:37:42 cvsjaap Exp $";

#include "treecommands.h"

#include "cmddriverbasics.h"
#include "cmdrecorder.h"

#include "uimenu.h"
#include "uitreeview.h"

namespace CmdDrive
{


void TreeCmd::unfoldTreeNodes( ObjectSet<const uiTreeViewItem>& nodelist ) const
{
    for ( int idx=0; idx<nodelist.size(); idx++ )
    {
	for ( int idy=0; idy<nodelist[idx]->nrChildren(); idy++ )
	{
	    const uiTreeViewItem* curkid = nodelist[idx]->getChild(idy);
	    if ( !curkid->isVisible() )
		continue;
	    if ( greyOutsSkipped() && !curkid->isEnabled() )
		continue;

	    nodelist.insertAfter( curkid, idx+idy );
	}
    }
}


bool TreeCmd::parTreeSelPre( const uiTreeView& uilview,
			     FileMultiString& treepath,
			     ObjectSet<const uiTreeViewItem>& nodesfound,
			     FileMultiString& curpath, bool multisel ) const
{
    if ( !uilview.nrItems() && treepath.rep()=="\a<" )
	return true;

    const bool wholesubtree =
			StringProcessor(treepath).removeTokenAppendix( '<' );

    if ( wholesubtree && !multisel )
    {
	mParseErrStrm << "Command cannot select whole subtree" << std::endl;
	return false;
    }

    const uiTreeViewItem* curitm = 0;
    bool disabledpath = false;
    bool collapsedpath = false;
    for ( int level=0; level<mSepStrSize(treepath); level++ )
    {
	BufferString itemstr( treepath[level] );
	mParDisambiguator( "tree node name", itemstr, itmselnr );
	nodesfound.erase();
	int nrgrey = 0;
	const int nrkids = curitm ? curitm->nrChildren() : uilview.nrItems();
	for ( int idx=0; idx<nrkids; idx++ )
	{
	    uiTreeViewItem* curkid = curitm ? curitm->getChild(idx)
					    : uilview.getItem(idx);

	    if  ( curkid && curkid->isVisible() &&
		  mSearchKey(itemstr).isMatching(curkid->text()) )
	    {
		if ( !greyOutsSkipped() || curkid->isEnabled() )
		    nodesfound += curkid;

		if ( !curkid->isEnabled() )
		    nrgrey++;
	    }
	}
	FileMultiString tmpstr( curpath ); tmpstr += itemstr;
	const bool ambicheck = !multisel || level<mSepStrSize(treepath)-1;
	mParStrPre( "tree node", nodesfound, nrgrey, tmpstr.unescapedStr(),
		    itmselnr, "path", ambicheck );

	if ( !collapsedpath && curitm && !curitm->isOpen() )
	{
	    collapsedpath = true;
	    mWinWarnStrm << "Tree node \"" << curpath.unescapedStr()
			 << "\" not expanded" << std::endl;
	}
	curitm = nodesfound[0];
	BufferString curitmtxt = curitm->text();
	WildcardManager& wcm = const_cast<TreeCmd*>(this)->wildcardMan();
	wcm.check( mSearchKey(itemstr), curitmtxt );
	mDressNameString( curitmtxt, sTreePath );
	curpath += curitmtxt;
	disabledpath = disabledpath || !curitm->isEnabled();
    }

    if ( wholesubtree )
	unfoldTreeNodes( nodesfound );

    mDisabilityCheck( "tree node", nodesfound.size(), disabledpath );
    return true;
} 


int TreeCmd::countTreeItems( const uiTreeView& uilview,
                        const uiTreeViewItem* treenode, bool countkids ) const
{
    if ( !treenode && !countkids )
	return 0;

    const uiTreeViewItem* parent = treenode;
    if ( treenode && !countkids )
	 parent = treenode->parent();

    int count = 0;
    const int nrkids = parent ? parent->nrChildren() : uilview.nrItems();
    for ( int idx=0; idx<nrkids; idx++ )
    {
        uiTreeViewItem* curkid = parent ? parent->getChild(idx) :
					  uilview.getItem(idx);
	if ( !curkid->isVisible() )
	    continue;
	if ( greyOutsSkipped() && !curkid->isEnabled() )
	    continue;

	count++;
	if ( curkid == treenode )
	    break;
    }
    return count;
}


const uiTreeViewItem* TreeCmd::singleSelected( const uiTreeView& uilview ) const
{
    ObjectSet<const uiTreeViewItem> nodesfound;
    FileMultiString treepath = "\a<";
    FileMultiString dummy;
    parTreeSelPre( uilview, treepath, nodesfound, dummy, true );

    for ( int idx=nodesfound.size()-1; idx>=0; idx-- )
    {
	if ( !nodesfound[idx]->isSelected() )
	    nodesfound.remove( idx );
    }

    return nodesfound.size()==1 ? nodesfound[0] : 0;
}


#define mParTreeSelPre( uilview, treepath, nodesfound, curpath, multisel ) \
\
    ObjectSet<const uiTreeViewItem> nodesfound; \
    FileMultiString curpath; \
    if ( !parTreeSelPre(*uilview, treepath, nodesfound, curpath, multisel) ) \
	return false;


#define mParColSelPre( objnm,uilview,nodes,itemstr,itemnr,columns,ambicheck ) \
\
    TypeSet<int> columns; \
\
    if ( mIsUdf(itemnr) ) \
	columns += 0; \
    else \
    { \
	BufferStringSet itemtexts; \
	for ( int row=-1; row<nodes.size(); row++ ) \
	{ \
	    itemtexts.erase(); \
	    for ( int col=0; col<uilview->nrColumns(); col++ ) \
	    { \
		const BufferString itemtxt = row<0 ? uilview->columnText(col) \
						   : nodes[row]->text(col); \
		itemtexts.add( itemtxt ); \
		if ( mSearchKey(itemstr).isMatching(itemtxt) ) \
		    columns += col; \
	    } \
	    if ( !columns.isEmpty() ) \
		break; \
	} \
	mParStrPre( objnm, columns, 0, itemstr, itemnr, "string", ambicheck ); \
	wildcardMan().check( mSearchKey(itemstr), itemtexts.get(columns[0]) ); \
    } \


#define mParTreeTagBase( parstr, parnext, pathcol ) \
\
    bool pathcol = false; \
    BufferString tagstr; \
    const char* mUnusedVar parnext = getNextWord( parstr, tagstr.buf() ); \
\
    if ( mMatchCI(tagstr,"PathCol") ) \
	pathcol = true; \
    else if ( !isalpha(tagstr[0]) ) \
	parnext = parstr; 

#define mParTreeTag( parstr, parnext, pathcol ) \
\
    mParTreeTagBase( parstr, parnext, pathcol ) \
    else \
    { \
	mParseErrStrm << "Selection option not in [PathCol]" << std::endl; \
	return false; \
    }


static bool hasTreeKeyStr( const char* parstr, int maxdquoted,
			   bool colbydefault=false )
{
    const char* parnext;
    const int nrdquoted = StringProcessor(parstr).consecutiveDQuoted(&parnext);
    if ( nrdquoted == maxdquoted )
	return true;

    mParTreeTagBase( parnext, dummy, pathcol );
    if ( nrdquoted==1 && pathcol )
	return true;

    const char* parnexxt;
    strtol( parnext, const_cast<char**>(&parnexxt), 0 );
    return colbydefault && nrdquoted==1 && parnext!=parnexxt;
}


#define mPathColCheck( pathcol, itemnr, parstr, parnext ) \
\
    if ( !pathcol ) \
    { \
	itemnr = mUdf(int); \
	parnext = parstr; \
    }


bool TreeClickCmd::act( const char* parstr )
{
    const char* extraparstr = hasTreeKeyStr(parstr,2) ? parstr : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnext = extraparstr==parstr ? extraparnext : parstr;
    mParTreeTag( parnext, parnexxt, pathcol );
    mParPathStrInit( "tree", parnexxt, parnexxxt, treepath );
    mParItemSelInit("column", parnexxxt, parnexxxxt, itemstr, itemnr, !pathcol);
    mPathColCheck( pathcol, itemnr, parnexxxt, parnexxxxt );
    mParMouse( parnexxxxt, partail, clicktags, "Left" );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
    mParColSelPre( "column",uilview,nodesfound,itemstr,itemnr,columns,true );

    mActivate( Tree, Activator(*uilview,*nodesfound[0],clicktags,columns[0]) );
    return true;
}

TreeActivator::TreeActivator( const uiTreeView& uilview,
			      const uiTreeViewItem& treeitm,
			      const BufferStringSet& clicktags, int column )
    : actlview_( const_cast<uiTreeView&>(uilview) )
    , actitem_( const_cast<uiTreeViewItem&>(treeitm) )
    , actclicktags_( clicktags )
    , actcolumn_( column )
{}


#define mTreeViewTrigger( notifier ) \
{ \
    actlview_.setNotifiedItem( actitem_.qItem() ); \
    actlview_.setNotifiedColumn( actcolumn_ ); \
    actlview_.notifier.trigger(); \
}

#define mTreeViewTrigger2( notifier1, notifier2 ) \
{ \
    mTreeViewTrigger( notifier1 ); \
    actlview_.notifier2.trigger(); \
}

#define mHandleLeftRightClick() \
    if ( actclicktags_.isPresent("Check") && actitem_.isCheckable() ) \
	actitem_.setChecked( !actitem_.isChecked(), true ); \
    if ( actclicktags_.isPresent("Left") ) \
	mTreeViewTrigger2( leftButtonClicked, mouseButtonClicked ) \
    else if ( actclicktags_.isPresent("Right") ) \
	mTreeViewTrigger2( rightButtonClicked, mouseButtonClicked ) \
    else \
	mTreeViewTrigger( mouseButtonClicked );

void TreeActivator::actCB( CallBacker* cb )
{
    if ( actitem_.treeView() == &actlview_ &&
	 actcolumn_>=0 && actcolumn_<actlview_.nrColumns() )
    {
	if ( actclicktags_.isPresent("Expand") )
	    actitem_.setOpen( !actitem_.isOpen() );
	else
	{
	    actlview_.setCurrentItem( &actitem_, actcolumn_ );

	    // TODO: Support multi-selection trees (not used in OD yet).

	    if ( actclicktags_.isPresent("Left") )
		mTreeViewTrigger2( leftButtonPressed, mouseButtonPressed )
	    else if ( actclicktags_.isPresent("Right") )
		mTreeViewTrigger2( rightButtonPressed, mouseButtonPressed )
	    else
		mTreeViewTrigger( mouseButtonPressed );

	    mHandleLeftRightClick();
	    if ( actclicktags_.isPresent("Double") )
	    {
		mTreeViewTrigger( doubleClicked );
		mHandleLeftRightClick();
	    }
	}
    }
}


#define mTreeButtonCheck( treenode, pathstr ) \
    if ( !treenode->isCheckable() ) \
    { \
	mWinErrStrm << "Tree node \"" << pathstr.unescapedStr() \
		    << "\" has no button" << std::endl; \
	return false; \
    }

bool TreeButtonCmd::act( const char* parstr )
{
    const char* extraparstr = hasTreeKeyStr(parstr,2) ? parstr : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnext = extraparstr==parstr ? extraparnext : parstr;
    mParPathStrInit( "tree", parnext, parnexxt, treepath );
    mParMouse( parnexxt, parnexxxt, clicktags, "Left" );
    mButtonCmdMouseTagCheck( clicktags );
    mParOnOffInit( parnexxxt, partail, onoff );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
    mTreeButtonCheck( nodesfound[0], pathstr );
    mParOnOffPre( "check-box", onoff, nodesfound[0]->isChecked(), true );
    
    clicktags.add( "Check" );
    mActivate( Tree, Activator(*uilview,*nodesfound[0],clicktags) );

    mParOnOffPost( "check-box", onoff, nodesfound[0]->isChecked() )
    return true;
}


bool TreeExpandCmd::act( const char* parstr )
{
    const char* extraparstr = hasTreeKeyStr(parstr,2) ? parstr : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnext = extraparstr==parstr ? extraparnext : parstr;
    mParPathStrInit( "tree", parnext, parnexxt, treepath );
    mParOnOffInit( parnexxt, partail, onoff );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );

    if ( !nodesfound[0]->nrChildren() )
    {
	mWinErrStrm << "Tree node \"" << pathstr.unescapedStr()
		    << "\" has no expander" << std::endl;
	return false;
    }

    mParOnOffPre( "expander", onoff, nodesfound[0]->isOpen(), true )
    
    BufferStringSet clicktags;
    clicktags.add( "Expand" );
    mActivate( Tree, Activator(*uilview,*nodesfound[0],clicktags) );

    mParOnOffPost( "expander", onoff, nodesfound[0]->isOpen() ) 
    return true;
}


bool TreeMenuCmd::act( const char* parstr )
{
    const char* extraparstr = hasTreeKeyStr(parstr,3) ? parstr : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnext = extraparstr==parstr ? extraparnext : parstr;
    mParTreeTag( parnext, parnexxt, pathcol );
    mParPathStrInit( "tree", parnexxt, parnexxxt, treepath );
    mParItemSelInit("column", parnexxxt, parnexxxxt, itemstr, itemnr, !pathcol);
    mPathColCheck( pathcol, itemnr, parnexxxt, parnexxxxt );
    mParMouse( parnexxxxt, parnexxxxxt, clicktags, "Right" );
    mParPathStrInit( "menu", parnexxxxxt, parnexxxxxxt, menupath );
    mParOnOffInit( parnexxxxxxt, partail, onoff );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
    mParColSelPre( "column",uilview,nodesfound,itemstr,itemnr,columns,true );

    prepareIntercept( menupath, onoff );

    mActivate( Tree, Activator(*uilview,*nodesfound[0],clicktags,columns[0]) );
    
    BufferString objnm("Column "); objnm += columns[0]+1; 
    objnm += " of node \""; objnm += pathstr.unescapedStr(); objnm += "\"";
    return didInterceptSucceed( objnm );
}


bool NrTreeItemsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,2) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParPathStrInit( "tree", parnexxt, partail, treepath );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );

    const uiTreeViewItem* curnode = 0;
    if ( !treepath.isEmpty() )
    {
	mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
	curnode = nodesfound[0];
	if ( curnode->parent() && curnode->parent()->isOpen() &&
	     !curnode->isOpen() && curnode->nrChildren() )
	{
	    mWinWarnStrm << "Tree node \"" << pathstr.unescapedStr()
			 << "\" not expanded" << std::endl;
	}
    }
    mParIdentPost( identname, countTreeItems(*uilview, curnode), parnext );
    return true;
}


bool NrTreeColsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,1) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* partail = extraparstr==parnext ? extraparnext : parnext;
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );

    mParIdentPost( identname, uilview->nrColumns(), parnext );
    return true;
}


bool IsTreeItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,2) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParPathStrInit( "tree", parnexxt, partail, treepath );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );

    const int ison = uilview->selectionMode()==uiTreeView::NoSelection  ? -1 :
		     nodesfound[0]->isSelected() ? 1 : 0;

    mParIdentPost( identname, ison, parnext );
    return true;
}


bool IsTreeItemExpandedCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,2) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParPathStrInit( "tree", parnexxt, partail, treepath );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );

    const int isopen = !nodesfound[0]->nrChildren() ? -1 :
		       nodesfound[0]->isOpen() ? 1 : 0;

    mParIdentPost( identname, isopen, parnext );
    return true;
}


bool IsTreeButtonOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,2) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParPathStrInit( "tree", parnexxt, partail, treepath );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
    mTreeButtonCheck( nodesfound[0], pathstr );

    const int ison = nodesfound[0]->isChecked() ? 1 : 0;

    mParIdentPost( identname, ison, parnext );
    return true;
}


#define mGetReturnPath( uilview, treeitem, form, returnpath ) \
\
    const uiTreeViewItem* curnode = treeitem; \
    BufferStringSet pathitems; \
    while ( curnode ) \
    { \
	mParForm( answer, form, curnode ? curnode->text() : "", \
				countTreeItems(*uilview, curnode, false) ); \
\
	StringProcessor(answer).addCmdFileEscapes( \
					StringProcessor::sAllEscSymbols() ); \
\
	BufferString pathitm = form==Number ? "*#" : ""; \
	pathitm += answer; \
	pathitems.add( pathitm ); \
	curnode = curnode->parent(); \
    } \
\
    FileMultiString returnpath; \
    for ( int idx=pathitems.size()-1; idx>=0; idx-- ) \
	returnpath += pathitems.get( idx );

bool CurTreePathCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,1) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParFramed( parnexxt, parnexxxt, framed );
    mParFormInit( parnexxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );

    const uiTreeViewItem* curitm = framed ? uilview->currentItem() :
					    singleSelected(*uilview);

    mGetReturnPath( uilview, curitm, form, returnpath );
    mParEscIdentPost( identname, returnpath, parnext, false );
    return true;
}


bool CurTreeColCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,1) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParFormInit( parnexxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    const int curcolidx = uilview->currentColumn();
    mParForm( answer, form, uilview->columnText(curcolidx), curcolidx+1 );
    mParIdentPost( identname, answer, parnext );
    return true;
}


bool CurTreeItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,1) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParFramed( parnexxt, parnexxxt, framed );
    mParFormInit( parnexxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );

    const uiTreeViewItem* curitm = framed ? uilview->currentItem() :
					    singleSelected(*uilview);

    mParForm( answer, form, curitm ? curitm->text() : "",
			    countTreeItems(*uilview, curitm, false) );

    mParIdentPost( identname, answer, parnext );
    return true;
}


bool GetTreePathCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,2) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParPathStrInit( "tree", parnexxt, parnexxxt, treepath );
    mParFormInit( parnexxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
    mGetReturnPath( uilview, nodesfound[0], form, returnpath );
    mParEscIdentPost( identname, returnpath, parnext, false );
    return true;
}


bool GetTreeColCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const bool hastreekey = hasTreeKeyStr( parnext, 2,true );
    const char* extraparstr = hastreekey ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParTreeTag( parnexxt, parnexxxt, pathcol );
    mParOptPathStrInit( "tree", parnexxxt, parnexxxxt, treepath, !pathcol );
    if ( !pathcol )
    {
	treepath = "\a<";
	parnexxxxt = parnexxxt;
    }

    mParItemSelInit( "column",parnexxxxt,parnexxxxxt,itemstr,itemnr,false );
    mParFormInit( parnexxxxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, true );
    mParColSelPre( "column",uilview,nodesfound,itemstr,itemnr,columns,true );
    mParForm( answer, form, uilview->columnText(columns[0]),columns[0]+1 );
    mParIdentPost( identname, answer, parnext );
    return true;
}


bool GetTreeItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,2) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParTreeTag( parnexxt, parnexxxt, pathcol );
    mParPathStrInit( "tree", parnexxxt, parnexxxxt, treepath );
    mParItemSelInit( "column",parnexxxxt,parnexxxxxt,itemstr,itemnr,!pathcol );
    mPathColCheck( pathcol, itemnr, parnexxxxt, parnexxxxxt );
    mParFormInit( parnexxxxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
    mParColSelPre( "column",uilview,nodesfound,itemstr,itemnr,columns,true );

    const int count = countTreeItems( *uilview, nodesfound[0], false );
    mParForm( answer, form, nodesfound[0]->text(columns[0]), count );
    mParIdentPost( identname, answer, parnext );
    return true;
}


#define mInterceptTreeMenu( menupath, allowroot, uilview, node, pathstr, col ) \
\
    BufferStringSet clicktags; clicktags.add( "Right" ); \
    CmdDriver::InterceptMode mode = \
		    allowroot ? CmdDriver::NodeInfo : CmdDriver::ItemInfo; \
    prepareIntercept( menupath, 0, mode ); \
    mActivate( Tree, Activator(*uilview, *node, clicktags, col) ); \
    BufferString objnm("Column "); objnm += columns[0]+1; \
    objnm += " of node \""; objnm += pathstr.unescapedStr(); objnm += "\""; \
    if ( !didInterceptSucceed(objnm) ) \
	return false; 

bool NrTreeMenuItemsCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,3) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParTreeTag( parnexxt, parnexxxt, pathcol );
    mParPathStrInit( "tree", parnexxxt, parnexxxxt, treepath );
    mParItemSelInit( "column",parnexxxxt,parnexxxxxt,itemstr,itemnr,!pathcol );
    mPathColCheck( pathcol, itemnr, parnexxxxt, parnexxxxxt );
    mParPathStrInit( "menu", parnexxxxxt, partail, menupath );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
    mParColSelPre( "column",uilview,nodesfound,itemstr,itemnr,columns,true );
    mInterceptTreeMenu(menupath,true,uilview,nodesfound[0],pathstr,columns[0]);

    mParIdentPost( identname, interceptedMenuInfo().nrchildren_, parnext );
    return true;
}


bool IsTreeMenuItemOnCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,3) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParTreeTag( parnexxt, parnexxxt, pathcol );
    mParPathStrInit( "tree", parnexxxt, parnexxxxt, treepath );
    mParItemSelInit( "column",parnexxxxt,parnexxxxxt,itemstr,itemnr,!pathcol );
    mPathColCheck( pathcol, itemnr, parnexxxxt, parnexxxxxt );
    mParPathStrInit( "menu", parnexxxxxt, partail, menupath );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
    mParColSelPre( "column",uilview,nodesfound,itemstr,itemnr,columns,true );
    mInterceptTreeMenu(menupath,false,uilview,nodesfound[0],pathstr,columns[0]);

    mParIdentPost( identname, interceptedMenuInfo().ison_, parnext );
    return true;
}


bool GetTreeMenuItemCmd::act( const char* parstr )
{
    mParIdentInit( parstr, parnext, identname, false );
    const char* extraparstr = hasTreeKeyStr(parnext,3) ? parnext : "\"\a#1\"";
    mParKeyStrInit( "tree", extraparstr, extraparnext, keys, selnr );
    const char* parnexxt = extraparstr==parnext ? extraparnext : parnext;
    mParTreeTag( parnexxt, parnexxxt, pathcol );
    mParPathStrInit( "tree", parnexxxt, parnexxxxt, treepath );
    mParItemSelInit( "column",parnexxxxt,parnexxxxxt,itemstr,itemnr,!pathcol );
    mPathColCheck( pathcol, itemnr, parnexxxxt, parnexxxxxt );
    mParPathStrInit( "menu", parnexxxxxt, parnexxxxxxt, menupath );
    mParFormInit( parnexxxxxxt, partail, form );
    mParTail( partail );

    mFindObjects( objsfound, uiTreeView, keys, nrgrey );
    mParKeyStrPre( "tree", objsfound, nrgrey, keys, selnr );
    mDynamicCastGet( const uiTreeView*, uilview, objsfound[0] );
    mParTreeSelPre( uilview, treepath, nodesfound, pathstr, false );
    mParColSelPre( "column",uilview,nodesfound,itemstr,itemnr,columns,true );
    mInterceptTreeMenu(menupath,false,uilview,nodesfound[0],pathstr,columns[0]);

    const MenuInfo menuinfo = interceptedMenuInfo();
    mParForm( answer, form, menuinfo.text_, menuinfo.siblingnr_ );
    mParIdentPost( identname, answer, parnext );
    return true;
}


//====== CmdComposers =========================================================

static bool findTreePath( const uiTreeView& uilview,
			  const uiTreeViewItem& searchitem,
			  FileMultiString& treepath, bool& casedep,
			  const uiTreeViewItem* root=0 )
{
    bool itemfound = false;
    const uiTreeViewItem* curitem;
    FileMultiString pathtail;

    const int nritems = root ? root->nrChildren() : uilview.nrItems();
    for ( int idx=0; idx<nritems; idx++ )
    {
	curitem = root ? root->getChild(idx) : uilview.getItem(idx);
	if ( !curitem || !curitem->isVisible() || !curitem->isEnabled() )
	    continue;

	if ( curitem == &searchitem )
	{
	    itemfound = true;
	    casedep = false;
	    break;
	}

	if ( findTreePath(uilview, searchitem, pathtail, casedep, curitem) )
	{
	    itemfound = true;
	    break;
	}
    }

    if ( !itemfound )
	return false;

    BufferString curtxt = curitem->text();
    mDressNameString( curtxt, sTreePath );

    int nrmatches = 0;
    int selnr = 0;

    for ( int idx=0; idx<nritems; idx++ )
    {
        const uiTreeViewItem* treeitem = root ? root->getChild(idx) :
						uilview.getItem(idx);

	if ( !treeitem || !treeitem->isVisible() || !treeitem->isEnabled() )
	    continue;

	if ( SearchKey(curtxt,false).isMatching(treeitem->text()) )
	{
	    if ( SearchKey(curtxt,true).isMatching(treeitem->text()) )
	    {
		nrmatches++;
		if ( treeitem == curitem )
		    selnr = nrmatches;
	    }
	    else
		casedep = true;
	}
    }

    if ( selnr && nrmatches>1 )
    {
	curtxt += "#"; curtxt += selnr;
    }

    treepath.setEmpty();
    treepath += curtxt;
    for ( int idx=0; idx<pathtail.size(); idx++ )
	treepath += pathtail[idx];

    return true;
}


void TreeCmdComposer::init()
{
    reInit();
    stagenr_ = -1;

    bursteventnames_.add( "selectionChanged" );

    voideventnames_.add( "selectionChanged" );
    voideventnames_.add( "currentChanged" );
    voideventnames_.add( "itemChanged" );
    voideventnames_.add( "leftButtonPressed" );
    voideventnames_.add( "rightButtonPressed" );
    voideventnames_.add( "mouseButtonPressed" );
    voideventnames_.add( "mouseButtonClicked" );
    voideventnames_.add( "contextMenuRequested" );
}


void TreeCmdComposer::reInit()
{
    stagenr_ = 0;
    selchanged_ = false;
    clickeditem_ = 0;
    clickedcol_ = -1;
    leftclicked_ = false;
    ctrlclicked_ = false;
    treecmdsflushed_ = false;
}


#define mGetTreeView( uilview, retval ) \
\
    if ( eventlist_.isEmpty() ) \
	return retval; \
    mDynamicCastGet( const uiTreeView*, uilview, eventlist_[0]->object_ ); \
    if ( !uilview ) \
	return retval;

void TreeCmdComposer::updateInternalState()
{
    mGetTreeView( uilview, );
    if ( updateflag_ || CmdRecStopper::isInStopperList(uilview) )
    {
	storeTreeState();
	updateflag_ = false;
    }
}


void TreeCmdComposer::storeTreeState()
{
    mGetTreeView( uilview, );
    checkeditems_.erase();
    addToTreeState( *uilview );
}


void TreeCmdComposer::addToTreeState( const uiTreeView& uilview,
                                      const uiTreeViewItem* root )
{
    const int nritems = root ? root->nrChildren() : uilview.nrItems();
    for ( int idx=0; idx<nritems; idx++ )
    {
        const uiTreeViewItem* curitem = root ? root->getChild(idx) :
					       uilview.getItem(idx);

	if ( !curitem || !curitem->isVisible() || !curitem->isEnabled() )
	    continue;

	if ( curitem->isChecked() )
	    checkeditems_ += curitem;

	addToTreeState( uilview, curitem );
    }
}


void TreeCmdComposer::labelStoredStateOld()
{ wascheckeditems_ = checkeditems_; }


void TreeCmdComposer::labelStoredStateNew()
{ ischeckeditems_ = checkeditems_; }


#define mIsSet( iswasselectedchecked, treeitem ) \
    ( iswasselectedchecked##items_.indexOf(treeitem) >= 0 )


#define mGetTreeCmdArgs( event, dqkeystr, treenodesel, casedep, mousetag ) \
\
    mGetTreeView( uilview, ); \
    FileMultiString treepath; \
    bool casedep; \
\
    findTreePath( *uilview, *clickeditem_, treepath, casedep ); \
    BufferString treenodesel = (clickedcol_>0) ? " PathCol \"" : " \""; \
    treenodesel += treepath.unescapedStr(); treenodesel += "\""; \
    if ( clickedcol_ > 0 ) \
    { \
	mGetItemName( uilview, nrColumns, columnText, clickedcol_, \
		      colname, colcasedep ); \
\
	treenodesel += " \""; treenodesel += colname; treenodesel += "\""; \
	casedep = casedep || colcasedep; \
    } \
\
    const CmdRecEvent& event = *eventlist_[eventlist_.size()-1]; \
    BufferString dqkeystr; \
    if ( event.similarobjs_ ) \
    { \
	dqkeystr = " \""; dqkeystr += event.keystr_; dqkeystr += "\""; \
    } \
\
    BufferString mousetag = " "; \
    if ( ctrlclicked_ ) \
	mousetag += "Ctrl"; \
    if ( stagenr_==3 || stagenr_==4 ) \
	mousetag += "Double"; \
    mousetag += leftclicked_ ? "Left" : "Right";



void TreeCmdComposer::writeTreeClick()
{
    mGetTreeCmdArgs( event, dquotedkeystr, treenodesel, casedep, mousetag );

    insertWindowCaseExec( event, casedep );
    mRecOutStrm << "TreeClick" << dquotedkeystr << treenodesel << mousetag
		<< std::endl;
}


void TreeCmdComposer::writeTreeExpand()
{
    mGetTreeCmdArgs( event, dquotedkeystr, treenodesel, casedep, mousetag );

    const char* onoff = expanded_ ? " On" : " Off";

    insertWindowCaseExec( event, casedep );
    mRecOutStrm << "TreeExpand" << dquotedkeystr << treenodesel << onoff
		<< std::endl;
}


void TreeCmdComposer::writeTreeButton()
{
    mGetTreeCmdArgs( event, dquotedkeystr, treenodesel, casedep, mousetag );
    if ( mMatchCI(mousetag, " Left") )
	mousetag.setEmpty();

    const char* onoff = mIsSet(ischecked,clickeditem_) ? " On" : " Off";

    insertWindowCaseExec( event, casedep );
    mRecOutStrm << "TreeButton" << dquotedkeystr << treenodesel << mousetag
		<< onoff << std::endl;
}


void TreeCmdComposer::writeTreeMenu( const CmdRecEvent& menuevent )
{
    mGetTreeCmdArgs( event, dquotedkeystr, treenodesel, casedep, mousetag );
    if ( mMatchCI(mousetag, " Right") )
	mousetag.setEmpty();

    const char* onoff = !menuevent.mnuitm_->isCheckable() ? "" :
			( menuevent.mnuitm_->isChecked() ? " On" : " Off" );

    insertWindowCaseExec( event, casedep || menuevent.casedep_ );
    mRecOutStrm << "TreeMenu" << dquotedkeystr << treenodesel << mousetag
		<< " \"" << menuevent.menupath_ << "\"" << onoff << std::endl;
}


bool TreeCmdComposer::accept( const CmdRecEvent& ev )
{
    const bool accepted = CmdComposer::accept( ev );

    if ( quitflag_ || ignoreflag_ )
	return accepted;

    if ( !accepted && stagenr_<2 && !selchanged_ )
	return false;

    BufferString notifiername;

    if ( accepted )
    {
	if ( done() )
	{
	    updateflag_ = true;
	    notDone();
	}

	if ( ev.nraccepts_ )
	    return true;

	getNextWord( ev.msg_, notifiername.buf() );

	if ( mMatchCI(notifiername, "itemEntered") && !ev.begin_ )
	{
	    shrinkEventList( 1, -3 );
	    voideventnames_.add( "itemEntered" );
	    stagenr_ = 0;
	    //mNotifyTest( uiTreeView, ev.object_, leftButtonClicked );
	    return true;
	}
    }

    if ( ev.begin_ == ev.openqdlg_ )
	return accepted;

    if ( accepted )
    {
        mDynamicCastGet( uiTreeView*, uilview, ev.object_ );

	const bool notileft = mMatchCI( notifiername, "leftButtonClicked" );
	const bool notiright = mMatchCI( notifiername, "rightButtonClicked" );

	if ( stagenr_ == -1 )
	    return true;

	if ( stagenr_ == 0 )
	{
	    shrinkEventList( 3, -2 );
	    labelStoredStateOld();
	    stagenr_ = 1;
	}

	if ( stagenr_ == 1 )
	{
	    if ( notileft || notiright )
	    {
		stagenr_ = 2;
		clickeditem_ = uilview->itemNotified();
		clickedcol_ = uilview->columnNotified();
		leftclicked_ = notileft;
		ctrlclicked_ = false;
		storeTreeState();
		labelStoredStateNew();
	    }
	    if ( mMatchCI(notifiername, "selectionChanged") )
		selchanged_ = true;

	    expanded_ = mMatchCI( notifiername, "expanded" );
	    if ( expanded_ || mMatchCI(notifiername, "collapsed") )
	    {
		stagenr_ = 5;
		clickeditem_ = uilview->itemNotified();
	    }

	    return true;
	}

	if ( stagenr_ == 2 )
	{
	    if ( mMatchCI(notifiername,"doubleClicked") &&
		 clickeditem_==uilview->itemNotified() &&
		 clickedcol_==uilview->columnNotified() )
	    {
		stagenr_ = 3;
		return true;
	    }
	}

	if ( stagenr_ == 3 )
	{
	    if ( notileft || notiright )
	    {
		if ( clickeditem_==uilview->itemNotified() &&
		     clickedcol_==uilview->columnNotified() &&
		     leftclicked_==notileft )
		{
		    stagenr_ = 4;
		}
	    }
	    return true;
	}
    }

    if ( !treecmdsflushed_ )
    {
	const bool treebutchange = mIsSet(ischecked,clickeditem_) !=
	    			   mIsSet(waschecked,clickeditem_);

	if ( stagenr_ == 5 )
	    writeTreeExpand();
	else if ( ev.dynamicpopup_ )
	{
	    if ( treebutchange )
	    {
		mRecOutStrm << "# TreeButtonMenu: Command not yet implemented"
			    << std::endl;
	    }
	    writeTreeMenu( ev );
	}
	else if ( treebutchange )
	    writeTreeButton();
	else
	    writeTreeClick();

	treecmdsflushed_ = true;
    }

    if ( stagenr_ != 3 )
    {
	reInit();
	if ( accepted ) 
	    return accept( ev );
    }

    return accepted;
}


}; // namespace CmdDrive
