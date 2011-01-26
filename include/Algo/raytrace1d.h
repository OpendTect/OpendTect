#ifndef raytrace1d_h
#define raytrace1d_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: raytrace1d.h,v 1.3 2011-01-26 08:25:01 cvshelene Exp $
________________________________________________________________________

*/

#include "complex"
#include "factory.h"
#include "odmemory.h"
#include "objectset.h"
#include "task.h"

template <class T> class Array2D;
typedef std::complex<float> float_complex;

mClass RayTracer1D : public ParallelTask
{
public:
    			RayTracer1D();
			mDefineFactoryInClass(RayTracer1D,factory);
    virtual		~RayTracer1D();			
    struct Layer
    {
			Layer();
			Layer(const Layer&);
        Layer&		operator=(const Layer&);

	float		d0_;
	float		Vint_;
	float		delta_;
	float		epsilon_;
	float		eta_;
	float		dip_;
	float		density_;
    };

    void		setDownType(bool pwave)	{ downisp_ = pwave; }
    void		setUpType(bool pwave)	{ upisp_ = pwave; }
    bool		getDownTye() const	{ return downisp_; }
    bool		getUpType() const	{ return upisp_; }

    void		setSourceDepth(float d)	{ sourcedepth_ = d; }
    void		setReceiverDepth(float d) { receiverdepth_ = d; }
    float		sourceDepth() const	{ return sourcedepth_; }
    float		receiverDepth() const	{ return receiverdepth_; }

    virtual float	getOffset(int layer,float rayparam) const;
    void		setOffsets(const TypeSet<float>& offsets);
    void		setModel(bool pw,const ObjectSet<Layer>&,OD::PtrPolicy);
    			/*!<\note Model must be in depth. */
protected:

    od_int64		nrIterations() const;
    virtual bool	doPrepare(int);
    bool		doWork(od_int64,od_int64,int);
    float		findRayParam(int layer,float offset,float seed) const;
    virtual bool	compute(int layer,int offsetidx,float rayparam) = 0;

    ObjectSet<Layer>	pmodel_;
    ObjectSet<Layer>	smodel_;
    TypeSet<float>	offsets_;
    TypeSet<float>	velmax_; 

    bool		ownspmodel_;
    bool		ownssmodel_;
    bool		downisp_;
    bool		upisp_;
    
    int			sourcelayer_;
    int			receiverlayer_;
    float		sourcedepth_;
    float		receiverdepth_;
    float		relsourcedepth_;
    float		relreceiverdepth_;
};


mClass AngleRayTracer : public RayTracer1D
{
public:
			mDefaultFactoryInstantiation(RayTracer1D,
				AngleRayTracer,"AngleRaytracer","Angle");
    			AngleRayTracer();
			~AngleRayTracer()		{};			

    float		getSinAngle(int layeridx,int offsetidx) const;

protected:

    bool		doPrepare(int);
    bool		compute(int,int,float);
    
    Array2D<float>*	sini_;
};


mClass IsotropicRayTracer : public AngleRayTracer
{
public:
			mDefaultFactoryInstantiation(RayTracer1D,
				IsotropicRayTracer,"IsotropicRaytracer",
				"Isotropic");
    			IsotropicRayTracer();
			~IsotropicRayTracer()			{};

    struct GeomSpread   { enum Type { None, Distance, Vint }; };
    void		setGeomSpread(GeomSpread::Type t) { geomspread_ = t; }
    GeomSpread::Type	geomSpread() const	{ return geomspread_; }
    
    void		projectInCwave(bool yn)	{ projectincwave_ = yn; }
    bool		projectInCwave() const	{ return projectincwave_; }

    float_complex	getReflectivity(int layeridx, int offsetidx) const;

protected:

    bool		doPrepare(int);
    bool		compute(int,int,float);

    bool		projectincwave_;
    GeomSpread::Type	geomspread_;
    Array2D<float_complex>* reflectivity_;
};

#endif
