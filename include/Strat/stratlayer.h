#ifndef stratlayer_h
#define stratlayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2010
 RCS:		$Id: stratlayer.h,v 1.5 2010-09-06 13:57:50 cvsbert Exp $
________________________________________________________________________


-*/

#include "objectset.h"
#include "compoundkey.h"
#include "stratunitref.h"
class Property;

namespace Strat
{
class LeafUnitRef;

/*!\brief data for a layer */

mClass Layer
{
public:

    typedef CompoundKey	ID;

			Layer( const LeafUnitRef* r )
			: ref_(r)			{}

    const LeafUnitRef&	unitRef() const;
    void		setRef( const LeafUnitRef* r )	{ ref_ = r; }

    ObjectSet<Property>&	properties()		{ return props_; }
    const ObjectSet<Property>&	properties() const	{ return props_; }

    ID			id() const; // returns unitRef().fullCode()

protected:

    const LeafUnitRef*	ref_;
    ObjectSet<Property>	props_;

};


}; // namespace Strat

#endif
