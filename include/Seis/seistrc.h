#ifndef seistrc_h
#define seistrc_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: seistrc.h,v 1.4 2000-08-07 20:12:56 bert Exp $
________________________________________________________________________

A trace is composed of trace info and trace data.
@$*/

#include <datatrc.h>
#include <databuf.h>
#include <seisinfo.h>
#include <idobj.h>


class SeisTrc : public DataTrace
	      , public IDObject
{

    friend class	Wavelet;
    friend class	SeisTrcTranslator;

public:

    virtual SeisTrc*	getNew() const		= 0;
    SeisTrc*		clone() const
			{ SeisTrc* t = getNew(); t->copyAll(*this); return t; }
    virtual bool	copyData(const SeisTrc&);
    void		copyAll( const SeisTrc& trc )
			{ copyData(trc); info_ = trc.info_; }

    struct Event	{
			    Event( float v=mUndefValue, float p=mUndefValue )
			    : val(v), pos(p) {}
			    float val, pos;
			};
    enum EvType		{ NoEvent, Extr, Max, Min, ZC, ZCNegPos, ZCPosNeg,
			  GateMax, GateMin };
			DeclareEnumUtils(EvType);

    virtual		~SeisTrc();

			// Specification
    virtual bool	set(int,float)		= 0;
    virtual float	operator[](int) const	= 0;
    virtual bool	isSUCompat() const	{ return NO; }

			// Implementation
    int			getIndex( double val ) const
			{ return info_.nearestSample( val ); }
    double		getX( int idx ) const
			{ return info_.starttime + idx * 1e-6 * info_.dt; }
    float		getValue(double) const;
    inline int		size() const	{ return data_.size(); };
    double		start() const	{ return info_.starttime; }
    double		stop() const	{ return info_.sampleTime(size()-1); }
    double		step() const	{ return info_.dt * 1.e-6; }

    SeisTrcInfo&	info()		{ return info_; }
    const SeisTrcInfo&	info() const	{ return info_; }
    SamplingData	samplingData() const
			{ return SamplingData(info_.starttime,info_.dt*1e-6 ); }
    virtual bool	isNull() const;
    bool		clear()		{ clearData(); return true; }
    void		clearData()	{ data_.clear(); }
    inline bool		dataPresent( float t ) const
			{ return info_.dataPresent(t,size()); }
    void		gettr(SUsegy&) const;
    void		puttr(SUsegy&);

    bool		reSize(int);
    int			bytesPerSample() const	{return data_.bytesPerSample();}
    virtual void	getPacketInfo(SeisPacketInfo&) const;
    SampleGate		sampleGate(const TimeGate&,int check=NO) const;

    virtual void	stack(const SeisTrc&,int alongref=NO);
    void		normalize();
    void		corrNormalize();
    void		removeDC();
    void		mute(float,float taperlen);

    Event		find(EvType,TimeGate,int occ=1) const;
    Interval<float>	getRange() const;
    float		getFreq(int) const;

    const float*	calcImag(const SampleGate&) const;
    const SampleGate&	imagWin() const		{ return imagwin; }
				// Optimize extraction:
				// call calcPhase 'manually' for entire range
    float		getPhase(int) const;

    virtual int		nrValues() const		{ return 1; }
    virtual void	setNrValues(int)		{}
    virtual bool	set( int idx, float v, int )	{ return set(idx,v); }
    virtual float	get( int idx, int ) const	{ return (*this)[idx]; }

			// Use on your own responsibility. Will actually
			// be char*, short*, float* but don't count on it!
    unsigned char*	rawData()			{ return data_.data; }
    const unsigned char* rawData() const		{ return data_.data; }
    float*		imagData()			{ return imag; }
    const float*	imagData() const		{ return imag; }

protected:

			SeisTrc(int byts,int ns=0);

    DataBuffer		data_;
    SeisTrcInfo		info_;

    SampleGate		imagwin;
    float*		imag;

    void		getPreciseExtreme(Event&,int,int,float,float) const;
    virtual XFunctionIter* mkIter(bool,bool) const;

};

double corr(const SeisTrc&,const SeisTrc&,const SampleGate&,bool alpick=NO);
double dist(const SeisTrc&,const SeisTrc&,const SampleGate&,bool alpick=NO);


class SeisTrcCreater
{
public:

    virtual SeisTrcCreater*	clone() const		= 0;
    virtual SeisTrc*		get() const		= 0;

};


inline int clippedVal( float val, int lim )
{
    return val > (float)lim ? lim : (val < -((float)lim) ? -lim : mNINT(val));
}


#endif
