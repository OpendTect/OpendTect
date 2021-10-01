#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2010
________________________________________________________________________


-*/

#include "stratmod.h"
#include "compoundkey.h"
#include "stratcontent.h"
#include "typeset.h"
#include "uistring.h"

class PropertyRef;
class PropertyRefSelection;
class UnitOfMeasure;
namespace Math { class Formula; }

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
{ mODTextTranslationClass(Layer);
public:

    typedef CompoundKey	ID;

			Layer(const LeafUnitRef&);
			Layer(const Layer&);
			~Layer();
    Layer&		operator=(const Layer&);

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
    bool		isMath(int) const;
    const LayerValue*	getLayerValue(int) const;
    void		getValues(TypeSet<float>&) const;
    inline float	zBot() const	{ return ztop_ + thickness(); }
    inline float	depth() const	{ return ztop_ + 0.5f * thickness(); }

    inline void		setZTop( float v )		{ ztop_ = v; }
    void		setThickness(float v);
    void		setValue(int,float);
    void		setValue(int,const Math::Formula&,
				 const PropertyRefSelection&,float xpos=0.5f);
    void		setValue(int,const IOPar&,const PropertyRefSelection&);
    void		setValue(int,LayerValue*); //!< becomes mine
    void		setContent( const Content& c )	{ content_ = &c; }
    void		setXPos(float); // only affects Math lay vals

    ID			id() const;	//!< unitRef().fullCode()
    OD::Color		dispColor(bool lith_else_upnode) const;

    static const PropertyRef& thicknessRef();

protected:

    const LeafUnitRef*	ref_;
    float		ztop_;
    ObjectSet<LayerValue> vals_;
    const Content*	content_ = nullptr;

    void		setLV(int,LayerValue*);
};


mExpClass(Strat) LayerValue
{ mODTextTranslationClass(LayerValue);
public:

    virtual LayerValue* clone(const Layer* =nullptr) const	= 0;
    virtual		~LayerValue()			{}
    virtual bool	isSimple() const		{ return false; }
    virtual float	value() const			= 0;

    BufferString	dumpStr() const;
    virtual void	setXPos(float)			{}

};


mExpClass(Strat) SimpleLayerValue : public LayerValue
{ mODTextTranslationClass(SimpleLayerValue);
public:
			SimpleLayerValue( float val )
			    : val_ (val)		{}
    SimpleLayerValue*	clone(const Layer* =nullptr) const
			{ return new SimpleLayerValue(val_); }

    virtual bool	isSimple() const		{ return true; }
    virtual float	value() const			{ return val_; }
    void		setValue( float val )		{ val_ = val; }

protected:

    float		val_;

};


/*!\brief returns a layer value based on Math::Formula. It does not copy the
  Formula, so keep the formula alive while the layer is alive! */

mExpClass(Strat) FormulaLayerValue : public LayerValue
{ mODTextTranslationClass(FormulaLayerValue);
public:

			FormulaLayerValue(const Math::Formula&,
					  const Strat::Layer&,
					  const PropertyRefSelection&,
					  int outpridx,float xpos);
			FormulaLayerValue(const IOPar&,const Strat::Layer&,
					  const PropertyRefSelection&,
					  int outpridx);
			~FormulaLayerValue();
    FormulaLayerValue*	clone(const Layer*) const;

    bool		isBad() const		{ return !errmsg_.isEmpty(); }
    uiString		errMsg() const		{ return errmsg_; }
    void		fillPar(IOPar&) const;
    virtual void	setXPos(float);

    virtual float	value() const;

protected:

				FormulaLayerValue(const Math::Formula&,
				      const Strat::Layer&,float xpos,
				      bool copyform=false);

    const Math::Formula&	form_;
    const Layer&		lay_;
    const bool			myform_;
    float			xpos_;

    TypeSet<int>		inpidxs_;
    mutable TypeSet<double>	inpvals_;
    mutable uiString	        errmsg_;

    void			useForm(const PropertyRefSelection&,int outidx);

};


}; // namespace Strat

