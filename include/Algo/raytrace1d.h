#ifndef raytrace1d_h
#define raytrace1d_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: raytrace1d.h,v 1.8 2011-01-28 05:33:55 cvskris Exp $
________________________________________________________________________

*/

#include "odmemory.h"
#include "objectset.h"
#include "task.h"

template <class T> class Array2D;
class AILayer;
class IOPar;

mClass RayTracer1D : public ParallelTask
{
public:
    mClass Setup
    {
    public:
			Setup() 
			    : pdown_( true )
			    , pup_( true )
			    , sourcedepth_( 0 )
			    , recieverdepth_( 0 )
			{}

				mDefSetupMemb(bool,pdown);
				mDefSetupMemb(bool,pup);
				mDefSetupMemb(float,sourcedepth);
				mDefSetupMemb(float,recieverdepth);

	virtual void		fillPar(IOPar&) const;
	virtual bool		usePar(const IOPar&);

	static const char*	sKeyPWave()	{ return "Wavetypes"; }
	static const char*	sKeySRDepth()	{ return "SR Depths"; }
    };


    			RayTracer1D(const Setup&);
    virtual		~RayTracer1D();			
    Setup&		setup()				{ return setup_; }
    const Setup&	setup() const			{ return setup_; }

    void		setModel(bool pmodel,const TypeSet<AILayer>&);
    			/*!<Note, if both p-model and s-model are set,
			    they should be identical with regards to thers sizes
			    and the layrer's depths. */
    void		setOffsets(const TypeSet<float>& offsets);

    			//Available after execution
    float		getSinAngle(int layeridx,int offsetidx) const;
    float*		getSinAngleData() const;	

protected:
    friend class	OffsetFromRayParam;

    od_int64		nrIterations() const;
    virtual bool	doPrepare(int);
    bool		doWork(od_int64,od_int64,int);
    float		findRayParam(int layer,float offset,float seed) const;
    virtual bool	compute(int layer,int offsetidx,float rayparam);
    virtual float	getOffset(int layer,float rayparam) const;

    				//Model
    const TypeSet<AILayer>*	pmodel_;
    const TypeSet<AILayer>*	smodel_;
    TypeSet<float>		offsets_;
    Setup			setup_;

    				//Runtime variables
    TypeSet<float>		velmax_; 
    int				sourcelayer_;
    int				receiverlayer_;
    float			relsourcedepth_;
    float			relreceiverdepth_;

				//Results
    Array2D<float>*		sini_;
};


#endif
