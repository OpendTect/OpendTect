#ifndef propertyimpl_h
#define propertyimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Jan 2004
 RCS:		$Id: mathproperty.h,v 1.9 2010-09-24 13:39:22 cvsbert Exp $
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
			, val_(v)		{}

    virtual float	value() const		{ return val_; }
    virtual bool	canSet() const		{ return true; }
    virtual void	setValue( float v )	{ val_ = v; }

    float		val_;

};


/*!\brief Calculated property */

mClass MathProperty : public Property
{
public:
    			MathProperty( const PropertyRef& pr, const char* ds=0 )
			: Property(pr), expr_(0)
			{ inps_.allowNull(true); setDef(ds); }
			~MathProperty();

    const char*		def() const			{ return def_.buf(); }
    void		setDef(const char*);

    int			nrInputs() const;
    const char*		inputName(int) const;
    void		setInput(int,const Property*);
    			//!< Must be done for all inputs after each setDef()

    virtual float	value() const;
    virtual bool	canSet() const		{ return false; }
    virtual bool	dependsOn(const Property*) const;

protected:

    BufferString		def_;
    MathExpression*		expr_;
    ObjectSet<const Property>	inps_;

};


/*!\brief Property defined by other property */

mClass IndirectProperty : public Property
{
public:

    			IndirectProperty( const Property& pr, bool readonly )
			: Property(pr.ref())
			, pr_(const_cast<Property&>(pr))
    			, ro_(readonly)			{}

    virtual float	value() const
    			{ return pr_.value(); }
    virtual bool	canSet() const
    			{ return !ro_ && pr_.canSet();}
    virtual void	setValue( float v )
    			{ if ( !ro_ ) pr_.setValue( v ); }
    virtual bool        dependsOn( const Property* pr ) const
    			{ return pr_.dependsOn(pr); }

protected:

    Property&	pr_;
    bool	ro_;

};


#endif
