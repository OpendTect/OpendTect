#ifndef mathproperty_h
#define mathproperty_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Jan 2004
 RCS:		$Id$
________________________________________________________________________


-*/

#include "property.h"
class MathExpression;
class UnitOfMeasure;


/*!\brief Calculated property
 
  When creating a formula, be sure to use ensureGoodVariableName() on the
  property names. This will be done on the available properties too to
  create the match. In theory, this may create ambiguous formulas, but
  at least we can keep things simple this way.

 */

mClass MathProperty : public Property
{
public:
    			MathProperty(const PropertyRef&,const char* def=0);
    			MathProperty(const MathProperty&);
			~MathProperty();

    int			nrInputs() const;
    const char*		inputName(int) const;
    const UnitOfMeasure* inputUnit(int) const;
    void		setInputUnit(int idx,const UnitOfMeasure*);
    int			nrConsts() const;
    const char*		constName(int) const;
    bool		haveInput( int idx ) const    { return inps_[idx]; }
    void		setInput(int,const Property*);
    			//!< Must be done for all inputs after each setDef()
    float		constValue(int) const;
    void		setConst(int,float);

    virtual bool	init(const PropertySet&) const;
    virtual const char*	errMsg() const		{ return errmsg_.buf(); }
    virtual bool	dependsOn(const Property&) const;
    void		setUnit( const UnitOfMeasure* u )	{ uom_ = u; }
    const UnitOfMeasure* unit() const				{ return uom_; }

    mDefPropertyFns(MathProperty,"Math");

    static void		ensureGoodVariableName(char*);
    bool		isDepOn(const Property&) const;

protected:

    BufferString		def_;
    const UnitOfMeasure*	uom_;

    mutable MathExpression*	expr_;
    mutable ObjectSet<const Property> inps_;
    mutable ObjectSet<const UnitOfMeasure> inpunits_;
    mutable TypeSet<float>	consts_;
    mutable BufferString	errmsg_;
    mutable BufferString	fulldef_;

    const Property*		findInput(const PropertySet&,const char*,
	    				  bool) const;
    void			addDefInpUnit() const;

public:
    PropertyRef::StdType	inputType(int) const;

};


#endif
