#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2010
________________________________________________________________________

*/

#include "seismod.h"

#include "datachar.h"
#include "valseriesinterpol.h"

class Scaler;
class SeisTrc;
class SeisTrcBuf;
class TraceData;


namespace Seis
{

class ObjectSummary;

/*!\brief Buffer to a set of entire traces ( header + component data )
	  Can contain traces for several positions. */


mExpClass(Seis) RawTrcsSequence
{ mODTextTranslationClass(Seis::RawTrcsSequence);
public:
			RawTrcsSequence(const ObjectSummary&,int nrpos);
			RawTrcsSequence(const RawTrcsSequence&);
			~RawTrcsSequence();

    RawTrcsSequence&	operator =(const RawTrcsSequence&);

    bool		isOK() const;
    bool		isPS() const;
    const DataCharacteristics	getDataChar() const;

    const ZSampling&	getZRange() const;
    int			nrPositions() const;
    float		get(int idx,int pos,int comp) const;
    float		getValue(float,int pos,int comp) const;

    void		set(int idx,float val,int pos,int comp);
    void		setPositions(const TypeSet<TrcKey>&); //Becomes mine
    void		setTrcScaler(int pos,const Scaler*);
    const Scaler*	getTrcScaler(int pos) const;
    void		copyFrom(const SeisTrc&,int* ipos=0);
    void		copyFrom(const SeisTrcBuf&)		{}

    //No checks
    const unsigned char* getData(int ipos,int icomp,int is=0) const;
    unsigned char*	getData(int ipos,int icomp,int is=0);
    const TrcKey&	getPosition(int ipos) const;

private:

    const ValueSeriesInterpolator<float>&	interpolator() const;

    ObjectSet<TraceData>	data_;
    ObjectSet<Scaler>		trcscalers_;
    const ObjectSummary&	info_;
    const TypeSet<TrcKey>*	tks_;
    const int			nrpos_;

    mutable PtrMan<ValueSeriesInterpolator<float> >	intpol_;
    friend class ArrayFiller;
    friend class RawTrcsSequenceValueSeries;

public:

		// Special users only

    TraceData&		getTraceData( int pos ) { return *(data_[pos]); }

};


/*!> Seismic traces conforming the ValueSeries<float> interface.

  One of the components of a RawTrcsSequence can be selected to form
  a valueSeries

*/


mExpClass(Seis) RawTrcsSequenceValueSeries : public ValueSeries<float>
{
public:
			RawTrcsSequenceValueSeries(const RawTrcsSequence&,
						   int pos, int comp);
			~RawTrcsSequenceValueSeries();

    ValueSeries<float>* clone() const override;

    inline void		setPosition( int pos )		{ ipos_ = pos; }
    inline void		setComponent( int idx )		{ icomp_ = idx; }
    void		setValue(od_int64,float) override;
    float*		arr() override;

    float		value(od_int64) const override;
    bool		writable() const override		{ return true; }
    const float*	arr() const override;
    od_int64		size() const override;

private:

    RawTrcsSequence&	seq_;
    int			ipos_;
    int			icomp_;
};

} // namespace Seis
