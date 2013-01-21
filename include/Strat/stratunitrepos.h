#ifndef stratunitrepos_h
#define stratunitrepos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id$
________________________________________________________________________

-*/

#include "stratmod.h"
#include "repos.h"

namespace Strat
{
class RefTree;

/*!
\ingroup Strat  
\brief Repository of all stratigraphic descriptions defining the building
	  blocks of subsurface descriptions
*/

mExpClass(Strat) RepositoryAccess
{
public:

    bool		haveTree() const;
    Repos::Source	lastSource() const	{ return src_; }
    const char*		lastMsg() const		{ return msg_.buf(); }

    RefTree*		readTree(Repos::Source);
    bool		writeTree(const RefTree&,
	    			  Repos::Source src=Repos::Survey);

    static const char*	fileNameBase();

    RefTree*		readTree();	//!< Read the 'best' tree

protected:

    mutable BufferString	msg_;
    mutable Repos::Source	src_;

};

} // namespace Strat

#endif

