#ifndef mathexpression_h
#define mathexpression_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: mathexpression.h,v 1.12 2009-06-17 11:56:43 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
template <class T> class TypeSet;


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

A MathExpression can be queried about its variables with getNrVariables(), and
each variable's name can be queried with getVariableStr( int ).

When a calculations should be done, all variables must be set with
setVariable( int, float ). Then, the calculation can be done with getValue().

-*/

mClass MathExpression
{
public:

    static MathExpression* 	parse( const char* );
    static void			getPrefixAndShift(const char*,BufferString&,
	    					  int&);
    virtual float		getValue() const		= 0;

    virtual int			getNrVariables() const;
    virtual const char*		getVariableStr(int) const;
    virtual void		setVariable(int,float);

    //returns the number of different variables : 
    //recursive "THIS" and shifted variables are excluded
    int				getNrDiffVariables() const;

    // functions for each variable in expression
    const char*			getVarPrefixStr( int varidx ) const
				{ return varprefixes_.get( varidx ).buf(); }
    enum VarType		{ Variable, Constant, Recursive };
    VarType			getType(int varidx) const;
    int				getUsrVarIdx(int varidx) const;

    int				getPrefixIdx(const char*,bool) const;
    bool			isRecursive() const	{ return isrecursive_; }

    virtual MathExpression*	clone() const = 0;

    virtual			~MathExpression();

protected:
				MathExpression(int nrinputs);

    int				getNrInputs() const { return inputs_.size(); }
    bool			setInput( int, MathExpression* );
    void			copyInput( MathExpression* target ) const;

    void			checkVarPrefix(const char*);


    ObjectSet<TypeSet<int> >	variableobj_;
    ObjectSet<TypeSet<int> >	variablenr_;
    ObjectSet<MathExpression>	inputs_;
    BufferStringSet		varprefixes_;
    bool			isrecursive_;

};


#endif
