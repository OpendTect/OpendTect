#ifndef propertyimpl_h
#define propertyimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Jan 2004
 RCS:		$Id: mathproperty.h,v 1.14 2010-10-14 09:58:06 cvsbert Exp $
________________________________________________________________________


-*/

#include "property.h"
#include "undefval.h"
class MathExpression;


/*!\brief Simple property */

mClass ValueProperty : public Property
{
public:

    			ValueProperty( const PropertyRef& pr,
				       float v=mUdf(float) )
			: Property(pr)
			, val_(v)			{}

    float		val_;

    mDefPropFns(ValueProperty,"Value");

};

/*!\brief Range of values.  pos_ is usually in [0,1]. */

mClass RangeProperty : public Property
{
public:

    			RangeProperty( const PropertyRef& pr )
			: Property(pr)
			, pos_(0.5)
			, rg_(mUdf(float),0)	{}

    Interval<float>	rg_;
    float		pos_;

    mDefPropFns(RangeProperty,"Range");

};


/*!\brief Calculated property */

mClass MathProperty : public Property
{
public:
    			MathProperty( const PropertyRef& pr )
			: Property(pr), expr_(0)      { inps_.allowNull(true); }
			~MathProperty();

    int			nrInputs() const;
    const char*		inputName(int) const;
    bool		haveInput( int idx ) const    { return inps_[idx]; }
    void		setInput(int,const Property*);
    			//!< Must be done for all inputs after each setDef()

    virtual void	reset();
    virtual bool	dependsOn(const Property&) const;

    mDefPropFns(MathProperty,"Math");

protected:

    BufferString		def_;
    MathExpression*		expr_;
    ObjectSet<const Property>	inps_;

};


#endif
