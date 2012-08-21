#ifndef fingervein_h
#define fingervein_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bo Zhang/Yuancheng Liu
 Date:          July 2012
 RCS:           $Id: fingervein.h,v 1.8 2012-08-21 06:08:45 cvsranojay Exp $
________________________________________________________________________


-*/

#include "algomod.h"
#include "factory.h"

template <class T> class Array2D;
template <class T> class Array3D;

class TaskRunner;

/*Get a flag output for faults based on 2D input attribute data.*/

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
protected:

    bool			computeMaxCurvature(Array2D<float>&,int sigma,
	    					    TaskRunner* tr);
    void			thinning(Array2D<bool>& res);
    void			thinStep(const Array2D<bool>& input,
					 Array2D<bool>& output,bool isfirst);
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


/*Calculate azimuth and dip for 3D data.*/
mClass(Algo) FaultAngle
{
public:    

    				FaultAngle(const Array3D<float>&);
				~FaultAngle();

    void			setThreshold(float threshold,bool isabove);
    void			setMinFaultLength(int minleng);
    bool			compute(TaskRunner* tr=0);

    const Array3D<float>*	getAzimuth() const { return azimuth_stable_; }
    const Array3D<float>*	getDip() const	   { return dip_stable_; }
    const Array3D<bool>*	getFaultConfidenceLow() const	
    				{ return conf_low_; }
    const Array3D<bool>*	getFaultConfidenceMed() const	
    				{ return conf_med_; }
    const Array3D<bool>*	getFaultConfidenceHigh() const	
    				{ return conf_high_; }
protected:

    friend class		veinSliceCalculator;
    friend class		azimuthPcaCalculator;

    bool			compute2D(TaskRunner* tr=0);
    bool			vein_t0_slice(const Array3D<float>& input,
	    				int sigma,float percent,
					Array3D<bool>& output,TaskRunner* tr);
    void			vein_vertical_slice(const Array3D<float>& input, 					int sigma,float percent, 
					Array3D<bool>& vein_bina_0,
					Array3D<bool>& vein_bina_45, 
	    				Array3D<bool>& vein_bina_90,
					Array3D<bool>& vein_bina_135);
    bool			azimuth_pca_vein(const Array3D<bool>& conf_low,
					const Array3D<bool>& conf_medi,
					int elem_leng,float null_val,
					Array3D<float>& output,TaskRunner* tr);
    void			azimuth_stabilise(const Array3D<bool>& conf_low,
	    				const Array3D<float>& azimuth_pca, 
					int elem_leng,float null_val,
			    		Array3D<float>& output);
    void			get_fault_confidence(const Array3D<bool>& vbina,
	    				const Array3D<bool>& vein_bina_0,
	    				const Array3D<bool>& vein_bina_45,
    					const Array3D<bool>& vein_bina_90,
					const Array3D<bool>& vein_bina_135,
					Array3D<bool>& conf_low, 
					Array3D<bool>& conf_medi,
					Array3D<bool>& conf_high);
    void			dip_stablise(const Array3D<bool>& conf_low,
	    				const Array3D<float>& azm_pca_stab,
	    				const Array3D<float>& dip_pca, 
    					int wind_size,int elem_leng,
					float null_val,Array3D<float>& output);
    void			dip_pca_vein(const Array3D<bool>& conf_low,
					const Array3D<bool>& conf_medi, 
					const Array3D<float>& azm_pca_stab,
    					int wind_size,int elem_leng,
					float null_val,Array3D<float>& dip_pca);
    void			angle_section_stabilise( 
	    				const Array2D<bool>& base_bina,
		    			const Array2D<float>& azimuth_orig, 
					int elem_leng,float uppr_perc,
					float lowr_perc,float angl_tole,
					float null_value,Array2D<float>& res);
    float			angle_pca(const TypeSet<int>& point_set_x,
					  const TypeSet<int>& point_set_y,
					  float null_value);
    void			get_component_angle(
	    				const Array2D<bool>& base_bina_sect,
					const Array2D<bool>& upgr_bina_sect,
					int elem_leng,float null_val,
					Array2D<float>& azimuth_sect);
    bool			vein_usage(const Array2D<float>& img, 
		    			int fault_min_length,
					int sigma,float perc,  
					bool is_t_slic,Array2D<bool>& vein_bina,
					TaskRunner* tr);
    bool			vein_max_curvature(const Array2D<float>& img, 
		    			int sigma,bool is_t_slic,
					Array2D<float>& vein_score,
					TaskRunner* tr);
    void			thinning(const Array2D<bool>& input,
		   			 Array2D<bool>& res);
    void			thinStep(const Array2D<bool>& input,
					 Array2D<bool>& output,bool isfirst);

    const Array3D<float>&	input_;
    float			threshold_;
    bool			isfltabove_;
    int				minfaultlength_;
    Array3D<float>*		azimuth_stable_;
    Array3D<float>*		dip_stable_; 
    Array3D<bool>*		conf_low_; 
    Array3D<bool>*		conf_med_; 
    Array3D<bool>*		conf_high_; 
};

#endif

