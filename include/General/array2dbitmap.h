#ifndef array2dbitmap_h
#define array2dbitmap_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2006
 RCS:           $Id: array2dbitmap.h,v 1.3 2006-09-12 15:44:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "sets.h"
#include "arraynd.h"


typedef Array2D<char>	A2DBitMap;
#define A2DBitMapImpl	Array2DImpl<char>


/*! \brief Array2D Bitmap generation parameters */

struct A2DBitmapGenPars
{
		A2DBitmapGenPars()
		  : nointerpol_(false)
		  , autoscale_(true)
		  , clipratio_(0.025)
		  , scale_(0,1)			{}

    bool	nointerpol_;	//!< Do not interpolate between actual samples
    bool	autoscale_;	//!< If not, use the scale_
    float	clipratio_;	//!< ratio of numbers to be clipped before
    				//!< determining min/max, only when autoscale_
    Interval<float> scale_;	//!< Used when autoscale_ is false

    static const char cNoFill;	//!< -127, = background/transparent

};


/*! \brief Array2D<float>& + statistics */

class A2DBitMapInpData
{
public:

    			A2DBitMapInpData( Array2D<float>& d )
			    : data_(d)	{ collectData(); }

    Array2D<float>&	data()		{ return data_; }
    const Array2D<float>& data() const	{ return data_; }
    int			nrPts() const	{ return statpts_.size(); }
    Interval<float>	scale(float clipratio) const;
    virtual float	midVal() const
					{ return statpts_[nrPts()/2]; }

    void		collectData(); //!< again.

protected:

    Array2D<float>&	data_;
    TypeSet<float>	statpts_;

    void		selectData();

};


/*! \brief Array2D Bitmap generation setup

The first dimension may be irregularly sampled. The second dimension must
be 100% regular. For the first dimension, you set up the axis by providing
the positions in a float array. If you don't provide that array, one will be
generated, the postions are assumed to be: 0 1 2 ..., which is also what
is done for the second dimension (which has to be regular).

Then, you can zoom in by setting the different ranges. The default ranges will
be -0.5 to N-0.5, i.e. half a distance between the cols/rows is added on all
sides as border.

The positions in dim 0 *must* be sorted, only the distances may vary.
The average distance between the positions is used to calculate the
default border.

Dim 0 <-> X
Dim 1 <-> Y

*/


class A2DBitmapPosSetup
{
public:

			A2DBitmapPosSetup(const Array2DInfo&,float* dim0pos=0);
				// If passed, dim0pos becomes mine
    virtual		~A2DBitmapPosSetup();

    void		setPositions(float* dim0positions);
				//!< dim0posistions will become mine
    inline const float*	dim0Positions() const
    			{ return dim0pos_; }
    int			dimSize( int dim ) const
    			{ return dim ? szdim1_ : szdim0_; }

    			// 'zoom in' (or out)
    void		setDim0Range( const Interval<float>& r ) { dim0rg_ = r;}
    void		setDim1Range( const Interval<float>& r ) { dim1rg_ = r;}

    inline const Interval<float>& dimRange( int dim ) const
    			{ return dim ? dim1rg_ : dim0rg_; }
    inline float	avgDist( int dim ) const
    			{ return dim ? 1 : dim0avgdist_; }
    inline float	dimEps( int dim ) const
			{ return 1e-6 * avgDist(dim); }

    void		setBitmapSizes(int,int) const;
    inline int		nrXPix() const		{ return nrxpix_; }
    inline int		nrYPix() const		{ return nrypix_; }

    inline float	getPixPerDim( int dim ) const
			{ return dim ? pixperdim1_ : pixperdim0_; }
    inline float	getPixOffs( int dim, float pos ) const
			{ return  (pos - (dim ? dim1rg_ : dim0rg_).start)
				* getPixPerDim( dim ); }

    int			getPix(int dim,float) const;
    			// Nr of pixels from (0,0), always inside bitmap
    bool		isInside(int dim,float) const;
    			// Is position in dim inside bitmap?

protected:


    float*		dim0pos_;
    int			szdim0_;
    int			szdim1_;

    Interval<float>	dim0rg_;
    Interval<float>	dim1rg_;

    float		dim0avgdist_;

    // Vars for current bitmap
    int			nrxpix_;
    int			nrypix_;
    float		pixperdim0_;
    float		pixperdim1_;

};


/*!\brief Generates Array2D bitmap from Array2D<float> */

class A2DBitmapGenerator
{
public:

    virtual		~A2DBitmapGenerator()	{ delete &pars_; }

    void		setBitmap(A2DBitMap&);
    A2DBitMap&		bitmap()		{ return *bitmap_; }
    const A2DBitMap&	 bitmap() const		{ return *bitmap_; }
    int			bitmapSize(int dim) const;

    void		initBitmap();	//!< with A2DBitmapGenPars::cNoFill
    void		fill();
    virtual bool	dump(std::ostream&)	{ return false; }

    A2DBitmapGenPars&		pars()		{ return pars_; }
    const A2DBitmapGenPars&	pars() const	{ return pars_; }
    const A2DBitMapInpData&	data() const	{ return data_; }
    const A2DBitmapPosSetup&	setup() const	{ return setup_; }

protected:

				A2DBitmapGenerator(const A2DBitMapInpData&,
						   const A2DBitmapPosSetup&,
						   A2DBitmapGenPars&);
				   //!< pass a new instance of (subclass of)
				   //!< A2DBitmapGenPars

    const A2DBitMapInpData&	data_;
    const A2DBitmapPosSetup&	setup_;
    A2DBitmapGenPars&		pars_;
    A2DBitMap*			bitmap_;

    // handy vars, duplicated from respective objects
    int				szdim0_;
    int				szdim1_;
    const float*		dim0pos_;
    Interval<float>		dim0rg_;
    Interval<float>		dim1rg_;
    float			dim0perpix_;
    float			dim1perpix_;
    Interval<float>		scalerg_;
    float			scalewidth_;

    virtual void		doFill()		= 0;

};


#endif
