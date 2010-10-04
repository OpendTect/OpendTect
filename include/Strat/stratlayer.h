#ifndef stratlayer_h
#define stratlayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2010
 RCS:		$Id: stratlayer.h,v 1.7 2010-10-04 08:14:43 cvsbert Exp $
________________________________________________________________________


-*/

#include "compoundkey.h"
#include "property.h"
class Property;
class PropertySet;

namespace Strat
{
class LeafUnitRef;

/*!\brief data for a layer.

  Layers are atached to a UnitRef. They have Property's, and the first
  of the properties is always 'Thickness'. Initially, this Property will be
  a ValueProperty. You can later replace this with another type, but
  thou shalt not remove it!
 
 */

mClass Layer
{
public:

    typedef CompoundKey	ID;

			Layer(const LeafUnitRef&,float thickness=0);

    const LeafUnitRef&	unitRef() const;
    inline void		setRef( const LeafUnitRef& r )	{ ref_ = &r; }

    PropertySet&	properties()		{ return props_; }
    const PropertySet&	properties() const	{ return props_; }
    Property&		thickness()		{ return props_.get(0); }
    const Property&	thickness() const	{ return props_.get(0); }

    ID			id() const; // returns unitRef().fullCode()

    static const PropertyRef& thicknessRef();

protected:

    const LeafUnitRef*	ref_;
    PropertySet		props_;

};


}; // namespace Strat

#endif
