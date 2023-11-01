#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "scaler.h"

namespace ZDomain { class Info; }


/*!
\brief Base class for classes derived from ValueSeries<double>, where
 the values represent a Z value from a given ZDomain::Info
*/

mExpClass(Algo) ZValueSeries : public virtual ValueSeries<double>
{
public:
				~ZValueSeries();

    bool			operator ==(const ZValueSeries&) const;
    bool			operator !=(const ZValueSeries&) const;

    virtual bool		isRegular() const	{ return false; }

    void			setScaler(const LinScaler&);

    const ZDomain::Info&	zDomainInfo() const	{ return zdomaininfo_; }
    bool			isTime() const;
    bool			isDepth() const;
    bool			inMeter() const;
    bool			inFeet() const;

protected:
				ZValueSeries(const ZDomain::Info&);

    const LinScaler*		getScaler() const	{ return scaler_; }

private:
    const ZDomain::Info&	zdomaininfo_;
    LinScaler*			scaler_ = nullptr;
};


/*!
\brief ValueSeries<double> implementation where the Z values are
 retrieved from a SamplingData object, i.e. the values are never
 stored, but always calculated when needed
*/

mExpClass(Algo) RegularZValues : public virtual SamplingValues<double>
			       , public virtual ZValueSeries
{
public:
				RegularZValues(const ZSampling&,
					       const ZDomain::Info&);
				RegularZValues(const SamplingData<float>&,
					      od_int64 sz,const ZDomain::Info&);
				RegularZValues(const SamplingData<double>&,
					      od_int64 sz,const ZDomain::Info&);
				~RegularZValues();

    bool			operator ==(const RegularZValues&) const;
    bool			operator !=(const RegularZValues&) const;

    ValueSeries<double>*	clone() const override;

    bool			isRegular() const override	{ return true; }
    double			getStep() const;
    od_int64			getIndex(double) const;

    double			value(od_int64) const override;

    static SamplingData<double> getDoubleSamplingData(
						const SamplingData<float>&);

};


/*!
\brief ValueSeries<double> implementation where the Z values are
 retrieved from a stored array. The pointer is never managed, and must
 remain in memory while this object is being used.
*/

template <class AT>
mClass(Algo) ArrayZValues : public ArrayValueSeries<double,AT>
			  , public ZValueSeries
{
public:
				ArrayZValues(AT* zvals,od_int64 sz,
					     const ZDomain::Info&);
				ArrayZValues(ArrPtrMan<AT>& zvals,od_int64 sz,
					     const ZDomain::Info&);
				ArrayZValues(TypeSet<AT>&,const ZDomain::Info&);
				~ArrayZValues();

    bool			operator ==(const ArrayZValues<AT>&) const;
    bool			operator !=(const ArrayZValues<AT>&) const;

    ValueSeries<double>*	clone() const override;
    bool			isOK() const override;

    ValueSeries<double>&	asVS();
    const ValueSeries<double>&	asVS() const;

    double			value(od_int64) const override;
    bool			writable() const override;
    void			setValue(od_int64,double) override;

    bool			canSetAll() const override;
    void			setAll(double) override;

    bool			selfSufficient() const override;
    bool			reSizeable() const override;
    bool			setSize(od_int64) override;

    double*			arr() override;
    const double*		arr() const override;

    od_int64			size() const override;
    char			bytesPerItem() const override;

};


// ArrayZValues

template <class AT> inline
ArrayZValues<AT>::ArrayZValues( AT* zvals, od_int64 sz,
				const ZDomain::Info& zinfo )
    : ArrayValueSeries<double,AT>(zvals,false,sz)
    , ZValueSeries(zinfo)
{
}


template <class AT> inline
ArrayZValues<AT>::ArrayZValues( ArrPtrMan<AT>& zvals, od_int64 sz,
				const ZDomain::Info& zinfo )
    : ArrayZValues<AT>(zvals.ptr(),sz,zinfo)
{
}


template <class AT> inline
ArrayZValues<AT>::ArrayZValues( TypeSet<AT>& zvals, const ZDomain::Info& zinfo )
    : ArrayZValues<AT>(zvals.arr(),zvals.size(),zinfo)
{
}


template <class AT> inline
ArrayZValues<AT>::~ArrayZValues()
{
}


template <class AT> inline
bool ArrayZValues<AT>::operator ==( const ArrayZValues<AT>& oth ) const
{
    if ( &oth == this )
	return true;

    return ArrayValueSeries<double,AT>::operator ==( oth ) &&
	   ZValueSeries::operator ==( oth );
}


template <class AT> inline
bool ArrayZValues<AT>::operator !=( const ArrayZValues<AT>& oth ) const
{
    return !(oth == *this);
}


template <class AT> inline
ValueSeries<double>* ArrayZValues<AT>::clone() const
{
    auto* ret = new ArrayZValues<AT>( mSelf().storArr(), size(), zDomainInfo());
    return dynamic_cast<ArrayValueSeries<double,AT>* >( ret );
}


template <class AT> inline
bool ArrayZValues<AT>::isOK() const
{
    return ArrayValueSeries<double,AT>::isOK() && size() > 0;
}


template <class AT> inline
ValueSeries<double>& ArrayZValues<AT>::asVS()
{
    auto* ret = dynamic_cast<ArrayValueSeries<double,AT>* >( this );
    return *ret;
}


template <class AT> inline
const ValueSeries<double>& ArrayZValues<AT>::asVS() const
{
    return mSelf().asVS();
}


template <class AT> inline
double ArrayZValues<AT>::value( od_int64 idx ) const
{
    double val = ArrayValueSeries<double,AT>::value( idx );
    return getScaler() ? getScaler()->scale( val ) : val;
}


template <class AT> inline
bool ArrayZValues<AT>::writable() const
{
    return ArrayValueSeries<double, AT>::writable();
}


template <class AT> inline
void ArrayZValues<AT>::setValue( od_int64 idx, double val )
{
    if ( getScaler() )
	val = getScaler()->scale( val );

   ArrayValueSeries<double,AT>::setValue( idx, val );
}


template <class AT> inline
bool ArrayZValues<AT>::canSetAll() const
{
    return ArrayValueSeries<double,AT>::canSetAll();
}


template <class AT> inline
void ArrayZValues<AT>::setAll( double val )
{
    ArrayValueSeries<double,AT>::setAll( val );
}


template <class AT> inline
bool ArrayZValues<AT>::selfSufficient() const
{
    return ArrayValueSeries<double,AT>::selfSufficient();
}


template <class AT> inline
bool ArrayZValues<AT>::reSizeable() const
{
    return ArrayValueSeries<double,AT>::reSizeable();
}


template <class AT> inline
bool ArrayZValues<AT>::setSize( od_int64 sz )
{
    return ArrayValueSeries<double,AT>::setSize( sz );
}


template <class AT> inline
double* ArrayZValues<AT>::arr()
{
    return ArrayValueSeries<double,AT>::arr();
}


template <class AT> inline
const double* ArrayZValues<AT>::arr() const
{
    return ArrayValueSeries<double,AT>::arr();
}


template <class AT> inline
od_int64 ArrayZValues<AT>::size() const
{
    return ArrayValueSeries<double,AT>::size();
}


template <class AT> inline
char ArrayZValues<AT>::bytesPerItem() const
{
    return ArrayValueSeries<double,AT>::bytesPerItem();
}
