#ifndef windowfunction_h
#define windowfunction_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer
 Date:		2007
 RCS:		$Id: windowfunction.h,v 1.5 2007-12-02 09:23:54 cvsnanne Exp $
________________________________________________________________________

-*/
 
 
#include "factory.h"
#include "mathfunc.h"

class IOPar;

/*!Base class for window functions. The inheriting classes will give a value
   between 0 and 1 in the interval -1 to 1. Outside that interval, the result
   is zero. */

class WindowFunction : public FloatMathFunction
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
};


class BoxWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
    float			getValue(float) const;
};


class HammingWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
    float			getValue(float) const;
};


class HanningWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
    float			getValue(float) const;
};


class BlackmanWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
    float			getValue(float) const;
};


class BartlettWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

    const char*			name() const;
    float			getValue(float) const;
};


class CosTaperWindow : public WindowFunction
{
public:
    static void			initClass();
    static const char*		sName();
    static WindowFunction*	create();

				CosTaperWindow()	{ setVariable( 0.05 ); }

    const char*			name() const;
    float			getValue(float) const;

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
