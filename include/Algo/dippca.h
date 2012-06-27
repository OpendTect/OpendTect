#ifndef dippca_h
#define dippca_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bo Zhang/Y.Liu
 Date:          June 2012
 RCS:           $Id: dippca.h,v 1.1 2012-06-27 16:23:02 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "enums.h"
#include "factory.h"
#include "odmemory.h"

template <class T> class Array2D;
template <class T> class Array3D;

class Dip2DCalculator;
class Dip3DCalculator;
class TaskRunner;

/*Calculate Dip/Azimuth use the method of PCA. Example of use:

  Dip3D d3d( your data )
  d3d.setSetup( your setup )
  d3d.compute();
 */


mClass DipPCA
{
public:    

    				~DipPCA()			{}
    struct Setup
    {				Setup();
	Setup&			operator=(const Setup&);

	float			threshold_;
	bool			isabove_; //Flt value is above threshold or not
	int			boxlength_;
	int			boxwidth_;
	int			boxheight_;
	StepInterval<int>	thetarg_; //dip angle in positive degree
	StepInterval<int>	alpharg_; //azimuth angle in positive degree
    };	

    void			setSetup(Setup nsetup)	{ setup_ = nsetup; }
    virtual bool		compute(TaskRunner* tr=0) { return true; } 

protected:

    static bool			get2DGridDip(const TypeSet<int>& xs, 
	    			       const TypeSet<int>& ys,
				       float xdist,float ydist,float& dip);
    static bool			get3DGridDip(const TypeSet<int>& xs, 
	    			       const TypeSet<int>& ys,
				       const TypeSet<int>& zs,
				       float xdist, float ydist, float zdist,
				       float& absdip,float& inldip,
				       float& crldip,float& azimuth);
    Setup			setup_;
};



mClass Dip2D : public DipPCA
{
public:
    				Dip2D(const Array2D<float>& input,
					   float xdist,float ydist);
				~Dip2D();

    bool			compute(TaskRunner* tr=0); 
    
    enum Output			{ Dip=0, Dilation=1, Thinning=2 };
    const Array2D<float>*	get(Output) const;

protected:

    friend class		Dip2DCalculator;
    bool			fillGap();


    const Array2D<float>&	input_;

    Array2D<float>*		dip_; //The fault dip
    Array2D<float>*		dilation_;	
    				//The fault dip after perform dilation to dip
    Array2D<float>*		dthinner_;
    				//The fault dip after thining to dilation

    const int			xsz_;
    const int			ysz_;
    const float			xdist_;
    const float			ydist_;
};


mClass Dip3D : public DipPCA
{
public:
    				Dip3D(const Array3D<float>& input,
					float xdist,float ydist,float zdist);
				~Dip3D();

    bool			compute(TaskRunner* tr=0); 
    
    enum Output			{ AbsDip=0, InlDip=1, CrlDip=2, Azimuth=3 };
    const Array3D<float>*	get(Output) const;

protected:

    friend class		Dip3DCalculator;
    const Array3D<float>&	input_;

    Array3D<float>*		absdip_; 
    Array3D<float>*		inldip_; 
    Array3D<float>*		crldip_; 
    Array3D<float>*		azimuth_;	

    const int			xsz_;
    const int			ysz_;
    const int			zsz_;
    const float			xdist_;
    const float			ydist_;
    const float			zdist_;
};

#endif
