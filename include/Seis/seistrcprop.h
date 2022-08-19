#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "seistype.h"
#include "valseriesevent.h"
class SeisTrc;
class Scaler;
#define mFlValSerEv ValueSeriesEvent<float,float>


/*!\brief calculates properties of a trace component */

mExpClass(Seis) SeisTrcPropCalc
{
public:
		SeisTrcPropCalc( const SeisTrc& t, int ic=0 )
		: trc(t), curcomp(ic)		{}

    void	setComponent( int i )		{ curcomp = i; }

    ValueSeriesEvent<float,float>
		find(VSEvent::Type,Interval<float>,int occ=1) const;
    double	corr(const SeisTrc&,const SampleGate&,bool alpick=false) const;
    double	dist(const SeisTrc&,const SampleGate&,bool alpick=false) const;
    float	getFreq(float z) const;
    float	getFreq(int isamp) const;
    float	getPhase(float zpos,bool indegrees=false) const;
    float	getPhase(int isamp,bool indegrees=false) const;

    const SeisTrc&	trace() const		{ return trc; }

protected:

    const SeisTrc&	trc;
    int			curcomp;

};


/*!\brief changes properties of one or all trace components.
  Component -1 (the default) changes all components.
 */

mExpClass(Seis) SeisTrcPropChg : public SeisTrcPropCalc
{
public:
		SeisTrcPropChg( SeisTrc& t, int ic=-1 )
		: SeisTrcPropCalc(t,ic)		{}

    void	stack(const SeisTrc&,bool alongref=false,float wght=1);
    void	scale(float fac,float shft=0);
    void	scale(const Scaler&);
    void	normalize(bool aroundzero);
    void	corrNormalize();
    void	removeAVG();
    void	mute(float pos,float taperlen)		{topMute(pos,taperlen);}
    void	topMute(float,float);
    void	tailMute(float,float);

    SeisTrc&	trace()					{ return mtrc(); }

protected:

    inline SeisTrc&	mtrc()	{ return const_cast<SeisTrc&>(trc); }

};
