#ifndef stratfw_h
#define stratfw_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratunitrepos.h,v 1.2 2004-01-06 17:12:02 bert Exp $
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

#endif
