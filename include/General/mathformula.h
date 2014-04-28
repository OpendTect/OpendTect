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
#include "propertyref.h"
class UnitOfMeasure;


namespace Math
{

class Expression;


/*!\brief Special variables for Math Formulae/Expressions */

mExpClass(General) SpecVar
{
public:

			SpecVar( const char* varnm, const char* dispnm,
				 bool hasuns=false,
				 PropertyRef::StdType typ=PropertyRef::Other )
			    : varnm_(varnm), dispnm_(dispnm)
			    , hasunits_(hasuns), type_(typ)	{}
    bool		operator ==( const SpecVar& oth ) const
					{ return varnm_ == oth.varnm_; }

    BufferString	varnm_;
    BufferString	dispnm_;
    bool		hasunits_;
    PropertyRef::StdType type_;

};


/*!\brief Set of special variables for Math Formulae/Expressions */

mExpClass(General) SpecVarSet : public TypeSet<SpecVar>
{
public:
			SpecVarSet()		{}

    int			getIndexOf(const char* varnm) const;
    void		getNames(BufferStringSet&,bool usrdisp=true) const;
    void		add( const char* varnm, const char* dispnm,
			   bool hasuns=false, PropertyRef::StdType typ
						=PropertyRef::Other )
			{ *this += SpecVar(varnm,dispnm,hasuns,typ); }

			// convenience

    SpecVar*		find( const char* nm )
			{ int idx=getIndexOf(nm); return idx<0?0:&(*this)[idx];}
    const SpecVar*	find( const char* nm ) const
			{ return const_cast<SpecVarSet*>(this)->find( nm ); }
    bool		isPresent( const char* nm ) const
			{ return getIndexOf(nm) >= 0; }
    const BufferString&	varName( int idx ) const
			{ return (*this)[idx].varnm_; }
    const BufferString&	dispName( int idx ) const
			{ return (*this)[idx].dispnm_; }
    bool		hasUnits( int idx )
			{ return (*this)[idx].hasunits_; }
    PropertyRef::StdType propType( int idx )
			{ return (*this)[idx].type_; }

    static const SpecVarSet& getEmpty();

};




/*!\brief Math formula: (expression, inputs, units, calculation, storage).

 Before construction gather your 'special variables'. Users can insert them
 and they are not 'regular input'. See SpecVars and SpecVarSet.

 At construction or when calling setText(), the input text is parsed. Then,
 the variable definitions are known. You can use setInputUnit(idx) before
 calculation, this unit is the unit that the user wants the variable value to
 be converted to before inserting it into the mathexpression.

 When you know what is required, you can provide numbers for the different
 inputs and get a result using getValue(). The input values you provide must
 be in internal units.

 If your formula is to work on a series of input vectors, then two things
 become possible:
 1) Recursive values. This is when the user uses out[i] ot this[i].
 2) Input shifts. For example: ampl[-1] or den[3].
 For (1), you can specify start values. The maximum shift is the size of the
 TypeSet. These recursive variables will not show up in the inputs.
 For (2), you need to have extra values in the input value array
 (i.e. you need to loop over the getShifts(iinp).

 Before using getValue(), you want to use setInputDef(idx) and maybe set the input unit conversions. This makes it possible to store/retrieve in IOPar, but this is aso required if there are constants in the expression.

 The setOutputUnit() can be used to keep track of what the result
 means, or to get the result in that unit from getValue().

 Lastly, the SpecVarSet you provide usually lives forever, but should at least
 stay alive during the lifetime of the formula (it is not copied).

*/


mExpClass(General) Formula
{
public:

			Formula(bool inputsareseries=true,const char* txt=0);
			Formula(bool inputsareseries,const SpecVarSet&,
				const char* txt=0);
			Formula(const Formula&);
			~Formula();
    Formula&		operator =(const Formula&);

		// 0. Specify the formula text. Will construct a new Expression

    void		setText(const char*);

		// 1. Things known after construction or setText()

    bool		isOK() const		{ return expr_; }
    bool		isBad() const		{ return !expr_
						      && !text_.isEmpty(); }
    const char*		errMsg() const		{ return errmsg_; }

    const char*		text() const		{ return text_; }
    int			nrInputs() const	{ return inps_.size(); }
    const char*		variableName( int iinp ) const
						{ return inps_[iinp].varname_; }
    bool		isConst( int iinp ) const
						{ return inps_[iinp].isConst();}
    bool		isSpec( int iinp ) const
						{ return inps_[iinp].isSpec(); }
    int			specIdx(int) const;
    const TypeSet<int>&	getShifts( int iinp ) const
						{ return inps_[iinp].shifts_; }
    Interval<int>	shiftRange( int iinp ) const
						{ return inps_[iinp].shftRg(); }
    bool		isRecursive() const	{ return maxRecShift() > 0; }
    int			maxRecShift() const	{ return recstartvals_.size(); }

		// 2. Things to set before calculation or store

    void		setInputDef(int,const char*);
    void		setInputUnit(int,const UnitOfMeasure*);
    void		setOutputUnit( const UnitOfMeasure* uom )
						{ outputunit_ = uom; }
    TypeSet<double>&	recStartVals()		{ return recstartvals_; }
    void		clearInputDefs();

		// 3. Things you have set yourself or that were retrieved

    const UnitOfMeasure* outputUnit() const	{ return outputunit_; }
    const char*		inputDef( int iinp ) const
						{ return inps_[iinp].inpdef_; }
    const UnitOfMeasure* inputUnit( int iinp ) const
						{ return inps_[iinp].unit_; }
    double		getConstVal(int) const;
			//!< if isConst returns toDouble(inputDef(i)), else Udf

		// 4. To get an output value

    int			nrValues2Provide() const;
    void		startNewSeries() const; // resets recursive values
    float		getValue(const float*,bool internal_units=true) const;
    double		getValue(const double*,bool internal_units=true) const;

		// 5. store/retrieve to/from IOPar

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);


