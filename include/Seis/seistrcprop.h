#ifndef seistrcprop_h
#define seistrcprop_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: seistrcprop.h,v 1.1 2001-02-13 17:46:50 bert Exp $
________________________________________________________________________

A trace is composed of trace info and trace data.
@$*/

#include <seistype.h>
#include <ranges.h>
class SUsegy;
class SeisTrc;


class SeisTrcPropCalc
{
public:
		SeisTrcPropCalc( const SeisTrc& t, int ic=0 )
		: trc(t), curcomp(ic)		{}

    void	setComponent( int i )		{ curcomp = i; }

    Seis::Event	find(Seis::Event::Type,Interval<float>,int occ=1) const;
    void	gettr(SUsegy&) const;
    double	corr(const SeisTrc&,const SampleGate&,bool alpick=NO) const;
    double	dist(const SeisTrc&,const SampleGate&,bool alpick=NO) const;
    float	getFreq(int isamp) const;
    float	getPhase(int isamp) const;

    const SeisTrc&	trace() const		{ return trc; }

protected:

    const SeisTrc&	trc;
    int			curcomp;

    void		getPreciseExtreme(Seis::Event& ev,int,int,
					  float,float) const;
};


class SeisTrcPropChg : public SeisTrcPropCalc
{
public:
		SeisTrcPropChg( SeisTrc& t, int ic=0 )
		: SeisTrcPropCalc(t,ic)		{}

    void	puttr(const SUsegy&);
    void	stack(const SeisTrc&,bool alongref=NO);
    void	normalize();
    void	corrNormalize();
    void	removeDC();
    void	mute(float,float taperlen);

    SeisTrc&	trace()			{ return mtrc(); }

protected:

    inline SeisTrc&	mtrc()	{ return const_cast<SeisTrc&>(trc); }

};


#endif
