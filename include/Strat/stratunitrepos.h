#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"
#include "repos.h"
#include "uistring.h"

class MultiID;

namespace Strat
{
class RefTree;

/*!
\ingroup Strat
\brief Repository of all stratigraphic descriptions defining the building
	  blocks of subsurface descriptions
*/

mExpClass(Strat) RepositoryAccess
{ mODTextTranslationClass(RepositoryAccess)
public:
			RepositoryAccess();
			~RepositoryAccess();

    bool		haveTree() const;
    uiString		lastMsg() const		{ return msg_; }

    RefTree*		read(const MultiID&);
    bool		write(const RefTree&,const MultiID&);

    static const char*	fileNameBase();

    RefTree*		readTree();	//!< Read the 'best' tree

protected:

    RefTree*		readFromFile(const char*);
    bool		writeToFile(const RefTree&,const char*);

    mutable uiString		msg_;
    mutable Repos::Source	src_;

public:
    Repos::Source	lastSource() const	{ return src_; }
    RefTree*		readTree(Repos::Source);
    bool		writeTree(const RefTree&,
					  Repos::Source src=Repos::Survey);
};

} // namespace Strat
