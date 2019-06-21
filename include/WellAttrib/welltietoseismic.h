#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltietoseismic.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "elasticmodel.h"
#include "ranges.h"
#include "reflectivitymodel.h"
#include "uistring.h"

class Wavelet;
namespace Well { class Data; class Log; }

namespace WellTie
{

class Data;


mExpClass(WellAttrib) DataPlayer
{ mODTextTranslationClass(DataPlayer);
public:
			DataPlayer(Data&,const DBKey&,
				   const BufferString& linenm);
			~DataPlayer();

    bool		computeSynthetics(const Wavelet&);
    bool		extractSeismics();
    bool		doFastSynthetics(const Wavelet&);
    bool		isOKSynthetic() const;
    bool		isOKSeismic() const;
    bool		hasSeisId() const;

    bool		computeAdditionalInfo(const Interval<float>&);
    bool		computeCrossCorrelation();
    bool		computeEstimatedWavelet(int newsz);
    void		setCrossCorrZrg( const Interval<float>& zrg )
								{ zrg_ = zrg; }

    const uiString&	errMsg() const		{ return errmsg_; }
    const uiString&	warnMsg() const		{ return warnmsg_; }

protected:

    bool		setAIModel();
    bool		doFullSynthetics(const Wavelet&);
    bool		copyDataToLogSet();
    bool		processLog(const Well::Log*,Well::Log&,const char*);
    void		createLog(const char*nm,float* dah,float* vals,int sz);
    bool		checkCrossCorrInps();
			//!< check input synt/seis and zrg
    bool		extractWvf(bool issynt);
    bool		extractReflectivity();

    ElasticModel	elasticmodel_;
    Data&		data_;
    const DBKey&	seisid_;
    const BufferString& linenm_;
    Interval<float>	zrg_; //!< time range for cross-correlation
    float_complex*	refarr_; //!< reflectivity in the cross-corr window
    float*		syntarr_; //!< waveform for cross-correlation
    float*		seisarr_; //!< waveform for cross-correlation

    uiString		errmsg_;
    uiString		warnmsg_;
};

} // namespace WellTie
