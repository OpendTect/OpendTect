#ifndef mathformula_h
#define mathformula_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "typeset.h"
class UnitOfMeasure;


namespace Math
{

class Expression;


/*!\brief Math formula: (expression, inputs, units, calculation) */

mExpClass(General) Formula
{
public:

			Formula(const char* txt=0);
			Formula(const Formula&);
			~Formula();
    Formula&		operator =(const Formula&);

    bool		isOK() const		{ return expr_; }
    const char*		errMsg() const		{ return errmsg_; }

    const char*		text() const		{ return text_; }
    int			nrInputs() const	{ return inps_.size(); }
    bool		isConst( int iinp )	{ return inps_[iinp].isconst_; }
    int			indexOf(const char* varnm) const;
    const UnitOfMeasure* outputUnit() const	{ return outputunit_; }
    const char*		inputName( int iinp ) const
						{ return inps_[iinp].name_; }
    const UnitOfMeasure* inputUnit( int iinp ) const
						{ return inps_[iinp].unit_; }

    TypeSet<double>&	recursiveStartVals()	{ return recstartvals_; }
    int			maxRecursiveShift() const { return maxshift_; }
    bool		isRecursive() const	{ return maxshift_ > 0; }

    void		setText(const char*);
    void		setInputName(int,const char*);
    void		setInputUnit(int,const UnitOfMeasure*);
    void		setOutputUnit( const UnitOfMeasure* uom )
						{ outputunit_ = uom; }

    void		startNewSeries() const; // resets recursive values
    float		getValue(const float*,bool internal_units=true) const;
    double		getValue(const double*,bool internal_units=true) const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    const Expression*	expression() const	{ return expr_; };

    static const char*	sKeyExpression()	{ return "Expression"; }
    static const char*	sKeyRecStartVals()	{ return "Recursion start"; }

protected:

    class InpDef
    {
    public:

				InpDef( const char* nm, bool isc )
				    : name_(nm), unit_(0), isconst_(isc) {}
	bool			operator==( const InpDef& id ) const
				{ return name_ == id.name_; }

	BufferString		name_;
	const UnitOfMeasure*	unit_;
	bool			isconst_;

    };

    BufferString	text_;
    TypeSet<InpDef>	inps_;
    const UnitOfMeasure* outputunit_;
    TypeSet<double>	recstartvals_;

    Expression*		expr_;

    TypeSet<int>	inpidxs_;
    TypeSet<int>	varshifts_;
    int			maxshift_;
    mutable TypeSet<double> prevvals_;
    mutable BufferString errmsg_;

};


} // namespace Math


#endif
