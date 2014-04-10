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



/*!\brief Math formula: (expression, inputs, units, calculation).

 At construction or when calling setText(), the input text is parsed. Then,
 the variable defintions are known. You can use setInputUnit(idx) before
 calculation, this unit is the unit that the user wants the variable value to
 be converted to before inserting it into the mathexpression.

 When you know what is required, you can provide numbers for teh different
 inputs and get a result using getValue().

 Before store/retrieve in IOPar, or just to keep track of what provides what,
 you want to use setInputDef(idx). Those are names of attributes, properties
 - the stuff that is available as input.

 Similarly, the setOutputUnit() can be used to keep track of what the result
 means, or to get the result in that unit from getValue().

*/


mExpClass(General) Formula
{
public:

			Formula(const char* txt=0);
			Formula(const Formula&);
			~Formula();
    Formula&		operator =(const Formula&);

    bool		isOK() const		{ return expr_; }
    const char*		errMsg() const		{ return errmsg_; }

		// 1. Things know after construction or set()

    const char*		text() const		{ return text_; }
    int			nrInputs() const	{ return inps_.size(); }
    const char*		variableName( int iinp ) const
						{ return inps_[iinp].varname_; }
    bool		isConst( int iinp )	{ return inps_[iinp].isconst_; }
    bool		isRecursive() const	{ return maxshift_ > 0; }
    int			maxRecursiveShift() const { return maxshift_; }

		// 2. Things to set before calculation, or store
    void		setText(const char*);
    void		setInputDef(int,const char*);
    void		setInputUnit(int,const UnitOfMeasure*);
    void		setOutputUnit( const UnitOfMeasure* uom )
						{ outputunit_ = uom; }
    TypeSet<double>&	recursiveStartVals()	{ return recstartvals_; }

		// 3. Things you have set yourself or that were retrieved

    const UnitOfMeasure* outputUnit() const	{ return outputunit_; }
    const char*		inputDef( int iinp ) const
						{ return inps_[iinp].inpdef_; }
    const UnitOfMeasure* inputUnit( int iinp ) const
						{ return inps_[iinp].unit_; }

		// 4. To get an output value

    void		startNewSeries() const; // resets recursive values
    float		getValue(const float*,bool internal_units=true) const;
    double		getValue(const double*,bool internal_units=true) const;

		// 5. store/retrieve to/from IOPar

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
				    : varname_(nm), unit_(0), isconst_(isc) {}
	bool			operator==( const InpDef& id ) const
				{ return varname_ == id.varname_; }

	BufferString		varname_; // from Expression
	bool			isconst_; // from Expression
	BufferString		inpdef_; // filled by class user
	const UnitOfMeasure*	unit_;    // filled by class user

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

    int			indexOf(const char* varnm) const;

};


} // namespace Math


#endif
