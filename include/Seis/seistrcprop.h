#ifndef seistrcprop_h
#define seistrcprop_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: seistrcprop.h,v 1.5 2004-10-19 12:48:21 bert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "ranges.h"
class SUsegy;
class SeisTrc;

/*!\brief calculates properties of a trace component */

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

/*!\brief changes properties of one or all trace components.
  Component -1 (the default) changes all components.
 */

class SeisTrcPropChg : public SeisTrcPropCalc
{
public:
		SeisTrcPropChg( SeisTrc& t, int ic=-1 )
		: SeisTrcPropCalc(t,ic)		{}

    void	puttr(const SUsegy&);
    void	stack(const SeisTrc&,bool alongref=NO);
    void	scale(float fac,float shft=0);
    void	normalize(bool aroundzero);
    void	corrNormalize();
    void	removeDC();
    void	mute(float pos,float taperlen)		{topMute(pos,taperlen);}
    void	topMute(float,float);
    void	tailMute(float,float);

    SeisTrc&	trace()					{ return mtrc(); }

protected:

    inline SeisTrc&	mtrc()	{ return const_cast<SeisTrc&>(trc); }

};


#endif
