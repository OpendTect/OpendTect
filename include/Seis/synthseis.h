#ifndef synthseis_H
#define synthseis_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: synthseis.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

@$*/
 
#include <gendefs.h>
class Wavelet;
class IFRUnitList;
class IFRealUnit;
class SeisTrc;
class Quantity;
class Property;

class SynthSeisPars
{
public:
    enum ResamplePolicy	{ AAFilter, MostFreq, Direct };
			SynthSeisPars( int ns )
			: nrsamples(ns) , startsmpls(0)
			, denqty(0), aiqty(0), velqty(0), outqty(0)
			, outpty(0)
			, issonic(YES)
			, defsr(0.004)
			, rpol(AAFilter)
			{}

    int			nrsamples;
    int			startsmpls;
    const Quantity*	denqty;
    const Quantity*	aiqty;
    const Quantity*	velqty;
    const Quantity*	outqty;
    const Property*	outpty;
    bool		issonic;
    float		defsr;
    ResamplePolicy	rpol;

};


class SynthSeisMaker
{
public:
			SynthSeisMaker(const SynthSeisPars&,const Wavelet*);
			~SynthSeisMaker();
    int			state() const		{ return state_; }

    bool		generate(const IFRUnitList&,SeisTrc&,const IFRealUnit*);

    int			phase;		// antialias filter def 0=zero-phase
    int			outputseis;	// 0=AI/Qty, 1='real', 2=sail

    void		setResampling(int);

protected:

    float		sr;
    int			ns_out;
    int			ns_dense;
    float		sr_dense;
    int			state_;
    const Wavelet*	wavelet;
    float*		dataptr;
    float*		tmpptr;
    const Quantity*	velqty;
    bool		issonic;
    const Quantity*	denqty;
    const Quantity*	aiqty;
    const Quantity*	outqty;
    const Property*	outpty;
    int			startsmpls;
    float		reftime;
    float		defsr;
    SynthSeisPars::ResamplePolicy rpol;

    void		getQtyVals(IFRealUnit*,float&,float&);
    bool		generateValues(const IFRUnitList&,const IFRealUnit*);
    void		getReflFromImp(float*,float*,int,bool);

    int			resampling; // Factor to sample log finer
				    // default = 20

};


#endif
