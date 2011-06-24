#ifndef propertyimpl_h
#define propertyimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Jan 2004
 RCS:		$Id: mathproperty.h,v 1.19 2011-06-24 13:37:39 cvsbert Exp $
________________________________________________________________________


-*/

#include "property.h"
#include "undefval.h"
class MathExpression;


/*!\brief Simple property */

mClass ValueProperty : public Property
{
public:

    			ValueProperty( const PropertyRef& pr )
			: Property(pr)
			, val_(pr.disp_.range_.center())	{}
    			ValueProperty( const PropertyRef& pr, float v )
			: Property(pr)
			, val_(v)				{}

    float		val_;

    mDefPropertyFns(ValueProperty,"Value");

};

/*!\brief Range of values.  pos_ is usually in [0,1]. */

mClass RangeProperty : public Property
{
public:

    			RangeProperty( const PropertyRef& pr )
			: Property(pr)
			, rg_(pr.disp_.range_)		{}
    			RangeProperty( const PropertyRef& pr,
				       Interval<float> rg )
			: Property(pr)
			, rg_(rg)			{}

    Interval<float>	rg_;

    mDefPropertyFns(RangeProperty,"Range");

};


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
    bool		haveInput( int idx ) const    { return inps_[idx]; }
    void		setInput(int,const Property*);
    			//!< Must be done for all inputs after each setDef()

    virtual bool	init(const PropertySet&) const;
    virtual const char*	errMsg() const		{ return errmsg_.buf(); }
    virtual bool	dependsOn(const Property&) const;

    mDefPropertyFns(MathProperty,"Math");

    static void		ensureGoodVariableName(char*);

protected:

    BufferString		def_;
    MathExpression*		expr_;
    mutable ObjectSet<const Property> inps_;
    mutable BufferString	errmsg_;

    const Property*		findInput(const PropertySet&,const char*,
	    				  bool) const;

};


#endif
