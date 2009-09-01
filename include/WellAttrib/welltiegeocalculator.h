#ifndef welltiegeocalculator_h
#define welltiegeocalculator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
 RCS:           $Id: welltiegeocalculator.h,v 1.11 2009-09-01 14:20:57 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "arrayndimpl.h"
#include "fft.h"

#include <complex>

/*
  brief class to perform the computations needed by TWTS  
*/   

class WellTieSetup;
class Wavelet;
class WellTieParams;
class IOPar;
namespace Well
{
    class MGR; 
    class Data;
    class D2TModel;
}


mClass WellTieGeoCalculator
{
public:
			WellTieGeoCalculator(const WellTieParams*,
					     const Well::Data*);
			~WellTieGeoCalculator() {};

//d2tm operations
    Well::D2TModel* 	getModelFromVelLog(const char*, bool);
     void 		TWT2Vel(const TypeSet<float>&,
	     			const TypeSet<float>&,
				TypeSet<float>&,bool);

//logs operations
    void                computeAI(const Array1DImpl<float>&,
				 const Array1DImpl<float>&,Array1DImpl<float>&);
    void                computeReflectivity(const Array1DImpl<float>&,
					    Array1DImpl<float>&,int);
    void 		interpolateLogData(TypeSet<float>&,float,bool);
    bool 		isValidLogData(const TypeSet<float>&);
    void		removeSpikes(TypeSet<float>&);
    int 		getFirstDefIdx(const TypeSet<float>&);
    int 		getLastDefIdx(const TypeSet<float>&);

//wvlt operations
    void                convolveWavelet(const Array1DImpl<float>&,
	    				const Array1DImpl<float>&, 
					Array1DImpl<float>&,int);
    void 		deconvolve( const Array1DImpl<float>&,
				    const Array1DImpl<float>&,
				    Array1DImpl<float>&,int);

//stretch/squeeze
    void		stretch();
    void 		doStretch(int,int,float);
    const int 		getIdx(const Array1DImpl<float>&,float) const;

    mStruct StretchData
    {			StretchData()
			    : inp_(0)
			    , outp_(0)
			    , stretchfac_((&pick2_-&start_)/(&pick1_-&start_))
			    , squeezefac_((&stop_-&pick2_)/(&stop_-&pick1_))
			    {}

	const Array1DImpl<float>* inp_;
	Array1DImpl<float>* outp_;
	int 		start_;
	int		stop_;
    	int 		pick1_;	
    	int 		pick2_;	
	float		stretchfac_;
	float		squeezefac_;
    };

    StretchData& 	getStretchData() { return sd_; }

//others   
    void 		crosscorr( const Array1DImpl<float>&,
				  const Array1DImpl<float>&,
				  Array1DImpl<float>&);
    void 		lowPassFilter(Array1DImpl<float>&,float);
    void		resampleData(const Array1DImpl<float>&,
				     Array1DImpl<float>&,float);

protected:

    double 		denfactor_;
    double 		velfactor_;
    const Well::Data&	wd_;
    const WellTieSetup& wtsetup_;
    const WellTieParams& params_;
    StretchData		sd_;

};

#endif
