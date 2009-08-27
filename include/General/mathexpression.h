#ifndef mathexpression_h
#define mathexpression_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: mathexpression.h,v 1.16 2009-08-27 14:08:04 cvshelene Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
template <class T> class TypeSet;

/*!\brief Parsed Math expression.

A MathExpression can be queried about its variables with getNrVariables(), and
each variable's name can be queried with getVariableStr( int ).

When a calculations should be done, all variables must be set with
setVariable( int, float ). Then, the calculation can be done with getValue().

-*/

mClass MathExpression
{
public:

    virtual float		getValue() const		= 0;

    virtual int			nrVariables() const;
    virtual const char*		fullVariableExpression(int) const;
    virtual void		setVariableValue(int,float);

				// recursive "out" or "this" excluded
    int				nrUniqueVarNames() const
    				{ return varnms_.size(); }
    const char*			uniqueVarName( int idx ) const
				{ return varnms_.get(idx).buf(); }
    int				indexOfUnVarName( const char* nm )
				{ return varnms_.indexOf( nm ); }
    int				firstOccurVarName(const char*) const;

    enum VarType		{ Variable, Constant, Recursive };
    VarType			getType(int varidx) const;
    int				getConstIdx(int varidx) const;

    bool			isRecursive() const
    				{ return isrecursive_; }

    virtual MathExpression*	clone() const = 0;

    virtual			~MathExpression();

protected:

				MathExpression(int nrinputs);

    int				nrInputs() const { return inputs_.size(); }
    bool			setInput( int, MathExpression* );
    void			copyInput( MathExpression* target ) const;

    void			addIfOK(const char*);


    ObjectSet<TypeSet<int> >	variableobj_;
    ObjectSet<TypeSet<int> >	variablenr_;
    ObjectSet<MathExpression>	inputs_;
    BufferStringSet		varnms_;
    bool			isrecursive_;

    friend class		MathExpressionParser;

};


/*!\brief parses a string with a mathematical expression.

The expression can consist of constants, variables, operators and standard
mathematical functions.
A constant can be any number like 3, -5, 3e-5, or pi. Everything that does
not start with a digit and is not an operator is treated as a variable.
An operator can be either:

+, -, *, /, ^,  >, <, <=, >=, ==, !=, &&, ||, cond ? true stat : false stat, 
or |abs|

A mathematical function can be either:

sin(), cos(), tan(), ln(), log(), exp() or sqrt().

If the parser returns null, it couldn't parse the expression.
Then, errmsg_ should contain info.

-*/


mClass MathExpressionParser
{
public:

    				MathExpressionParser( const char* str=0 )
				    : inp_(str)		{}

    void			setInput( const char* s ) { inp_ = s; }
    MathExpression*		parse() const;

    static BufferString		varNameOf(const char* fullvarnm,int* shift=0);
    static MathExpression::VarType varTypeOf(const char*);
    static int			constIdxOf(const char*);

    const BufferString&		errMsg() const		{ return errmsg_; }

protected:

    BufferString		inp_;
    mutable BufferString	errmsg_;

    MathExpression*		parse(const char*) const;

};


#endif
