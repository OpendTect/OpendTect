#ifndef stratlayer_h
#define stratlayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2010
 RCS:		$Id$
________________________________________________________________________


-*/

#include "stratmod.h"
#include "stratcontent.h"
#include "compoundkey.h"
#include "typeset.h"
class Color;
class PropertyRef;

namespace Strat
{
class LeafUnitRef;
class RefTree;
class Lithology;

/*!\brief data for a layer.

  Layers are atached to a UnitRef. To understand the values, you need access to
  the governing PropertyRefSet, usually attached to the LayerSequence that
  the Layer is part of.
 
 */

mClass(Strat) Layer
{
public:

    typedef CompoundKey	ID;

			Layer(const LeafUnitRef&);

    BufferString	name() const;
    const LeafUnitRef&	unitRef() const;
    inline void		setRef( const LeafUnitRef& r )	{ ref_ = &r; }
    const RefTree&	refTree() const;
    const Lithology&	lithology() const;
    const Content&	content() const;

    inline float	zTop() const			{ return ztop_; }
    inline float	thickness() const		{ return vals_[0]; }
    inline int		nrValues() const		{ return vals_.size(); }
    float		value(int) const;		//!< can be undef
    inline void		setZTop( float v )		{ ztop_ = v; }
    inline void		setThickness( float v )		{ vals_[0] = v; }
    void		setValue(int,float);
    void		setContent( const Content& c )	{ content_ = &c; }

    inline float	zBot() const	{ return zTop() + thickness(); }
    inline float	depth() const	{ return zTop() + 0.5f * thickness(); }

    ID			id() const;	//!< unitRef().fullCode()
    Color		dispColor(bool lith_else_upnode) const;

    const float*	values() const	{ return vals_.arr(); }

    static const PropertyRef& thicknessRef();

protected:

    const LeafUnitRef*	ref_;
    float		ztop_;
    TypeSet<float>	vals_;
    const Content*	content_;

};


}; // namespace Strat

#endif

