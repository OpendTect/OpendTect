#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "valseriesimpl.h"

#define sLinScaler	"Linear"
#define sLogScaler	"Logarithmic"
#define sExpScaler	"Exponential"
#define sAsymptScaler	"Asymptotic"


/*!
\brief Scaling of floating point numbers.

  Scaler is an interface for scaling and scaling back numbers. Also, string I/O
  is defined, as well as a factory (Scaler::get).
*/

mExpClass(Algo) Scaler
{
public:
    static Scaler*	get(const char*);
    virtual		~Scaler()		{}
    void		put(char*,int sz) const;

    virtual bool	isEmpty() const		{ return false; }
    virtual Scaler*	clone() const		= 0;
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
			LinScaler( double c=0, double f=1 )
			: constant(c), factor(f)	{}
			LinScaler( double x0, double y0, double x1, double y1 );
    void		set( double x0, double y0, double x1, double y1 );
    LinScaler*		clone() const override
			{ return new LinScaler(constant,factor); }
    inline bool		isEmpty() const override;

    const char*		type() const override		{ return sLinScaler; }

    double		scale(double) const override;
    double		unScale(double) const override;
    const char*		toString() const override;
    void		fromString(const char*) override;

    bool		operator==( const LinScaler& b ) const
			{
			    return mIsEqual(constant,b.constant,mDefEps) &&
				   mIsEqual(factor,b.factor,mDefEps);
			}

    double		constant;
    double		factor;
};

inline bool LinScaler::isEmpty() const
{
    return constant > -1e-15 && constant < 1e-15 && mIsEqual(factor,1,mDefEps);
}


/*!
\brief Logarithmic scaling, base e or ten.
*/

mExpClass(Algo) LogScaler : public Scaler
{
public:
			LogScaler( bool powerof10=true )
			: ten_(powerof10)		{}
    const char*		type() const override		{ return sLogScaler; }
    LogScaler*		clone() const override
			{ return new LogScaler(ten_); }

    double		scale(double) const override;
    double		unScale(double) const override;
    const char*		toString() const override;
    void		fromString(const char*) override;
    bool		operator==( const LogScaler& b ) const
			{ return ten_==b.ten_; }

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
    const char*		type() const override	{ return sExpScaler; }
    ExpScaler*		clone() const override
			{ return new ExpScaler(ten_); }

    double		scale(double) const override;
    double		unScale(double) const override;
    const char*		toString() const override;
    void		fromString(const char*) override;

    bool		operator==( const ExpScaler& b ) const
			{ return ten_==b.ten_; }

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
			: center_(c), width_(w), linedge_(l), factor(1)
						{ set(c,w,l); }
    const char*		type() const override	{ return sAsymptScaler; }
    AsymptScaler*	clone() const override
			{ return new AsymptScaler(center_,width_,linedge_); }

    double		scale(double) const override;
    double		unScale(double) const override;
    const char*		toString() const override;
    void		fromString(const char*) override;

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
};


/*!
\brief ValueSeries implementation where the values are
 retrieved from another ValueSeries. The input object is never managed, and must
 remain in memory while this object is being used.
 Scaling is applied when reading from the source,
 and unapplied when writing to it.
*/

template <class RT, class AT>
mClass(Algo) ScaledValueSeries : public ValueSeries<RT>
{
public:
				ScaledValueSeries(const Scaler*,
						  ValueSeries<AT>&);
				~ScaledValueSeries();

    ValueSeries<RT>*		clone() const override;
    bool			isOK() const override
				{ return src_.isOK(); }
    bool			writable() const override
				{ return src_.writable(); }

    bool			canSetAll() const override
				{ return src_.canSetAll(); }
    void			setAll(RT) override;

    RT				value(od_int64) const override;
    void			setValue(od_int64,RT) override;

    od_int64			size() const override
				{ return src_.size(); }

    static ValueSeries<RT>*	getFrom(ValueSeries<AT>&,
					const Scaler* =nullptr);

private:

    ValueSeries<AT>&		src_;
    const Scaler*		scaler_ = nullptr;

};



template <class RT, class AT> inline
ScaledValueSeries<RT,AT>::ScaledValueSeries( const Scaler* scaler,
					     ValueSeries<AT>& src )
    : src_(src)
{
    if ( scaler && !scaler->isEmpty() )
	scaler_ = scaler->clone();
}


template <class RT, class AT> inline
ScaledValueSeries<RT,AT>::~ScaledValueSeries()
{
    delete scaler_;
}


template <class RT, class AT> inline
ValueSeries<RT>* ScaledValueSeries<RT,AT>::clone() const
{
    return new ScaledValueSeries<RT,AT>( scaler_, src_ );
}


template <class RT, class AT> inline
void ScaledValueSeries<RT,AT>::setAll( RT val )
{
    const od_int64 sz = size();
    for ( od_int64 idx=0; idx<sz; idx++ )
	setValue( idx, val );
}


template <class RT, class AT> inline
RT ScaledValueSeries<RT,AT>::value( od_int64 idx ) const
{
    const RT val = RT (src_.value(idx) );
    return scaler_ ? scaler_->scale( val ) : val;
}


template <class RT, class AT> inline
void ScaledValueSeries<RT,AT>::setValue( od_int64 idx, RT val )
{
    if ( scaler_ )
	val = scaler_->unScale( val );

    src_.setValue( idx, AT (val) );
}


template <class RT, class AT> inline
ValueSeries<RT>* ScaledValueSeries<RT,AT>::getFrom( ValueSeries<AT>& src,
						    const Scaler* scaler )
{
    if ( src.arr() && !scaler )
	return new ArrayValueSeries<RT,AT>( src.arr(), false, src.size() );
    return new ScaledValueSeries<RT,AT>( scaler, src );
}
