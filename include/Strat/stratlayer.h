#ifndef stratlayer_h
#define stratlayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Jan 2004
 RCS:		$Id: stratlayer.h,v 1.4 2009-07-22 16:01:19 cvsbert Exp $
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
