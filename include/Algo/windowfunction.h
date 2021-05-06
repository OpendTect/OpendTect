#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		2007
________________________________________________________________________

-*/


#include "algomod.h"
#include "factory.h"
#include "mathfunc.h"


/*!
\brief Base class for window functions. The inheriting classes will give a
value between 0 and 1 in the interval -1 to 1. Outside that interval, the
result is zero.
*/

mExpClass(Algo) WindowFunction : public FloatMathFunction
{
public:
    virtual const char*	name() const			= 0;
    virtual bool	hasVariable() const		{ return false; }
    virtual float	getVariable() const		{ return mUdf(float); }
    virtual bool	setVariable(float)		{ return true; }
    virtual const char*	variableName() const		{ return 0; }
    virtual bool	isAcceptableVariable(float) const	{ return true; }

    static const char*	sKeyVariable()			{ return "Variable"; }
    static const char*	sKeyTaperVal()			{ return "taperval"; }
    static bool		hasVariable(const BufferString& wintyp);

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
mExpClass(Algo) nm##Window : public WindowFunction \
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


#define mDeclWFClassWithVariable(clss) \
				clss##Window(); \
    bool			hasVariable() const	{ return true; } \
    bool			setVariable(float); \
    bool			isAcceptableVariable(float) const; \

/*!
\brief Tapered Cosine Window Function.
*/

mExpClass(Algo) CosTaperWindow : public WindowFunction
{
public:

    mDeclWFStdFns(CosTaper)
    mDeclWFClassWithVariable(CosTaper)

    float			getVariable() const	{ return threshold_; }
    const char*			variableName() const
				{ return "Taper length";}

    static bool			isLegacyTaper(const BufferString&);
    static float		getLegacyTaperVariable(const BufferString&);

protected:

    float			threshold_;
    float			factor_;
};


/*!
\brief Kaiser Window Function
*/

mExpClass(Algo) KaiserWindow : public WindowFunction
{
public:

    mDeclWFStdFns(Kaiser)
    mDeclWFClassWithVariable(Kaiser)

				KaiserWindow(double twidth,int nrsamples);
    bool			set(double width,int nrsamples);
				//Alternate way of setting alpha

    float			getVariable() const	{ return (float)alpha_;}
    const char*			variableName() const	{ return "alpha"; }

				// Variable must be set first
    double			getWidth() const	{ return width_; }
    double			getError() const;
    int				getLength() const	{ return ns_; }
    double			getWidth(int nrsamples) const;
    double			getError(int nrsamples) const;

protected:

    double			alpha_;
    double			denom_;
    double			width_;
    int				ns_;
};


mDefineFactory(Algo,WindowFunction,WINFUNCS);


