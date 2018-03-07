#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2003
________________________________________________________________________

-*/

#include "stratmod.h"
#include "repos.h"
#include "uistring.h"

namespace Strat
{
class RefTree;

/*!
\ingroup Strat
\brief Repository of all stratigraphic descriptions defining the building
	  blocks of subsurface descriptions
*/

mExpClass(Strat) RepositoryAccess
{ mODTextTranslationClass(RepositoryAccess);
public:

    bool		haveTree() const;
    Repos::Source	lastSource() const	{ return src_; }
    uiString		lastMsg() const		{ return msg_; }

    RefTree*		readTree(Repos::Source);
    bool		writeTree(const RefTree&,
				  Repos::Source src=Repos::Survey);

    static const char*	fileNameBase();

    RefTree*		readTree();	//!< Read the 'best' tree

protected:

    mutable uiString	        msg_;
    mutable Repos::Source	src_;

};

} // namespace Strat
