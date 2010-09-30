#ifndef stratlayer_h
#define stratlayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2010
 RCS:		$Id: stratlayer.h,v 1.6 2010-09-30 10:58:10 cvsbert Exp $
________________________________________________________________________


-*/

#include "compoundkey.h"
#include "property.h"
class Property;
class PropertySet;

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
    inline void		setRef( const LeafUnitRef* r )	{ ref_ = r; }

    PropertySet&	properties()		{ return props_; }
    const PropertySet&	properties() const	{ return props_; }

    ID			id() const; // returns unitRef().fullCode()

protected:

    const LeafUnitRef*	ref_;
    PropertySet		props_;

};


}; // namespace Strat

#endif
