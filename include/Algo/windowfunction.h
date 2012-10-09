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
 
 
#include "factory.h"
#include "mathfunc.h"

class IOPar;

/*!Base class for window functions. The inheriting classes will give a value
   between 0 and 1 in the interval -1 to 1. Outside that interval, the result
   is zero. */

mClass WindowFunction : public FloatMathFunction
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

#define mDeclWFGetValueFns() \
    float		getValue(float) const; \
    float		getValue( const float* x ) const { return getValue(*x); }

};


mClass BoxWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
				mDeclWFGetValueFns()

};


mClass HammingWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
				mDeclWFGetValueFns()
};


mClass HanningWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
				mDeclWFGetValueFns()
};


mClass BlackmanWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
				mDeclWFGetValueFns()
};


mClass BartlettWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
				mDeclWFGetValueFns()
};


mClass FlatTopWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
				mDeclWFGetValueFns()
};


mClass CosTaperWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

				CosTaperWindow()	{ setVariable( 0.05 ); }

    const char*			name() const;
				mDeclWFGetValueFns()

    bool			hasVariable() const	{ return true; }
    float			getVariable() const	{ return threshold_; }
    bool  			setVariable(float);
    const char*			variableName() const{return "Taper length";}

protected:

    float			threshold_;
    float			factor_;
};



mDefineFactory( WindowFunction, WinFuncs );

#endif
