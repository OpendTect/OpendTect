/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas 
 Date:		January 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "objectfinder.h"

#include "cmddriverbasics.h"
#include "searchkey.h"

#include "bufstringset.h"
#include "separstr.h"

#include "uibutton.h"
#include "uidockwin.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimainwin.h"
#include "uitoolbar.h"


namespace CmdDrive
{


ObjectFinder::ObjectFinder( const uiMainWin& uimw, bool cs,
			WildcardManager* wildcardman )
    : curwin_( uimw )
    , casesensitive_( cs )
    , wcm_( wildcardman )
{}


#define mResTrue(res,nodelist) { res = true; if ( !nodelist ) return res; }

bool ObjectFinder::findNodes( NodeTag tag,
			      ObjectSet<const CallBacker>* nodelist,
			      const char* searchexpr ) const
{
    bool res = false;

    const ObjectSet<uiToolBar>& toolbars = curwin_.toolBars();
    for ( int tbidx=0; tbidx<toolbars.size(); tbidx++ )
    {
	if ( tag!=Everything && tag!=AllToolbars && tag!=ToolbarBase+tbidx )
	    continue;

	if ( !toolbars[tbidx]->isVisible() )
	    continue;

	ObjectSet<const CallBacker> entities;
	toolbars[tbidx]->getEntityList( entities );
	
	for ( int objidx=0; objidx<entities.size(); objidx++ )
	{
	    if ( findNodes(entities[objidx], nodelist, searchexpr, false) )
		mResTrue(res,nodelist);
	}
    }

    const ObjectSet<uiDockWin>& dockwins = curwin_.dockWins();
    for ( int idx=0; idx<dockwins.size(); idx++ )
    {
	if ( tag!=Everything && tag!=AllDockWins && tag!=DockWinBase+idx )
	    continue;

	if ( findNodes(*dockwins[idx]->topGroup(), nodelist, searchexpr) )
	    mResTrue(res,nodelist);
    }

    if ( tag!=Everything && tag!=CurWinTopGrp )
	return res;
	
    uiGroupObj* curwintopgrp = *const_cast<uiMainWin&>(curwin_).topGroup();
    return findNodes( curwintopgrp, nodelist, searchexpr) || res;
}


static void addAlias( BufferString& name, BufferStringSet& aliases )
{
    mDressNameString( name, sKeyStr );

    // For testing multikeystring generation of CmdRecorder
    // if ( name.size()>2 ) { name[1]='*'; name[2]='\0'; }

    if ( !aliases.addIfNew(name) )
	return;

    for ( int idx=aliases.size()-1; idx>0; idx-- )
    {
	if ( aliases[idx-1]->size() >= aliases[idx]->size() )
	    return;
	aliases.swap( idx-1, idx );
    }
}


void ObjectFinder::getAliases( const CallBacker& entity,
			       BufferStringSet& aliases ) 
{
    aliases.erase();
    const UIEntity uientity( &entity );

    BufferString objtext = uientity.name();
    BufferString tiptext = uientity.toolTip();
    BufferString alttext;

    mDynamicCastGet( const uiLabel*, uilabel, uientity.object() ); 
    if ( uilabel )
	alttext = uilabel->text();

    mDynamicCastGet( const uiButton*, uibut, uientity.object() ); 
    if ( uibut )
    {
	alttext = const_cast<uiButton*>(uibut)->text();

	StringProcessor(objtext).filterAmpersands();
	StringProcessor(alttext).filterAmpersands();
    }

    if ( uientity.action() )
	StringProcessor(objtext).filterAmpersands();

    addAlias( objtext, aliases );

    if ( !tiptext.isEmpty() )
	addAlias( tiptext, aliases );

    if ( !alttext.isEmpty() )
	addAlias( alttext, aliases );
}


bool ObjectFinder::findNodes( const CallBacker* root,  
			      ObjectSet<const CallBacker>* nodelist,
       			      const char* searchexpr, bool visonly ) const
{
    bool res = false;
    const UIEntity uientity( root );

    if ( !uientity.isValid() || (visonly && !uientity.visible()) )
	return res;

    if ( !searchexpr )
    {
	mResTrue(res,nodelist);
	*nodelist += root;
    }
    else
    {
	BufferStringSet aliases;
	getAliases( *root, aliases );
	const SearchKey key( searchexpr, casesensitive_ );

	for ( int idx=0; idx<aliases.size(); idx++ )
	{
	    if ( key.isMatching(*aliases[idx]) )
	    {
		if ( wcm_ )
		    wcm_->check( key, *aliases[idx] );

		mResTrue(res,nodelist);
		*nodelist += root;
		break;
	    }
	}
    }

    const ObjectSet<uiBaseObject>* children = uientity.childList();

    for ( int idx=0; children && idx<children->size(); idx++ )
    {
	mDynamicCastGet( const uiObject*, uiobj, (*children)[idx] );
	if ( !uiobj ) 
	    continue;
	if ( findNodes(uiobj, nodelist, searchexpr, visonly) )
	    mResTrue(res,nodelist);
    }

    return res;
}


bool ObjectFinder::findNodes( NodeTag tag, const CallBacker* root,
			      ObjectSet<const CallBacker>* nodelist,
			      const char* searchexpr ) const
{
    if ( tag != UiObjNode )
	return findNodes( tag, nodelist, searchexpr );

    const UIEntity uientity( root );
    if ( uientity.isValid() )
	return findNodes( root, nodelist, searchexpr, uientity.visible() );

    return false;
}


bool ObjectFinder::isKeyInTree( NodeTag tag, const CallBacker* root,
				const char* searchexpr ) const
{
    return findNodes( tag, root, 0, searchexpr );
}


int ObjectFinder::toolBarIndex( const CallBacker* entity ) const
{
    const ObjectSet<uiToolBar>& toolbars = curwin_.toolBars();
    for ( int tbidx=0; entity && tbidx<toolbars.size(); tbidx++ )
    {
	ObjectSet<const CallBacker> entities;
	toolbars[tbidx]->getEntityList( entities );
	const int objidx = entities.indexOf( entity );

	if ( objidx>=0 )
	  return tbidx;
    }
    return -1;
}


int ObjectFinder::dockWinIndex( const CallBacker* entity ) const
{
    const ObjectSet<uiDockWin>& dockwins = curwin_.dockWins();
    for ( int idx=0; entity && idx<dockwins.size(); idx++ )
    {
	if ( entity == *dockwins[idx]->topGroup() )
	    return idx;
    }
    return -1;
}


#define mReturnTag(tag) { curtag=tag; curnode=0; return true; }

bool ObjectFinder::getAncestor( NodeTag& curtag,
				const CallBacker*& curnode ) const
{
    if ( curtag==CurWinTopGrp || curtag==AllToolbars || curtag==AllDockWins )
	mReturnTag( Everything );
    
    const ObjectSet<uiToolBar>& toolbars = curwin_.toolBars();
    if ( curtag>=ToolbarBase && curtag<ToolbarBase+toolbars.size() )
	mReturnTag( AllToolbars );

    const ObjectSet<uiDockWin>& dockwins = curwin_.dockWins();
    if ( curtag>=DockWinBase && curtag<DockWinBase+dockwins.size() )
	mReturnTag( AllDockWins );

    if ( curtag != UiObjNode )
	return false;

    const int toolbaridx = toolBarIndex( curnode );
    if ( toolbaridx >= 0 )
	mReturnTag( (NodeTag)(ToolbarBase+toolbaridx) );

    const UIEntity uientity( curnode );
    if ( !uientity.parent() )
	return false;

    curnode = uientity.parent()->mainObject();

    const int dockwinidx = dockWinIndex( curnode );
    if ( dockwinidx >= 0 )
	curtag = (NodeTag)(DockWinBase+dockwinidx);

    if ( curnode == *const_cast<uiMainWin&>(curwin_).topGroup() )
    	curtag = CurWinTopGrp;
    
    return true;
}


bool ObjectFinder::selectNodes( ObjectSet<const CallBacker>& nodesfound,
				const FileMultiString& keys,
       				int* unfoundkeyidx ) const
{
    for ( int keyidx=0; keyidx<mSepStrSize(keys); keyidx++ )
    {
	ObjectSet<const CallBacker> commonancestorobj;
	commonancestorobj.allowNull( true );
	TypeSet<NodeTag> commonancestortag;
	for ( int idx=0; idx<nodesfound.size(); idx++ )
	{
	    NodeTag curtag = UiObjNode;
	    const CallBacker* curnode = nodesfound[idx];
	    
	    while ( true )
	    {
		if ( isKeyInTree(curtag, curnode, keys[keyidx]) )
		{
		    commonancestortag += curtag;
		    commonancestorobj += curnode;
		    break;
		}

		if ( !getAncestor(curtag, curnode) )
		{
		    nodesfound.erase();
		    if ( unfoundkeyidx )
			*unfoundkeyidx = keyidx;
		    return false;
		}	    
	    }
	}

	for ( int idx=0; idx<nodesfound.size(); idx++ )
	{
	    NodeTag curtag = commonancestortag[idx];
	    const CallBacker* curnode = commonancestorobj[idx];

	    while ( getAncestor(curtag, curnode) )
	    {
		for ( int idy=nodesfound.size()-1; idy>=0; idy-- )
		{
		    if ( commonancestortag[idy]==curtag &&
			 commonancestorobj[idy]==curnode )
		    {
			nodesfound.removeSingle( idy );
			commonancestortag.removeSingle( idy );
			commonancestorobj.removeSingle( idy );
			if ( idx >= idy )
			    idx--;
		    }
		}
	    }
	}
    }

    return true;
}


int ObjectFinder::deleteGreys( ObjectSet<const CallBacker>& objsfound, bool yn )
{
    int nrgreyfound = 0;
    for ( int idx=objsfound.size()-1; idx>=0; idx-- )
    {
	if ( !UIEntity(objsfound[idx]).sensitive() ) 
	{
	    nrgreyfound++;
	    if ( yn )
		objsfound.removeSingle( idx );
	}
    }

    return nrgreyfound;
}


}; // namespace CmdDrive;
