#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		8-9-1995
 Contents:	Scaler objects
________________________________________________________________________

-*/

#include "algomod.h"
#include "gendefs.h"
#include "undefval.h"

#define sLinScaler	"Linear"
#define sLogScaler	"Logarithmic"
#define sExpScaler	"Exponential"
#define sAsymptScaler	"Asymptotic"


/*!\brief Scaling of floating point numbers.

  Scaler is an interface for scaling and scaling back numbers. Also, string I/O
  is defined, as well as a factory (Scaler::get).
*/

mExpClass(Algo) Scaler
{
public:

    virtual		~Scaler()		{}

    static Scaler*	get(const char*);
    static Scaler*	get(const IOPar&);
    static void		putToPar(IOPar&,const Scaler*);
    void		put(char*,int sz) const;
    void		put(IOPar&) const;

    virtual bool	isEmpty() const		{ return false; }
    virtual Scaler*	clone() const		= 0;
    virtual Scaler*	inverse() const		{ return 0; }
    virtual const char*	type() const		= 0;

    virtual double	scale(double) const	= 0;
    virtual double	unScale(double) const	{ return mUdf(double); }
    virtual const char*	toString() const	= 0;
    virtual void	fromString(const char*)	= 0;

};


/*!
\brief Linear scaling
*/

mExpClass(Algo) LinScaler : public Scaler
{
public:
			LinScaler( double c=0, double f=1. )
			: constant_(c), factor_(f)	{}
			LinScaler( double x0, double y0, double x1, double y1 );
    void		set( double x0, double y0, double x1, double y1 );
    virtual LinScaler*	clone() const
			{ return new LinScaler(constant_,factor_); }
    virtual LinScaler*	inverse() const;
    inline bool		isEmpty() const;

    const char*		type() const			{ return sLinScaler; }

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);

    bool		operator==( const LinScaler& oth ) const
			{
			    return constant_ == oth.constant_
				&& factor_ == oth.factor_;
			}

    double		constant_;
    double		factor_;
};

inline bool LinScaler::isEmpty() const
{
    return !constant_ && factor_ == 1.;
}


/*!\brief Logarithmic scaling, base e or ten.  */

mExpClass(Algo) LogScaler : public Scaler
{
public:
			LogScaler( bool powerof10=true )
			: ten_(powerof10)		{}
    const char*		type() const			{ return sLogScaler; }
    virtual LogScaler*	clone() const
			{ return new LogScaler(ten_); }
    virtual Scaler*	inverse() const;

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);
    bool		operator==( const LogScaler& oth ) const
			{ return ten_==oth.ten_; }

    bool		ten_;
};


/*!
\brief Exponential scaling, base e or ten.
*/

mExpClass(Algo) ExpScaler : public Scaler
{
public:
			ExpScaler( bool powerof10=true )
			: ten_(powerof10)		{}
    const char*		type() const			{ return sExpScaler; }
    virtual ExpScaler*	clone() const
			{ return new ExpScaler(ten_); }
    virtual Scaler*	inverse() const;

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);

    bool		operator==( const ExpScaler& oth ) const
			{ return ten_==oth.ten_; }

    bool		ten_;
};


/*!
\brief Asymptotic or 'Squeeze' scaling, with a linear (main) part.

  This scaler scales between -1 and 1. Between center()-width()
  and center()+width() this will happen linearly. The value at width() will be
  linedge(). Therefore, linedge should be set to a value near 1, like the
  default 0.95.
  Outside the width() boundaries, a 1 / (1 + x^2) function will make
  sure the output will be between -1 and 1. Thus, this scaler acts as a
  reversible squeeze function, with a non-deforming (linear), fast central part.
*/

mExpClass(Algo) AsymptScaler : public Scaler
{
public:
			AsymptScaler( double c=0, double w=1, double l=0.95 )
			: center_(c), width_(w), linedge_(l), factor_(1)
						{ set(c,w,l); }
    const char*		type() const		{ return sAsymptScaler; }
    virtual AsymptScaler* clone() const
			{ return new AsymptScaler(center_,width_,linedge_); }

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);

    inline bool		operator==( const AsymptScaler& oth ) const
			{ return center_ == oth.center_
			      && width_ == oth.width_
			      && linedge_ == oth.linedge_; }

    void		set(double,double,double);
    inline double	center() const		{ return center_; }
    inline double	width() const		{ return width_; }
    inline double	linedge() const		{ return linedge_; }

protected:

    double		center_;
    double		width_;
    double		linedge_;

    double		factor_;
};
