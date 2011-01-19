#ifndef raytrace1d_h
#define raytrace1d_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: raytrace1d.h,v 1.1 2011-01-19 16:20:10 cvsyuancheng Exp $
________________________________________________________________________

*/

#include "complex"
#include "factory.h"
#include "odmemory.h"
#include "objectset.h"
#include "task.h"


template <class T> class Array2D;
typedef std::complex<float> float_complex;


/*!\brief

*/


mClass RayTracer1D : public ParallelTask
{
public:
    			RayTracer1D();
			mDefineFactoryInClass(RayTracer1D,factory);
			~RayTracer1D();			
    struct Layer
    {
			Layer();
        Layer&		operator=(const Layer&);

	float		d0_;
	float		Vint_;
	float		delta_;
	float		epsilon_;
	float		eta_;
	float		dip_;
	float		density_;
    };

    struct GeomSpread   { enum Type { None, Distance, Vint }; };
    GeomSpread::Type	geomSpread() const	{ return geomspread_; }
    void		setGeomSpread(GeomSpread::Type t) { geomspread_ = t; }

    void		setDownType(bool pwave)	{ downisp_ = pwave; }
    bool		getDownTye() const	{ return downisp_; }
    void		setUpType(bool pwave)	{ upisp_ = pwave; }
    bool		getUpType() const	{ return upisp_; }

    void		setSourceDepth(float d)	{ sourcedepth_ = d; }
    float		sourceDepth() const	{ return sourcedepth_; }
    void		setReceiverDepth(float d) { receiverdepth_ = d; }
    float		receiverDepth() const	{ return receiverdepth_; }

    void		setModel(bool pwave,const ObjectSet<Layer>&,
	    			 OD::PtrPolicy);
    void		setOffsets(const TypeSet<float>& offsets);

    void		doAngleOnly(bool yn)	{ angleonly_ = yn; }
    bool		angleOnly() const	{ return angleonly_; }
    void		projectInCwave(bool yn)	{ projectincwave_ = yn; }
    bool		projectInCwave() const	{ return projectincwave_; }

    float_complex	getReflectivity(int layer,int offset) const;
    float		getSinAngle(int layer,int offset) const;

protected:

    od_int64		nrIterations() const;
    bool		doPrepare(int);
    bool		doWork(od_int64,od_int64,int);
    virtual bool	rayTrace(int layer)			= 0;

    float		findRayParam(int layer,float offset,float seed) const;
    float		getOffset(int layer,float rayparam) const;
    bool		computeReflectivity(int layer,float rayparam,
	    				    float_complex& ref) const;

    TypeSet<float>		offsets_;
    ObjectSet<Layer>		pmodel_;
    ObjectSet<Layer>		smodel_;

    TypeSet<float>		velmax_; 

    Array2D<float>*		sini_;
    Array2D<float_complex>*	reflectivity_;

    GeomSpread::Type		geomspread_;
    bool			ownspmodel_;
    bool			ownssmodel_;

    bool			downisp_;
    bool			upisp_;
    
    bool			angleonly_;
    bool			projectincwave_;

    int				sourcelayer_;
    float			sourcedepth_;
    float			relsourcedepth_;

    int				receiverlayer_;
    float			receiverdepth_;
    float			relreceiverdepth_;
};
    

#endif
