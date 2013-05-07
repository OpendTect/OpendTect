#ifndef array2dinterpolimpl_h
#define array2dinterpolimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Feb 2009
 RCS:           $Id$
________________________________________________________________________


-*/

#include "algomod.h"
#include "array2dinterpol.h"
#include "thread.h"
#include "rowcol.h"
#include "position.h"

template <class T> class Array2DImpl;
class RowCol;
class DAGTriangleTree;
class Triangle2DInterpolator;
class Extension2DInterpolExecutor;


/*!
\brief Interpolates 2D arrays using inverse distance method.

  Parameters:
  
  1. searchradius - 
		sets the search radius. Should have the same unit as
		given in setRowStep and getColStep.
		If undefined, all defined nodes will be used, and no other
		settings will be used.
  2. stepsize/nrsteps - 
		sets how many nodes that will be done in each step. Each step 
		will only use points defined in previous steps (not really true
		as shortcuts are made). In general, larger steps gives faster
		interpolation, but lower quality. If stepsize is 10, and nrsteps
		is 10, a border of maximum 100 nodes will be interpolated around
		the defined positions.
  3. cornersfirst - 
		if true, algorithm will only interpolate nodes that has the 
		same number of support (i.e. neigbors) within +-stepsize, before
		reevaluating which nodes to do next. Enabling cornersfirst will
		give high quality output at the expense of speed.
*/

mExpClass(Algo) InverseDistanceArray2DInterpol : public Array2DInterpol
{
public:
		mDefaultFactoryInstantiation(Array2DInterpol, 
			InverseDistanceArray2DInterpol,
			"Inverse distance",sFactoryKeyword());

		InverseDistanceArray2DInterpol();
		~InverseDistanceArray2DInterpol();

    bool	setArray(Array2D<float>&,TaskRunner*);
    bool	canUseArrayAccess() const	{ return true; }
    bool	setArray(ArrayAccess&,TaskRunner*);

    float	getSearchRadius() const		{ return searchradius_; }
    void	setSearchRadius(float r)	{ searchradius_ = r; }

    void	setStepSize(int n)		{ stepsize_ = n; }
    int		getStepSize() const		{ return stepsize_; }

    void	setNrSteps(int n)		{ nrsteps_ = n; }
    int		getNrSteps() const		{ return nrsteps_; }

    void	setCornersFirst(bool n)		{ cornersfirst_ = n; }
    bool	getCornersFirst() const		{ return cornersfirst_; }
    bool	nothingToFill() const		{ return nrIterations()<1; }  

    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
    virtual bool	doWork(od_int64,od_int64,int);
    od_int64		nrIterations() const	{ return totalnr_; }
    const char*		nrDoneText() const	{ return "Nodes gridded"; }

    bool		doPrepare(int);
    virtual bool	initFromArray(TaskRunner*);
    od_int64		getNextIdx();
    void		reportDone(od_int64);

    				//settings
    int				nrsteps_;
    int				stepsize_;
    bool			cornersfirst_;
    float			searchradius_;

    				//workflow control stuff
    int				nrthreads_;
    int				nrthreadswaiting_;
    bool			waitforall_;
    Threads::ConditionVar	condvar_;
    bool			shouldend_;
    int				stepidx_;

    int				nrinitialdefined_;
    int				totalnr_;	

    				//Working arrays
    bool*			curdefined_;
    bool*			nodestofill_;
    TypeSet<od_int64>		definedidxs_;		//Only when no radius
    TypeSet<RowCol>		neighbors_;		//Only when radius
    TypeSet<float>		neighborweights_;	//Only when radius


    				//Per support size				
    TypeSet<od_int64>		addedwithcursuport_;
    int				nraddedthisstep_;
    od_int64			prevsupportsize_;

    				//perstep
    TypeSet<od_int64>		todothisstep_;
    TypeSet<od_int64>		nrsources_;	//linked to todothisstep or zero
};


/*!
\brief Uses triangulation method to interpolate two dimensional arrays.
*/

mExpClass(Algo) TriangulationArray2DInterpol : public Array2DInterpol
{
public:
		mDefaultFactoryInstantiation(Array2DInterpol, 
			TriangulationArray2DInterpol,
			"Triangulation",sFactoryKeyword());

    		TriangulationArray2DInterpol();
    		~TriangulationArray2DInterpol();

    bool	setArray(Array2D<float>&,TaskRunner*);
    bool	canUseArrayAccess() const	{ return true; }
    bool	setArray(ArrayAccess&,TaskRunner*);
    bool	nothingToFill() const		{ return nrIterations()<1; } 

    bool	doInterpolation() const		{ return dointerpolation_; }
    void	doInterpolation(bool yn)	{ dointerpolation_ = yn; }
    float	getMaxDistance() const		{ return maxdistance_; }
    void	setMaxDistance(float r)		{ maxdistance_ = r; }
    
    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
    int		minThreadSize() const		{ return 10000; }
    bool	doWork(od_int64,od_int64,int);
    od_int64	nrIterations() const		{ return totalnr_; }
    const char*	nrDoneText() const		{ return "Nodes gridded"; }

    bool        doPrepare(int);
    virtual bool	initFromArray(TaskRunner*);
    void	getNextNodes(TypeSet<od_int64>&);

    				//triangulation stuff
    bool			dointerpolation_;
    float			maxdistance_;
    DAGTriangleTree*		triangulation_;
    Triangle2DInterpolator*	triangleinterpolator_;
    TypeSet<int>		coordlistindices_;

    				//Working arrays
    bool*			curdefined_;
    bool*			nodestofill_;
    od_int64			curnode_;
    Threads::Mutex		curnodelock_;

    				//Work control
    od_int64			totalnr_;
};


/*!
\brief Array 2D interpolator that works by extending the data into the udf areas.
*/

mExpClass(Algo) ExtensionArray2DInterpol : public Array2DInterpol
{
public:
		mDefaultFactoryInstantiation(Array2DInterpol,
			ExtensionArray2DInterpol,"Extension",sFactoryKeyword());

    		ExtensionArray2DInterpol();
    		~ExtensionArray2DInterpol();

    bool	canUseArrayAccess() const	{ return true; }
    
    void	setNrSteps(int n)		{ nrsteps_ = n; }
    od_int64	getNrSteps() const		{ return nrsteps_; }

    bool	fillPar(IOPar&) const;
    bool	usePar(const IOPar&);

protected:

    bool	doWork(od_int64,od_int64,int);
    od_int64	nrIterations() const		{ return 1; }
    
    od_int64	nrsteps_;   
    
    Extension2DInterpolExecutor* executor_; 
    friend class Extension2DInterpolExecutor;    
};

#endif

