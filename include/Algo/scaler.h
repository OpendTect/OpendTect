#ifndef scaler_h
#define scaler_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		8-9-1995
 Contents:	Scaler objects
 RCS:		$Id: scaler.h,v 1.1.1.2 1999-09-16 09:19:12 arend Exp $
________________________________________________________________________

@$*/

#include <gendefs.h>

#define sLinScaler	"Linear"
#define sLogScaler	"Logarithmic"
#define sExpScaler	"Exponential"

class Scaler
{
public:
    static Scaler*	Scaler::get(const char*);
    virtual		~Scaler()		{}
    void		put(char*) const;

    virtual Scaler*	duplicate() const	= 0;
    virtual const char*	type() const		= 0;

    virtual double	scale(double) const	= 0;
    virtual double	unScale(double) const	{ return mUndefValue; }
    virtual const char*	toString() const	= 0;
    virtual void	fromString(const char*)	= 0;
};


class LinScaler : public Scaler
{
public:
			LinScaler( double c=0, double f=1 )
			: constant(c), factor(f)	{}
    virtual LinScaler*	duplicate() const
			{ return new LinScaler(constant,factor); }
    const char*		type() const			{ return sLinScaler; }

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);
    
    double		constant;
    double		factor;
};


class LogScaler : public Scaler
{
public:
			LogScaler( bool t = YES )
			: ten(t)			{}
    const char*		type() const			{ return sLogScaler; }
    virtual LogScaler*	duplicate() const
			{ return new LogScaler(ten); }

    double		scale(double) const;
    const char*		toString() const;
    void		fromString(const char*);
    
    bool		ten;
};


class ExpScaler : public Scaler
{
public:
			ExpScaler( bool t = YES )
			: ten(t)			{}
    const char*		type() const			{ return sExpScaler; }
    virtual ExpScaler*	duplicate() const
			{ return new ExpScaler(ten); }

    double		scale(double) const;
    const char*		toString() const;
    void		fromString(const char*);
    
    bool		ten;
};


#endif
