#ifndef fingervein_h
#define fingervein_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bo Zhang/Yuancheng Liu
 Date:          July 2012
 RCS:           $Id$
________________________________________________________________________


-*/

#include "algomod.h"
#include "factory.h"

template <class T> class Array2D;
template <class T> class Array3D;

class TaskRunner;

/*!
\ingroup Algo
\brief Get a flag output for faults based on 2D input attribute data.
*/

mClass(Algo) FingerVein
{
public:    

    				FingerVein(const Array2D<float>&,
					   float threshold,bool isabove,
					   bool istimeslcie,
					   Array2D<bool>& output);
				~FingerVein()	{}

    bool			compute(bool domerge=true,bool dothinning=true,
	    				int minfltlength=15,
					float overlaprate=0.5,
					int sigma=3,float thresholdpercent=0.93,
					TaskRunner* tr=0);
    const TypeSet<TypeSet<int> >& validConnComponents() const 
    				{ return validconncomps_; }
    const TypeSet<int>& 	nrConnComponents() const { return nrcomps_; }
    const TypeSet<int>& 	compIndices() const  	 { return compids_; }
    void			thinning(Array2D<bool>& res,bool skeleton=true);

protected:

    void			removeSmallComponents(Array2D<bool>&,
	    				int minfltlength,float overlaprate,
					bool savecomps=true);

    const Array2D<float>&	input_;
    Array2D<bool>&		output_;
    float			threshold_;
    bool			isabove_;
    bool			istimeslice_;
    TypeSet<TypeSet<int> >	validconncomps_;
    TypeSet<int>		nrcomps_;
    TypeSet<int>		compids_;
};


/*!
\ingroup Algo
\brief Calculate azimuth and dip for 3D data.
*/

mClass(Algo) FaultOrientation
{
public:    

    				FaultOrientation();
				~FaultOrientation();

    void			setThreshold(float threshold,bool isabove);
    void			setMinFaultLength(int minleng);
    void			setParameters(int sigma=3,float scorerate=0.9);
    				/*!<sigma is Gaussian kernel sigma, scorerate 
				  is between 0 and 1.*/
    bool			compute(const Array3D<float>& input,
	    				bool forazimuth=true,bool fordip=true,
					TaskRunner* tr=0);

    const Array3D<float>*	getAzimuth() const { return azimuth_stable_; }
    const Array3D<float>*	getDip() const	   { return dip_stable_; }

    enum ConfidenceLevel	{ Low=0, Median=1, High=2 };
    const Array3D<bool>*	getFaultConfidence(ConfidenceLevel);
    
    static bool			compute2DVeinBinary(const Array2D<float>& img, 
		    			float threshold, bool isabove,
					int minlength,int sigma,float perc,  
					bool is_t_slic,Array2D<bool>& vein_bina,
					TaskRunner* tr);
    static bool			computeMaxCurvature(const Array2D<float>& inp, 
					int sigma,bool is_t_slic,
					Array2D<float>& vein_score,
					TaskRunner* tr);
    static void			thinning(const Array2D<bool>& input,
		   			 Array2D<bool>& res);
    static void			skeletonHilditch( Array2D<char>& input);
    static void			computeComponentAngle(
	    				const Array2D<bool>& base_bina_sect,
					const Array2D<bool>& upgr_bina_sect,
					int elem_leng,float null_val,
					Array2D<float>& azimuth_sect);
protected:

    friend class		veinSliceCalculator;
    friend class		azimuthPcaCalculator;

    void			cleanAll();
    bool			compute2D(const Array3D<float>& input,
	    				  TaskRunner* tr=0);
    bool			computeVeinSlices(const Array3D<float>& input,
					Array3D<bool>& output,TaskRunner* tr);
    void			computeVerticalVeinSlice(
	    				const Array3D<float>& input,
					Array3D<bool>& vein_bina_0,
					Array3D<bool>& vein_bina_45, 
	    				Array3D<bool>& vein_bina_90,
					Array3D<bool>& vein_bina_135);
    bool			computeAzimuthPCA(const Array3D<bool>& conf_low,
					const Array3D<bool>& conf_medi,
					int elem_leng,float null_val,
					Array3D<float>& output,TaskRunner* tr);
    void			stabilizeAzimuth(const Array3D<bool>& conf_low,
	    				const Array3D<float>& azimuth_pca, 
					int elem_leng,float null_val,
			    		Array3D<float>& output);
    void			setFaultConfidence(const Array3D<bool>& vbina,
	    				const Array3D<bool>& vein_bina_0,
	    				const Array3D<bool>& vein_bina_45,
    					const Array3D<bool>& vein_bina_90,
					const Array3D<bool>& vein_bina_135,
					Array3D<bool>& conf_low, 
					Array3D<bool>& conf_medi,
					Array3D<bool>& conf_high);
    void			stablizeDip(const Array3D<bool>& conf_low,
	    				const Array3D<float>& azm_pca_stab,
	    				const Array3D<float>& dip_pca, 
    					int wind_size,int elem_leng,
					float null_val,Array3D<float>& output);
    void			computeDipPCA(const Array3D<bool>& conf_low,
					const Array3D<bool>& conf_medi, 
					const Array3D<float>& azm_pca_stab,
    					int wind_size,int elem_leng,
					float null_val,Array3D<float>& dip_pca);
    void			stabilizeAngleSection( 
	    				const Array2D<bool>& base_bina,
		    			const Array2D<float>& azimuth_orig, 
					int elem_leng,float uppr_perc,
					float lowr_perc,float angl_tole,
					float null_value,Array2D<float>& res);
    static void			thinStep(const Array2D<bool>& input,
					 Array2D<bool>& output,bool isfirst);
    static float		getAnglePCA(const TypeSet<int>& point_set_x,
					  const TypeSet<int>& point_set_y,
					  float null_value);

    float			threshold_;
    float			scorerate_;
    bool			isfltabove_;
    int				minfaultlength_;
    int				sigma_;
    Array3D<float>*		azimuth_stable_;
    Array3D<float>*		dip_stable_; 
    Array3D<bool>*		conf_low_; 
    Array3D<bool>*		conf_med_; 
    Array3D<bool>*		conf_high_; 
};

#endif
