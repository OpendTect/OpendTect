#ifndef stratfw_h
#define stratfw_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratunitrepos.h,v 1.3 2004-02-17 10:58:52 bert Exp $
________________________________________________________________________

-*/

#include "stratunitref.h"
class PropertyRef;

namespace Strat
{

class Lithology;

/*!\brief Stratigraphic framework defining the stratigraphic building blocks
         of subsurface descriptions */

class FW : public NodeUnitRef
{
public:

				FW() : NodeUnitRef(0,"","All")	{}

    ObjectSet<Lithology>	liths_;
    ObjectSet<PropertyRef>	props_;

};


}; // namespace Strat


/*!\mainpage Stratigraphy

This module supports hierarchical naming of units, with supporting tools like
property assignment and lithology selection.

Although a stratigraphic framework can be used independently of any geometry,
an easy way to think about it is how it could be used for wells. Every layer
in a well can be interpreted as being a specific instantiation of an abstract
unit defined in the framework. For example, a sand layer in the upper
cretaceous could be labeled cret.upp.sand . Two layers further a similar sand
layer can have the same label. Both are then instantiations of this
cret.upp.sand .

Such a classification system invariably has a tree structure. The nodes in the
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
