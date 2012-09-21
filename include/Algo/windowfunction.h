#ifndef windowfunction_h
#define windowfunction_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		2007
 RCS:		$Id$
________________________________________________________________________

-*/
 
 
#include "algomod.h"
#include "factory.h"
#include "mathfunc.h"

class IOPar;

/*!Base class for window functions. The inheriting classes will give a value
   between 0 and 1 in the interval -1 to 1. Outside that interval, the result
   is zero. */

mClass(Algo) WindowFunction : public FloatMathFunction
{
public:
    virtual const char*	name() const			= 0;
    virtual bool	hasVariable() const		{ return false; }
    virtual float	getVariable() const		{ return mUdf(float); }
    virtual bool  	setVariable(float)		{ return true; }
    virtual const char*	variableName() const		{ return 0; }

    static const char*	sKeyVariable() 			{ return "Variable"; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

#define mDeclWFStdFns(nm) \
    static void			initClass(); \
    static const char*		sName()		{ return #nm; }\
    static WindowFunction*	create()	{ return new nm##Window; } \
    const char*			name() const	{ return #nm; } \
    float	getValue(float) const; \
    float	getValue( const float* x ) const { return getValue(*x); }

    static void		addAllStdClasses(); // done by Algo/initalgo.cc

};


#define mDeclWFSimpleClass(nm) \
mClass(Algo) nm##Window : public WindowFunction \
{ \
public: \
    mDeclWFStdFns(nm); \
};

mDeclWFSimpleClass(Box)
mDeclWFSimpleClass(Hamming)
mDeclWFSimpleClass(Hanning)
mDeclWFSimpleClass(Blackman)
mDeclWFSimpleClass(Bartlett)
mDeclWFSimpleClass(FlatTop)


mClass(Algo) CosTaperWindow : public WindowFunction
{
public:

    mDeclWFStdFns(CosTaper)

				CosTaperWindow()	{ setVariable( 0.05 ); }

    bool			hasVariable() const	{ return true; }
    float			getVariable() const	{ return threshold_; }
    bool  			setVariable(float);
    const char*			variableName() const{return "Taper length";}

protected:

    float			threshold_;
    float			factor_;
};


mDefineFactory(Algo,WindowFunction,WINFUNCS);


#endif

