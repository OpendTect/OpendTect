#ifndef stratfw_h
#define stratfw_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratunitrepos.h,v 1.1 2003-12-15 17:29:43 bert Exp $
________________________________________________________________________

-*/

#include "sets.h"

namespace Strat
{

class Unit;
class Lithology;
class Property;

/*!\brief Stratigraphic framework defining the stratigraphic building blocks
         of subsurface descriptions */

class FW
{
public:

				FW() : topunit(0)		{}

    ObjectSet<Lithology>	liths_;
    ObjectSet<Property>		props_;
    Unit*			topunit_;

};


}; // namespace Strat

#endif