    const Expression*	expression() const	{ return expr_; };
    const SpecVarSet&	specVars() const	{ return *specvars_; }
    bool		inputsAreSeries() const	{ return inputsareseries_; }
    int			nrConsts() const;
    const char*		userDispText() const;

    static const char*	sKeyExpression()	{ return "Expression"; }
    static const char*	sKeyRecStartVals()	{ return "Recursion start"; }
    static const char*	sKeyFileType()		{ return "Math Formula"; }

protected:

    class InpDef
    {
    public:

	enum Type		{ Var, Const, Spec };

				InpDef( const char* nm, Type t )
				    : varname_(nm), unit_(0), type_(t)	{}
	bool			operator==( const InpDef& id ) const
				{ return varname_ == id.varname_; }

	BufferString		varname_;	// from Expression
	Type			type_;		// from Expression
	TypeSet<int>		shifts_;	// from Expression
	BufferString		inpdef_;	// filled by class user
	const UnitOfMeasure*	unit_;		// filled by class user

	Interval<int>		shftRg() const;
	bool			isConst() const	{ return type_ == Const; }
	bool			isSpec() const	{ return type_ == Spec; }

    };

    BufferString	text_;
    TypeSet<InpDef>	inps_;
    const UnitOfMeasure* outputunit_;
    TypeSet<double>	recstartvals_;
    const SpecVarSet*	specvars_;
    const bool		inputsareseries_;

    Expression*		expr_;

			// length: expr_->nrVariables()
    TypeSet<int>	inpidxs_;
    TypeSet<int>	recshifts_;
    TypeSet<int>	validxs_;

    mutable TypeSet<double> prevvals_;
    mutable BufferString errmsg_;

    int			varNameIdx(const char* varnm) const;
    void		addShift(int,int,int&,TypeSet< TypeSet<int> >&);

};


} // namespace Math


#endif
