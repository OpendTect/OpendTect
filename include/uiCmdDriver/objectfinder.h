#ifndef objectfinder_h
#define objectfinder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          January 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "sets.h"

class FileMultiString;
class uiMainWin;
class BufferStringSet;
class CallBacker;

namespace CmdDrive
{

class WildcardManager;

mExpClass(uiCmdDriver) ObjectFinder
{
public:
    			ObjectFinder(const uiMainWin&,bool casesensitive=true,
				     WildcardManager* =0);

    enum		NodeTag { Everything, AllToolbars, AllDockWins,
				  CurWinTopGrp, UiObjNode,
				  ToolbarBase=1000, DockWinBase=2000 };

    bool		findNodes(NodeTag,ObjectSet<const CallBacker>* nodelist,
				  const char* searchexpr=0) const;
    bool		findNodes(const CallBacker* root,
				  ObjectSet<const CallBacker>* nodelist,
				  const char* searchexpr=0,
				  bool visonly=true) const;
    bool		findNodes(NodeTag,const CallBacker* root,
				  ObjectSet<const CallBacker>* nodelist,
				  const char* searchexpr=0) const;

    bool		selectNodes(ObjectSet<const CallBacker>& nodesfound,
				    const FileMultiString& keys,
				    int* unfoundkeyidx=0) const;

    static int		deleteGreys(ObjectSet<const CallBacker>& objsfound,
				    bool yn=true);
    
    static void		getAliases( const CallBacker& entity,
				    BufferStringSet& aliases );

    bool 		getAncestor(NodeTag& curtag,
	    			    const CallBacker*& curnode) const;

protected:

    bool		isKeyInTree(NodeTag,const CallBacker* root,
				    const char* searchexpr) const;

    int 		toolBarIndex(const CallBacker* entity ) const;
    int 		dockWinIndex(const CallBacker* entity ) const;

    const uiMainWin&	curwin_;
    bool		casesensitive_;

    WildcardManager*	wcm_;
};


}; // namespace CmdDrive


#endif

