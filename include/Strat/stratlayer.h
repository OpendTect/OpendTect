#ifndef stratlayer_h
#define stratlayer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Jan 2004
 RCS:		$Id: stratlayer.h,v 1.1 2004-01-05 15:40:20 bert Exp $
________________________________________________________________________


-*/

#include "stratunitref.h"

namespace Strat
{

/*!\brief data for a layer */

class Layer : public Unit
{
public:

			Layer( const LeafUnitRef* r )
			: ref_(r)			{}

    const UnitRef*	unitRef() const			{ return ref_; }
    const LeafUnitRef*	leafUnitRef() const		{ return ref_; }

protected:

    LeafUnitRef*	ref_;

};


}; // namespace Strat

#endif
