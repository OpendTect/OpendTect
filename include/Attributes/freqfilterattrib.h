#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2003
________________________________________________________________________

-*/


#include "attributesmod.h"
#include "attribprovider.h"
#include "arrayndalgo.h"
#include "arrayndimpl.h"
#include <complex>


namespace Attrib
{

/*!
\brief %Frequency filtering attribute.

<pre>
  FreqFilter type=LowPass,HighPass,BandPass minfreq= maxfreq= nrpoles=
	     isfftfilter= window=

  Input:				  ||
  0	  Real data			  ||0	  Real Data
					  ||1	  Imaginary Data
  Output:				  ||
  0	  Frequency filtered data	  ||0	  Frequency filtered data
	    (Butterworth Filter)	  ||	       (FFT Filter)
</pre>
*/

mExpClass(Attributes) FreqFilter : public Provider
{ mODTextTranslationClass(FreqFilter);
public:
    static void		initClass();
			FreqFilter(Desc&);

    static const char*	attribName()		{ return "FreqFilter"; }
    static const char*	filtertypeStr()		{ return "type"; }
    static const char*	minfreqStr()		{ return "minfreq"; }
    static const char*	maxfreqStr()		{ return "maxfreq"; }
    static const char*	nrpolesStr()		{ return "nrpoles"; }
    static const char*	isfftfilterStr()	{ return "isfftfilter"; }
    static const char*	isfreqtaperStr()	{ return "isfreqtaper"; }
    static const char*	windowStr()		{ return "window"; }
    static const char*	fwindowStr()		{ return "fwindow"; }
    static const char*	paramvalStr()		{ return "paramval"; }
    static const char*	freqf1Str()		{ return "highfreqparamval"; }
    static const char*	freqf4Str()		{ return "lowfreqparamval"; }
    static const char*	filterTypeNamesStr(int);

protected:
			~FreqFilter();
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);
    static void		updateDefaults(Desc&);

    bool		getInputOutput(int input,
				       TypeSet<int>& res) const override;
    bool		getInputData(const BinID&, int idx) override;
    bool		computeData(const DataHolder&,const BinID& relpos,
			    int t0,int nrsamples,int threadid) const override;
    void		butterWorthFilter(const DataHolder&, int, int);
    void		fftFilter(const DataHolder&, int, int);

    void		setSz(int sz);
    
    const Interval<int>*	desZSampMargin(int input,
					       int output) const override;

    int				filtertype_;
    float			minfreq_;
    float			maxfreq_;
    int				nrpoles_;
    bool			isfftfilter_;
    int				fftsz_;

    ArrayNDWindow*		window_;
    BufferString		windowtype_;
    float			variable_;
    float			freqf1_;
    float			freqf4_;

    Interval<int>		zmargin_;

    Array1DImpl<float_complex>	signal_;
    
    const DataHolder*		redata_;
    const DataHolder*		imdata_;

    int				realidx_;
    int				imagidx_;
};

}; // namespace Attrib

