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

/*!\brief Repository of all stratigraphic descriptions defining the building
	  blocks of subsurface descriptions */

mClass(Strat) RepositoryAccess
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

}; // namespace Strat


/*!\mainpage Stratigraphy

This module supports hierarchical naming of units, with supporting tools like
lithology selection.

Although a stratigraphic framework can be used independently of any geometry,
an easy way to think about it is how it could be used for wells. Every layer
in a well can be interpreted as being a specific instantiation of an abstract
unit defined in the framework. For example, a sand layer in the upper
cretaceous could be labeled cret.upp.sand . Two layers further a similar sand
layer can have the same label. Both are then instantiations of this
cret.upp.sand 'template unit'.

A classification system invariably has a tree structure. The nodes in the
tree are stratigraphic units, the leaves are lithologies. Every unit can have
a number of properties (porosity, sand/shale ratio, etc.).

A well interpretation defines a linear sequence of instantiated units, i.e.
instantiations of the stratigraphic framework are often simply lists of units.
Keep in mind that this is not necessary: any network of connections (not just
the array-like ordering) between instantiations may be interesting in certain
problems.

To implement these thoughts the separation 'reference data' and 'actual
subsurface unit' has been made. A layer is a unit that holds a pointer to 
a conceptual unit (e.g. there are a couple of sand layers connected to
the cret.upp.sand reference data).

*/

#endif

