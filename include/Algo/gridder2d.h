#ifndef gridder2d_h
#define gridder2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Y.C.Liu & K. Tingdahl
 Date:		January 2008
 RCS:		$Id: gridder2d.h,v 1.12 2009-05-01 13:46:33 cvskris Exp $
________________________________________________________________________

-*/


#include "mathfunc.h"
#include "factory.h"
#include "position.h"
#include "positionlist.h"

template<class T> class Interval;
class IOPar;
class DAGTriangleTree;

/*! Generic interface for 2D gridding. */

mClass Gridder2D
{
public:
			mDefineFactoryInClass(Gridder2D,factory);


    virtual		~Gridder2D()				{}
    virtual const char*	name() const				= 0;
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
			    values in setValues */
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


mClass InverseDistanceGridder2D : public Gridder2D 
{
public:
		InverseDistanceGridder2D();
		InverseDistanceGridder2D(const InverseDistanceGridder2D&);

    static Gridder2D*	create();
    static void		initClass(); 
    Gridder2D*		clone() const;
    
    const char* 	name() const		{ return sName(); }
    static const char* 	sName() 		{ return "InverseDistance"; }
    static const char* 	sUserName() 		{ return "Inverse distance"; }
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


/*!Uses Delaunay triangulation to find a points neighbors and does inverse
   distance between the neighbors. */
mClass TriangulatedNeighborhoodGridder2D : public Gridder2D
{
public:
    			TriangulatedNeighborhoodGridder2D();
    			TriangulatedNeighborhoodGridder2D(
				const TriangulatedNeighborhoodGridder2D&);
			~TriangulatedNeighborhoodGridder2D();

    static const char* 	sName() 	{ return "TriangulatedNeighborhood"; }
    static const char* 	sUserName() 	{ return "Triangulated Neighborhood";}

    bool		setPoints(const TypeSet<Coord>&);

    void		setGridArea(const Interval<float>&,
	    			    const Interval<float>&);

    static Gridder2D*	create(); 
    static void		initClass();
    const char*		name() const		{ return sName(); }
    Gridder2D*		clone() const;
    
    bool		init();
protected:

    TypeSet<Coord>	mycoords_;
    DAGTriangleTree*	triangles_;
    Interval<float>	xrg_;
    Interval<float>	yrg_;
};


/*!Use Delaunay triangulation to triangulate all points, including the point
  of investigation, and use inverse distance among the point of investigation's
  neighborhood. */
mClass TriangulatedGridder2D: public Gridder2D
{
public:
    			TriangulatedGridder2D();
			TriangulatedGridder2D(
				const TriangulatedGridder2D&);
			~TriangulatedGridder2D();

    static const char* 	sName() 		{ return "Triangulated"; }
    static const char* 	sUserName() 		{ return "Triangulation"; }

    void		setGridArea(const Interval<float>&,
	    			    const Interval<float>&);

    static Gridder2D*	create(); 
    static void		initClass();
    const char*		name() const	{ return sName(); }
    Gridder2D*		clone() const;
    
    bool		init();

protected:

    DAGTriangleTree*	triangles_;
    Interval<float>	xrg_;
    Interval<float>	yrg_;
    TypeSet<int>	addedindices_;
};



#endif
