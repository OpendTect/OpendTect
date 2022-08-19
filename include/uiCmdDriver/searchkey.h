#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "bufstringset.h"

class uiMainWin;


namespace CmdDrive {


#define mMatchCI(expr1,expr2) caseInsensitiveEqual(expr1,expr2,0)

#define mSearchKey(expr) SearchKey(expr,drv_.isCaseSensitive())

class WildcardManager;

mExpClass(uiCmdDriver) SearchKey
{
public:
    			SearchKey(const char* expr,bool casesensitive=true);

    bool 		isMatching(const char* name) const;

    void		getMatchingWindows(const uiMainWin* applwin,
					   ObjectSet<uiMainWin>&,
					   WildcardManager* =0) const;
    			//!< Null uiMainWin* refers to matching open QDialog

    const BufferStringSet& wildcardList() const	{ return wildcardlist_; }

protected:

    BufferString	searchexpr_;
    bool		casesensitive_;

    bool		isMatch(const char* keyptr,const char* nameptr,
				const char* orgnameptr) const;

    mutable BufferStringSet wildcardlist_;

};


mExpClass(uiCmdDriver) WildcardManager
{
public:
    			WildcardManager();
			~WildcardManager();

    void		reInit();
    void		check(const SearchKey&,const char* name,
			      bool addescapes=true);
    void		flush(bool yn=true);

    int			nrWildcards() const	{ return wildcards_.size(); }

    const BufferString* wildcard(int) const;
    const BufferString* wildcardStr(int) const;

protected:

    BufferStringSet	wildcards_;
    BufferStringSet	wildcardstrings_;
    BufferStringSet	newwildcards_;
    BufferStringSet	newwildcardstrings_;
};

} // namespace CmdDrive
