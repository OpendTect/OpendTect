#ifndef scaler_h
#define scaler_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		8-9-1995
 Contents:	Scaler objects
 RCS:		$Id: scaler.h,v 1.3 2000-12-14 16:23:53 bert Exp $
________________________________________________________________________

@$*/

#include <gendefs.h>

#define sLinScaler	"Linear"
#define sLogScaler	"Logarithmic"
#define sExpScaler	"Exponential"
#define sAsymptScaler	"Asymptotic"

/*!\brief scaling floating points.
*/

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


/*!\brief linear scaling
*/

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

    bool		operator==( const LinScaler& b ) const
			{
			    return mIS_ZERO(constant-b.constant) &&
				   mIS_ZERO(factor-b.factor);
			}
    
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
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);
    bool		operator==( const LogScaler& b ) const
			{ return ten==b.ten; }
    
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
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);

    bool		operator==( const ExpScaler& b ) const
			{ return ten==b.ten; }
    
    bool		ten;
};


class AsymptScaler : public Scaler
{
public:
			AsymptScaler( double c=0, double w=1, double l=0.95 )
			: center_(c), width_(w), linedge_(l), factor(1)
						{ set(c,w,l); }
    const char*		type() const		{ return sAsymptScaler; }
    virtual AsymptScaler* duplicate() const
			{ return new AsymptScaler(center_,width_,linedge_); }

    double		scale(double) const;
    double		unScale(double) const;
    const char*		toString() const;
    void		fromString(const char*);

    bool		operator==( const AsymptScaler& b ) const
			{ return mIS_ZERO(center_-b.center_)
			      && mIS_ZERO(width_-b.width_)
			      && mIS_ZERO(linedge_-b.linedge_); }

    void		set(double,double,double);
    double		center() const		{ return center_; }
    double		width() const		{ return width_; }
    double		linedge() const		{ return linedge_; }

protected:

    double		center_;
    double		width_;
    double		linedge_;

    double		factor;
};


#endif
