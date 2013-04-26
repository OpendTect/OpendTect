#ifndef welltietoseismic_h
#define welltietoseismic_h

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
#include "ailayer.h"
#include "bufstringset.h"
#include "ranges.h"
#include "reflectivitymodel.h"

class LineKey;
class MultiID;
class Wavelet;
namespace Well { class Data; class D2TModel; class Log;}

namespace WellTie
{
    class Data;

mExpClass(WellAttrib) DataPlayer
{
public:
			DataPlayer(Data&,const MultiID&,const LineKey* lk=0);
			~DataPlayer();

    bool 		computeSynthetics(const Wavelet&);
    bool		extractSeismics();
    bool		doFastSynthetics(const Wavelet&);
    bool		isOKSynthetic() const;
    bool		isOKSeismic() const;
    bool		hasSeisId() const;

    bool		computeAdditionalInfo(const Interval<float>&);
    bool		computeCrossCorrelation();
    bool		computeEstimatedWavelet(const int newsz);
    void		setCrossCorrZrg( const Interval<float>& zrg )
    								{ zrg_ = zrg; }

    const char*		errMSG() const		{ return errmsg_.buf(); } 
   
protected:

    bool		setAIModel();
    bool		doFullSynthetics(const Wavelet&);
    bool		copyDataToLogSet();
    bool		processLog(const Well::Log*,Well::Log&,const char*); 
    void		createLog(const char*nm,float* dah,float* vals,int sz);
    bool		checkCrossCorrInps();
    			//!< check input synt/seis and zrg
    bool		extractWvf(const bool issynt);
    bool		extractReflectivity();

    ElasticModel 	aimodel_;
    ReflectivityModel	refmodel_;
    Data&		data_;
    const MultiID&	seisid_;
    const LineKey*	linekey_;
    Interval<float>	zrg_; //!< time range for cross-correlation
    float_complex*	refarr_; //!< reflectivity in the cross-corr window
    float*		syntarr_; //!< waveform for cross-correlation
    float*		seisarr_; //!< waveform for cross-correlation

    mutable BufferString errmsg_;
};

};//namespace WellTie
#endif

