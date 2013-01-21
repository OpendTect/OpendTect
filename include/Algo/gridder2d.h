#ifndef gridder2d_h
#define gridder2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C.Liu & K. Tingdahl
 Date:		January 2008
 RCS:		$Id$
________________________________________________________________________

-*/


#include "algomod.h"
#include "mathfunc.h"
#include "factory.h"
#include "position.h"
#include "positionlist.h"

template<class T> class Interval;
class IOPar;
class DAGTriangleTree;
class Triangle2DInterpolator;


/*!
\ingroup Algo
\brief Generic interface for 2D gridding.
*/

mExpClass(Algo) Gridder2D
{
public:
			mDefineFactoryInClass(Gridder2D,factory);


    virtual		~Gridder2D()				{}
    virtual Gridder2D*	clone() const				= 0;
    virtual bool	operator==(const Gridder2D&) const;
			/*!Only checks the name. Should be
			   re-implemented in inheriting classed if they
			   have own variables/ settings. */

    virtual bool	wantsAllPoints() const { return true; }
			//!<If false, points should be tested with isPointUsable
    virtual bool	isPointUsable(const Coord& cpt,
	    			      const Coord& dpt) const;
			/*!Given that we want to calculate cpt, is data
			   at dpt usable. */

    virtual void	setGridArea(const Interval<float>&,
	    			    const Interval<float>&) {}
			/*!<Tells gridder that you will not call
			    setGridPoint outside these ranges. May speed
			    up calculation. */

    virtual bool	setPoints(const TypeSet<Coord>&);
    			/*<!Points are assumed to remain in mem through
			    init(). Points should correspond to the
			    values in setValues. Don't re-set it unless they
			    have changes, as it may lead to substantial
			    computations. */
    const TypeSet<Coord>* getPoints() const { return points_; }
    virtual bool	setGridPoint(const Coord&);
			/*!This is where we want to compute a value */
    virtual bool	init()					= 0;
    virtual bool	isPointUsed(int) const;
			/*!<Returns wether a certain point is used in
			    the interpolation. Can only be called after
			    successful init.*/
    virtual bool	setValues(const TypeSet<float>&,bool hasudfs);
			/*<!Values are assumed to remain in mem at
			    getValue(). Values should correspond to the
			    coords in setPoints*/
    virtual float	getValue() const;
			/*!<Does the gridding. Can only be run after a
			    successful init. */

    virtual void	fillPar(IOPar&) const		{}
    virtual bool	usePar(const IOPar&)		{ return true; }  


    virtual const TypeSet<int>&	usedValues() const	{ return usedvalues_; }
    				/*!<Returns which coordinates that are 
				    used in the gridding. Can only be called
				    after a successful init. */
    virtual const TypeSet<float>& weights() const	{ return weights_; }
    				/*!<Returns the weights for the coordinates that
				    are used in the gridding. The weights are 
				    linked with the usedValues(). Can only be
				    called after a successful init. */

protected:
				Gridder2D();
				Gridder2D(const Gridder2D&);

    bool			inited_;
    const TypeSet<float>*	values_;
    const TypeSet<Coord>*	points_;

    TypeSet<int>		usedvalues_;
    TypeSet<float>		weights_;

    Coord			gridpoint_;
};


/*!
\ingroup Algo
\brief Uses inverse distance method for 2D gridding.
*/

mExpClass(Algo) InverseDistanceGridder2D : public Gridder2D 
{
public:
    mDefaultFactoryInstantiation( Gridder2D,
				InverseDistanceGridder2D,
				"InverseDistance", "Inverse distance" );

		InverseDistanceGridder2D();
		InverseDistanceGridder2D(const InverseDistanceGridder2D&);

    Gridder2D*		clone() const;
    
    static const char*	sKeySearchRadius()	{ return "SearchRadius"; }

    bool		operator==(const Gridder2D&) const;

    void		setSearchRadius(float);
    float		getSearchRadius() const { return radius_; }

    bool		wantsAllPoints() const { return false; }
    bool		isPointUsable(const Coord&,const Coord&) const;
    bool		init();
    
    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

protected:

    float		radius_;
};


/*!
\ingroup Algo
\brief Uses Delaunay triangulation to find a points neighbors and does
inverse distance between the neighbors.
*/

mExpClass(Algo) TriangulatedGridder2D: public Gridder2D
{
public:
    mDefaultFactoryInstantiation( Gridder2D,
				TriangulatedGridder2D,
				"Triangulated", "Triangulation" );
    			TriangulatedGridder2D();
			TriangulatedGridder2D(
				const TriangulatedGridder2D&);
			~TriangulatedGridder2D();
    Gridder2D*		clone() const;
    

    bool		setPoints(const TypeSet<Coord>&);
    void		setGridArea(const Interval<float>&,
	    			    const Interval<float>&);

    bool		init();

protected:

    DAGTriangleTree*		triangles_;
    Triangle2DInterpolator*	interpolator_;
    Interval<float>		xrg_;
    Interval<float>		yrg_;

    Coord			center_;
};



#endif

