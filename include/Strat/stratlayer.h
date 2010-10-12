#ifndef stratlayer_h
#define stratlayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2010
 RCS:		$Id: stratlayer.h,v 1.9 2010-10-12 12:07:40 cvsbert Exp $
________________________________________________________________________


-*/

#include "compoundkey.h"
#include "typeset.h"
class PropertyRef;

namespace Strat
{
class LeafUnitRef;

/*!\brief data for a layer.

  Layers are atached to a UnitRef. To understand the values, you need access to
  the governing PropertyRefSet, usually attached to the LayerSequence that
  the Layer is part of.
 
 */

mClass Layer
{
public:

    typedef CompoundKey	ID;

			Layer(const LeafUnitRef&);

    const LeafUnitRef&	unitRef() const;
    inline void		setRef( const LeafUnitRef& r )	{ ref_ = &r; }

    inline float	zTop() const		{ return vals_[0]; }
    inline float	thickness() const	{ return vals_[1]; }
    inline int		nrValues() const	{ return vals_.size(); }
    float		value(int) const;	//!< returns undef if necessary
    inline void		setZTop( float v )	{ vals_[0] = v; }
    inline void		setThickness( float v )	{ vals_[1] = v; }
    void		setValue(int,float);	//!< automatically adds space

    inline float	zBot() const	{ return zTop() + thickness(); }
    inline float	depth() const	{ return zTop() + 0.5*thickness(); }

    ID			id() const;		//!< unitRef().fullCode()

    const float*	values() const		{ return vals_.arr(); }

    static const PropertyRef& topDepthRef();
    static const PropertyRef& thicknessRef();

protected:

    const LeafUnitRef*	ref_;
    TypeSet<float>	vals_;

};


}; // namespace Strat

#endif
