#ifndef array2dinterpolimpl_h
#define array2dinterpolimpl_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Feb 2009
 RCS:           $Id: array2dinterpolimpl.h,v 1.5 2009-05-18 21:21:49 cvskris Exp $
________________________________________________________________________


-*/

#include "array2dinterpol.h"
#include "thread.h"
#include "rowcol.h"
#include "position.h"

template <class T> class Array2DImpl;
class RowCol;
class DAGTriangleTree;


/*!Class that interpolates 2D arrays with inverse distance.

Parameters:
searchradius	sets the search radius. Should have the same unit as
		given in setRowStep and getColStep.
		If undefined, all defined nodes will be used, and no other
		settings will be used.
stepsize/nrsteps
		sets how many nodes that will be done in each step. Each step 
		will only use points defined in previous steps (not really true
		as shortcuts are made). In general, larger steps gives faster
		interpolation, but lower quality. If stepsize is 10, and nrsteps
		is 10, a border of maximum 100 nodes will be interpolated around
		the defined positions.

cornersfirst	if true, algorithm will only interpolate nodes that has the 
		same number of support (i.e. neigbors) within +-stepsize, before
		reevaluating which nodes to do next. Enabling cornersfirst will
		give high quality output at the expense of speed.
*/

mClass InverseDistanceArray2DInterpol : public Array2DInterpol
{
public:
    static void			initClass();
    static const char*		sType()		{ return "Inverse distance"; }
    const char*			type() const 	{ return sType(); }
    static Array2DInterpol*	create();

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

protected:
    bool	doWork(od_int64,od_int64,int);
    od_int64	nrIterations() const		{ return totalnr_; }
    const char*	nrDoneText() const		{ return "Nodes gridded"; }

    bool	doPrepare(int);

    bool	initFromArray(TaskRunner*);
    od_int64	getNextIdx();
    void	reportDone(od_int64);

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
    TypeSet<int>		definedidxs_;		//Only when no radius
    TypeSet<RowCol>		neighbors_;		//Only when radius
    TypeSet<float>		neighborweights_;	//Only when radius


    				//Per support size				
    TypeSet<int>		addedwithcursuport_;
    int				nraddedthisstep_;
    int				prevsupportsize_;

    				//perstep
    TypeSet<int>		todothisstep_;
    TypeSet<int>		nrsources_;	//linked to todothisstep or zero
};


mClass TriangulationArray2DInterpol : public Array2DInterpol
{
public:

    static const char*		sType()		{ return "Triangulation"; }
    const char*			type() const	{ return sType(); }
    static void			initClass();
    static Array2DInterpol*	create();

    		TriangulationArray2DInterpol();
    		~TriangulationArray2DInterpol();

    bool	setArray(Array2D<float>&,TaskRunner*);
    bool	canUseArrayAccess() const	{ return true; }
    bool	setArray(ArrayAccess&,TaskRunner*);

protected:
    bool	doWork(od_int64,od_int64,int);
    od_int64	nrIterations() const		{ return totalnr_; }
    const char*	nrDoneText() const		{ return "Nodes gridded"; }

    bool	doPrepare(int);

    bool	initFromArray(TaskRunner*);

    				//triangulation stuff
    DAGTriangleTree*		triangulation_;
    TypeSet<Coord>		coordlist_;
    TypeSet<int>		coordlistindices_;
    int				firstthreadtestpos_;
    TypeSet<int>		corner2conns_;
    TypeSet<double>		corner2weights_;
    TypeSet<int>		corner3conns_;
    TypeSet<double>		corner3weights_;
    TypeSet<int>		corner4conns_;
    TypeSet<double>		corner4weights_;

    				//Working arrays
    bool*			curdefined_;
    bool*			nodestofill_;

    				//Work control
    od_int64			totalnr_;
};



#endif
