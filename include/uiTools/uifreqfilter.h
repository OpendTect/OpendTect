#ifndef uifreqfilter_h
#define uifreqfilter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2012
 RCS:           $Id: uifreqfilter.h,v 1.1 2012-04-26 14:32:10 cvsbruno Exp $
________________________________________________________________________

-*/

#include "fftfilter.h"
#include "uigroup.h"


class uiGenInput;

mClass uiFreqFilterSelFreq : public uiGroup
{
public:
    			uiFreqFilterSelFreq(uiParent*);
    			~uiFreqFilterSelFreq();
    mStruct Params
    {
	FFTFilter::Type filtertype_;
	float 		minfreq_;
	float 		maxfreq_;
    };
    const Params&	params() const 		{ return params_; }

protected:
    uiGenInput*		typefld_;
    uiGenInput*		freqfld_;
    Params		params_;

    virtual void	putToScreen();
    virtual void	getFromScreen(CallBacker*);
    virtual void	typeSel(CallBacker*);
};


#endif
