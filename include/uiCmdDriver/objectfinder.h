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

#include "sets.h"

class FileMultiString;
class uiMainWin;
class uiObject;
class BufferStringSet;

namespace CmdDrive
{

class WildcardManager;

mClass ObjectFinder
{
public:
    			ObjectFinder(const uiMainWin&,bool casesensitive=true,
				     WildcardManager* =0);

    enum		NodeTag { Everything, AllToolbars, AllDockWins,
				  CurWinTopGrp, UiObjNode,
				  ToolbarBase=1000, DockWinBase=2000 };

    bool		findNodes(NodeTag,ObjectSet<const uiObject>*,
				  const char* searchexpr=0) const;
    bool		findNodes(const uiObject* root,
				  ObjectSet<const uiObject>*,
				  const char* searchexpr=0,
				  bool visonly=true) const;
    bool		findNodes(NodeTag,const uiObject* root,
				  ObjectSet<const uiObject>*,
				  const char* searchexpr=0) const;

    bool		selectNodes(ObjectSet<const uiObject>&,
				    const FileMultiString& keys,
				    int* unfoundkeyidx=0) const;

    static int		deleteGreys(ObjectSet<const uiObject>&,bool yn=true);
    
    static void		getAliases( const uiObject&,
				    BufferStringSet& aliases );

    bool 		getAncestor(NodeTag& curtag,
	    			    const uiObject*& curnode) const;

protected:

    bool		isKeyInTree(NodeTag,const uiObject* root,
				    const char* searchexpr) const;

    int 		toolBarIndex(const uiObject*) const;
    int 		dockWinIndex(const uiObject*) const;

    const uiMainWin&	curwin_;
    bool		casesensitive_;

    WildcardManager*	wcm_;
};


}; // namespace CmdDrive


#endif

