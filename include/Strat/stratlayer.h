#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"

#include "compoundkey.h"
#include "property.h"
#include "ptrman.h"
#include "stratcontent.h"
#include "typeset.h"
#include "uistring.h"

namespace Math { class Formula; }

namespace Strat
{
class LeafUnitRef;
class RefTree;
class Lithology;
class LayerValue;

/*!\brief Refcounted Math::Formula snapshot for layer evaluation.

  Many FormulaLayerValue instances can share one SharedFormula so that
  generated layer models stay cheap in memory while remaining safe if the
  originating MathProperty is destroyed or replaced.
*/

mExpClass(Strat) SharedFormula : public ReferencedObject
{
public:
			SharedFormula(const Math::Formula&);

    Math::Formula&	form();
    const Math::Formula& form() const;

protected:
			~SharedFormula();

private:

    Math::Formula*	form_;

};


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
    void		getValues(float*,int sz) const;
    inline float	zBot() const	{ return ztop_ + thickness(); }
    inline float	depth() const	{ return ztop_ + 0.5f * thickness(); }

    inline void		setZTop( float v )		{ ztop_ = v; }
    void		setThickness(float v);
    void		setValue(int,float);
    void		setValue(int,const Math::Formula&,
				 const PropertyRefSelection&,float xpos=0.5f);
    void		setValue(int,const Math::Formula&,
				 const PropertyRefSelection&,
				 const Property::EvalOpts&);
    void		setValue(int,const SharedFormula&,
				 const PropertyRefSelection&,float xpos=0.5f);
    void		setValue(int,const SharedFormula&,
				 const PropertyRefSelection&,
				 const Property::EvalOpts&);
    void		setValue(int,const IOPar&,const PropertyRefSelection&);
    void		setValue(int,LayerValue*); //!< becomes mine
    void		setContent(const Content&);
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
    virtual		~LayerValue();
    virtual bool	isSimple() const		{ return false; }
    virtual float	value() const			= 0;

    BufferString	dumpStr() const;
    virtual void	setXPos(float)			{}

protected:
			LayerValue();

};


mExpClass(Strat) SimpleLayerValue : public LayerValue
{ mODTextTranslationClass(SimpleLayerValue);
public:
			SimpleLayerValue(float);
			~SimpleLayerValue();
    SimpleLayerValue*	clone(const Layer* =nullptr) const override
			{ return new SimpleLayerValue(val_); }

    bool		isSimple() const override	{ return true; }
    float		value() const override		{ return val_; }
    void		setValue( float val )		{ val_ = val; }

private:

    float		val_;

};


/*!\brief Layer value evaluated from a Math::Formula.

  Holds a ConstRefMan to a SharedFormula. Prefer attaching an already-shared
  formula (LayerModel::getSharedFormula / generator cache) so thousands of
  layers can share one formula snapshot.
*/

mExpClass(Strat) FormulaLayerValue : public LayerValue
{ mODTextTranslationClass(FormulaLayerValue);
public:

			FormulaLayerValue(const Math::Formula&,
					  const Strat::Layer&,
					  const PropertyRefSelection&,
					  int outpridx,float xpos);
			FormulaLayerValue(const Math::Formula&,
					  const Strat::Layer&,
					  const PropertyRefSelection&,
					  int outpridx,
					  const Property::EvalOpts&);
			FormulaLayerValue(const SharedFormula&,
					  const Strat::Layer&,
					  const PropertyRefSelection&,
					  int outpridx,float xpos);
			FormulaLayerValue(const SharedFormula&,
					  const Strat::Layer&,
					  const PropertyRefSelection&,
					  int outpridx,
					  const Property::EvalOpts&);
			FormulaLayerValue(const IOPar&,const Strat::Layer&,
					  const PropertyRefSelection&,
					  int outpridx);
			~FormulaLayerValue();
    FormulaLayerValue*	clone(const Layer*) const override;

    bool		isBad() const		{ return !errmsg_.isEmpty(); }
    uiString		errMsg() const		{ return errmsg_; }
    void		fillPar(IOPar&) const;
    void		setXPos(float) override;
    void		setRelZ(float);

    float		value() const override;

protected:

				FormulaLayerValue(const SharedFormula&,
				      const Strat::Layer&,float xpos);

    ConstRefMan<SharedFormula>	form_;
    const Layer&		lay_;
    float			xpos_			= 0.f;
    float			relz_			= 0.f;

    TypeSet<int>		inpidxs_;
    mutable TypeSet<double>	inpvals_;
    mutable uiString	        errmsg_;

    void			useForm(const PropertyRefSelection&,int outidx);
    const Math::Formula&	form() const	{ return form_->form(); }
    Math::Formula&		form()
				{ return getNonConst(form_->form()); }

};


} // namespace Strat
