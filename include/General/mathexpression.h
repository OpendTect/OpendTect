#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstringset.h"


namespace Math
{

/*!
\brief Parsed Math expression.

  A Expression can be queried about its variables with getNrVariables(),
  and each variable's name can be queried with getVariableStr( int ).

  When a calculations should be done, all variables must be set with
  setVariable( int, double ). Then, the calculation can be done with getValue().
-*/

mExpClass(General) Expression
{
public:

    virtual double	getValue() const		= 0;

    virtual int		nrVariables() const;
    virtual const char* fullVariableExpression(int) const;
    virtual void	setVariableValue(int,double);

			// recursive "out" or "this" excluded
    int			nrUniqueVarNames() const
			{ return varnms_.size(); }
    const char*		uniqueVarName( int idx ) const
			{ return varnms_.get(idx).buf(); }
    int			indexOfUnVarName( const char* nm ) const
			{ return varnms_.indexOf( nm ); }
    int			firstOccurVarName(const char*) const;

    enum VarType	{ Variable, Constant, Recursive };
    VarType		getType(int varidx) const;
    int			getConstIdx(int varidx) const;

    bool		isRecursive() const
			{ return isrecursive_; }

    virtual Expression*	clone() const = 0;

    virtual		~Expression();

    BufferString	type() const;
    void		dump( BufferString& str ) const { doDump(str,0); }

    virtual int		nrLevels() const;
    virtual bool	isCommutative() const		    = 0;

protected:

			Expression(int nrinputs);

    int			nrInputs() const { return inputs_.size(); }
    bool		setInput( int, Expression* );
    void		copyInput( Expression* target ) const;

    void		addIfOK(const char*);

    ObjectSet<TypeSet<int> > variableobj_;
    ObjectSet<TypeSet<int> > variablenr_;
    ObjectSet<Expression> inputs_;
    BufferStringSet	varnms_;
    bool		isrecursive_ = false;

    friend class	ExpressionParser;

    void		doDump(BufferString&,int nrtabs) const;
    virtual void	dumpSpecifics(BufferString&,int nrtabs) const	{}

};


/*!
\brief Parses a string with a mathematical expression.

  The expression can consist of constants, variables, operators and standard
  mathematical functions.
  A constant can be any number like 3, -5, 3e-5, or pi. Everything that does
  not start with a digit and is not an operator is treated as a variable.
  An operator can be either:

  +, -, *, /, ^,  >, <, <=, >=, ==, !=, &&, ||, cond ? true stat : false stat,
  or |abs|

  A mathematical function can be either:

  sin(), cos(), tan(), ln(), log(), exp() or sqrt ().

  If the parser returns null, it couldn't parse the expression.
  Then, errmsg_ should contain info.
-*/

mExpClass(General) ExpressionParser
{
public:

			ExpressionParser( const char* str=0,
					  bool inputsareseries=true )
			    : inp_(str), abswarn_(false)
			    , inputsareseries_(inputsareseries)	{}

    void		setInput( const char* s ) { inp_ = s; }
    Expression*		parse() const;

    static BufferString	varNameOf(const char* fullvarnm,int* shift=0);
    static Expression::VarType varTypeOf(const char*);
    static int		constIdxOf(const char*);

    const char*		errMsg() const		{ return errmsg_; }
    bool		foundOldAbs() const	{ return abswarn_; }

protected:

    BufferString	inp_;
    const bool		inputsareseries_;
    mutable BufferString errmsg_;
    mutable bool	abswarn_;

    Expression*		parse(const char*) const;

    bool		findOuterParens(char*,int,Expression*&) const;
    bool		findOuterAbs(char*,int,Expression*&) const;
    bool		findQMarkOper(char*,int,Expression*&) const;
    bool		findAndOrOr(char*,int,Expression*&) const;
    bool		findInequality(char*,int,Expression*&) const;
    bool		findPlusAndMinus(char*,int,Expression*&) const;
    bool		findOtherOper(BufferString&,int,Expression*&) const;
    bool		findVariable(char*,int,Expression*&) const;
    bool		findMathFunction(BufferString&,int,
					 Expression*&) const;
    bool		findStatsFunction(BufferString&,int,
					  Expression*&) const;

};


/*!
\brief Expression desc to build UI.
*/

mExpClass(General) ExpressionOperatorDesc
{
public:
			ExpressionOperatorDesc( const char* s,
					const char* d, bool isop, int n )
			    : symbol_(s), desc_(d)
			    , isoperator_(isop), nrargs_(n)	{}

    const char*		symbol_; // can have spaces, e.g. "? :"
    const char*		desc_;
    bool		isoperator_; //!< if not, function
    int			nrargs_; //!< 2 for normal operators
};


/*!
\brief Group of similar expression descs.
*/

mExpClass(General) ExpressionOperatorDescGroup
{
public:

    BufferString			name_;
    ObjectSet<ExpressionOperatorDesc>	opers_;

    static const ObjectSet<const ExpressionOperatorDescGroup>& supported();

};

} // namespace Math
