#ifndef scaler_h
#define scaler_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		8-9-1995
 Contents:	Scaler objects
 RCS:		$Id: scaler.h,v 1.13 2004-06-16 14:54:18 bert Exp $
________________________________________________________________________

-*/

#include <gendefs.h>

#define sLinScaler	"Linear"
#define sLogScaler	"Logarithmic"
#define sExpScaler	"Exponential"
#define sAsymptScaler	"Asymptotic"

#ifdef __msvc__
# define 	mDuplicate	duplicate_
#else
# define 	mDuplicate	duplicate
#endif

/*!\brief Scaling of floating point numbers.

Scaler is an interface for scaling and scaling back numbers. Also, string I/O
is defined, aswell as a factory (Scaler::get).

*/

class Scaler
{
public:
    static Scaler*	Scaler::get(const char*);
    virtual		~Scaler()		{}
    void		put(char*) const;

    virtual bool	isEmpty() const		{ return false; }
    virtual Scaler*	duplicate() const	= 0;
    virtual const char*	type() const		= 0;

    virtual double	scale(double) const	= 0;
    virtual double	unScale(double) const	{ return mUndefValue; }
    virtual const char*	toString() const	= 0;
    virtual void	fromString(const char*)	= 0;
};


/*!\brief Linear scaling
*/

class LinScaler : public Scaler
{
#define cloneTp		mPolyRet(Scaler,LinScaler)
public:
			LinScaler( double c=0, double f=1 )
			: constant(c), factor(f)	{}
			LinScaler( double x0, double y0, double x1, double y1 );
    void		set( double x0, double y0, double x1, double y1 );
    virtual cloneTp*	duplicate() const
			{ return new LinScaler(constant,factor); }
    inline bool		isEmpty() const;

#ifdef __msvc__
    virtual LinScaler*	duplicate_() const
			{ return new LinScaler(constant,factor); }
#endif
    const char*		type() const			{ return sLinScaler; }

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);

    bool		operator==( const LinScaler& b ) const
			{
			    return mIsEqual(constant,b.constant,mDefEps) &&
				   mIsEqual(factor,b.factor,mDefEps);
			}
    
    double		constant;
    double		factor;

#undef cloneTp
};

inline bool LinScaler::isEmpty() const
{
    return constant > -1e-15 && constant < 1e-15 && mIsEqual(factor,1,mDefEps);
}


/*!\brief Logarithmic scaling, base e or ten.
*/

class LogScaler : public Scaler
{
#define cloneTp		mPolyRet(Scaler,LogScaler)
public:
			LogScaler( bool t = YES )
			: ten(t)			{}
    const char*		type() const			{ return sLogScaler; }
    virtual cloneTp*	duplicate() const
			{ return new LogScaler(ten); }
#ifdef __msvc__
    virtual LogScaler*	duplicate_() const
			{ return new LogScaler(ten); }
#endif

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);
    bool		operator==( const LogScaler& b ) const
			{ return ten==b.ten; }
    
    bool		ten;
#undef cloneTp
};


/*!\brief Exponential scaling, base e or ten.
*/

class ExpScaler : public Scaler
{
#define cloneTp		mPolyRet(Scaler,ExpScaler)
public:
			ExpScaler( bool t = YES )
			: ten(t)			{}
    const char*		type() const			{ return sExpScaler; }
    virtual cloneTp*	duplicate() const
			{ return new ExpScaler(ten); }
#ifdef __msvc__
    virtual ExpScaler*	duplicate_() const
			{ return new ExpScaler(ten); }
#endif

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);

    bool		operator==( const ExpScaler& b ) const
			{ return ten==b.ten; }
    
    bool		ten;
#undef cloneTp
};


/*!\brief Asymptotic or 'Squeeze' scaling, with a linear (main) part.

This scaler scales between -1 and 1. Between center()-width()
and center()+width() this will happen linearly. The value at width() will be
linedge(). Therefore, linedge should be set to a value near 1, like the
default 0.95.
Outside the width() boundaries, a 1 / (1 + x^2) function will make
sure the output will be between -1 and 1. Thus, this scaler acts as a
reversible squeeze function, with a non-deforming (linear), fast central part.

*/

class AsymptScaler : public Scaler
{
#define cloneTp		mPolyRet(Scaler,AsymptScaler)
public:
			AsymptScaler( double c=0, double w=1, double l=0.95 )
			: center_(c), width_(w), linedge_(l), factor(1)
						{ set(c,w,l); }
    const char*		type() const		{ return sAsymptScaler; }
    virtual cloneTp*	duplicate() const
			{ return new AsymptScaler(center_,width_,linedge_); }
#ifdef __msvc__
    virtual AsymptScaler* duplicate_() const
			{ return new AsymptScaler(center_,width_,linedge_); }
#endif

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);

    inline bool		operator==( const AsymptScaler& b ) const
			{ return mIsEqual(center_,b.center_,mDefEps)
			      && mIsEqual(width_,b.width_,mDefEps)
			      && mIsEqual(linedge_,b.linedge_,mDefEps); }

    void		set(double,double,double);
    inline double	center() const		{ return center_; }
    inline double	width() const		{ return width_; }
    inline double	linedge() const		{ return linedge_; }

protected:

    double		center_;
    double		width_;
    double		linedge_;

    double		factor;
#undef cloneTp
};


#endif
