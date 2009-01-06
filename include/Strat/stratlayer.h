#ifndef stratlayer_h
#define stratlayer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Jan 2004
 RCS:		$Id: stratlayer.h,v 1.3 2009-01-06 05:34:47 cvsranojay Exp $
________________________________________________________________________


-*/

#include "stratunitref.h"

namespace Strat
{

/*!\brief data for a layer */

mClass Layer : public Unit
{
public:

			Layer( const LeafUnitRef* r )
			: ref_(r)			{}

    const UnitRef*	unitRef() const			{ return ref_; }
    const LeafUnitRef*	leafUnitRef() const		{ return ref_; }

protected:

    const LeafUnitRef*	ref_;

};


}; // namespace Strat

#endif
