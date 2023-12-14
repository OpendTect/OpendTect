#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "fftfilter.h"
#include "uigroup.h"


class uiGenInput;

mExpClass(uiTools) uiFreqFilterSelFreq : public uiGroup
{ mODTextTranslationClass(uiFreqFilterSelFreq);
public:
			uiFreqFilterSelFreq(uiParent*);
			~uiFreqFilterSelFreq();

    const Interval<float>& freqRange() const 		{ return freqrg_; }
    FFTFilter::Type	filterType() const 		{ return filtertype_; }

    void		setFreqRange(Interval<float> rg)
    			{ freqrg_ = rg; putToScreen(); }
    void		setMinFreq(float f) { freqrg_.start = f; putToScreen();}
    void		setMaxFreq(float f) { freqrg_.stop = f; putToScreen(); }

    void		setFilterType(FFTFilter::Type tp)
    			{ filtertype_ = tp; putToScreen(); }

    void		set(float minf,float maxf,FFTFilter::Type tp);

    Notifier<uiFreqFilterSelFreq> parchanged;

protected:
    uiGenInput*		typefld_;
    uiGenInput*		freqfld_;

    FFTFilter::Type 	filtertype_;
    Interval<float>	freqrg_;

    virtual void	putToScreen();
    virtual void	getFromScreen(CallBacker*);
    void		parChgCB(CallBacker*);
    virtual void	typeSel(CallBacker*);

private:
    const uiString	sMinMax();
};


mExpClass(uiTools) uiFreqFilter : public uiGroup
{ mODTextTranslationClass(uiFreqFilter);
public:
			uiFreqFilter(uiParent*);
			~uiFreqFilter();

    FFTFilter::Type	filterType() const		{ return filtertype_; }
    TypeSet<float>	frequencies(bool noudf=true) const;
    void		setLowPass(float f3,float f4);
    void		setHighPass(float f1,float f2);
    void		setBandPass(float f1,float f2,float f3,float f4);

    Notifier<uiFreqFilter> valueChanged;

protected:
    uiGenInput*		typefld_;
    uiGenInput*		freqfld_;

    FFTFilter::Type	filtertype_;
    float		f1_, f2_, f3_, f4_;

    void		setFilter(float,float,float,float,FFTFilter::Type);
    bool		zisStdTime() const;

    void		putToScreen();
    void		typeSelCB(CallBacker*);
    void		freqChgCB(CallBacker*);
    void		initGrp(CallBacker*);

};
