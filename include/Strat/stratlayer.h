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
#include "compoundkey.h"
#include "mathformula.h"
#include "stratcontent.h"
#include "typeset.h"

class PropertyRef;

namespace Strat
{
class LeafUnitRef;
class RefTree;
class Lithology;
class LayerValue;

/*!\brief data for a layer.

  Layers are atached to a UnitRef. To understand the values, you need access to
  the governing PropertyRefSet, usually attached to the LayerSequence that
  the Layer is part of.

 */

mExpClass(Strat) Layer
{
public:

    typedef CompoundKey	ID;

			Layer(const LeafUnitRef&);

    BufferString	name() const;
    const LeafUnitRef&	unitRef() const			{ return *ref_; }
    inline void		setRef( const LeafUnitRef& r )	{ ref_ = &r; }
    const RefTree&	refTree() const;
    const Lithology&	lithology() const;
    const Content&	content() const;

    inline float	zTop() const			{ return ztop_; }
    inline int		nrValues() const		{ return vals_.size(); }
    float		thickness() const;
    float		value(int) const;		//!< can be undef
    inline void		setZTop( float v )		{ ztop_ = v; }
    void		setThickness(float v);
    void		setValue(int,float);
    void		setFormula(int,const Math::Formula&);
    void		setContent( const Content& c )	{ content_ = &c; }

    inline float	zBot() const	{ return zTop() + thickness(); }
    inline float	depth() const	{ return zTop() + 0.5f * thickness(); }

    ID			id() const;	//!< unitRef().fullCode()
    Color		dispColor(bool lith_else_upnode) const;

    void		values(float*) const;

    static const PropertyRef& thicknessRef();

protected:

    const LeafUnitRef*		ref_;
    float			ztop_;
    ObjectSet<LayerValue>	vals_;
    TypeSet<TypeSet<int> >	inpidxes_;
    const Content*		content_;

};


mExpClass(Strat) LayerValue
{
public:

    virtual float	value(float*) const				=0;
    virtual void	setFormula(Math::Formula)			{};
    virtual void	setValue(float)					{};
};


mExpClass(Strat) SimpleLayerValue : public LayerValue
{
public:
			SimpleLayerValue()
			    : val_ (mUdf(float))	{};
			SimpleLayerValue( float val )
			    : val_ (val)		{};

    virtual float	value(float*) const		{ return val_; }
    void		setValue(float val)		{ val_ = val; }

protected:

    float		val_;
};


mExpClass(Strat) FormulaLayerValue : public LayerValue
{
public:
			FormulaLayerValue(Math::Formula);
    virtual float	value(float*) const;
    virtual void	setFormula(Math::Formula);

protected:

    Math::Formula	formula_;
};


}; // namespace Strat

#endif

