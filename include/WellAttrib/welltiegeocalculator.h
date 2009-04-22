#ifndef welltiegeocalculator_h
#define welltiegeocalculator_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Apr 2009
 RCS:           $Id: welltiegeocalculator.h,v 1.2 2009-04-22 09:22:06 cvsbruno Exp $
________________________________________________________________________

-*/


/*
  brief class to perform the computations needed by TWTS  
*/   
#include "namedobj.h"

template <class T> class Array1DImpl;
class WellTieSetup;
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
			WellTieGeoCalculator(const WellTieSetup&,
					     const Well::Data&);
			~WellTieGeoCalculator() {};


		    
    void 		interpolateData(TypeSet<float>&,float,bool);
    Well::D2TModel* 	getModelFromVelLog(bool);
    void 		TWT2Vel(const TypeSet<float>&,const TypeSet<float>&,
				TypeSet<float>&,bool);
    void 		lowPassFilter(Array1DImpl<float>&,float);
    void		resampleData(const Array1DImpl<float>&,
				     Array1DImpl<float>&,float);
    void                computeAI(const Array1DImpl<float>&,
				  const Array1DImpl<float>&,
				  Array1DImpl<float>&,bool=true);
    void                computeReflectivity(const Array1DImpl<float>&,
					    Array1DImpl<float>&);
    void                convolveWavelet(const Array1DImpl<float>&,
	    				const Array1DImpl<float>&, 
					Array1DImpl<float>&,int);
    void 		deconvolve( const Array1DImpl<float>&,
				    const Array1DImpl<float>&,
				    Array1DImpl<float>&);
    void 		stackWavelets(const TypeSet<float>&,
				      const TypeSet<float>&,
				      TypeSet<float>&);

    protected:

    const Well::Data&	wd_;
    const WellTieSetup& wtsetup_;
    const double 	denfactor_;
    const double 	velfactor_;
};

#endif
