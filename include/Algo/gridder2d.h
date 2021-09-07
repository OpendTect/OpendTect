#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C.Liu & K. Tingdahl
 Date:		January 2008
 RCS:		$Id$
________________________________________________________________________

-*/


#include "algomod.h"
#include "arrayndalgo.h"
#include "mathfunc.h"
#include "factory.h"
#include "coord.h"
#include "positionlist.h"

class DAGTriangleTree;
template <class T> class LinSolver;
class Triangle2DInterpolator;


/*!
\brief Generic interface for 2D gridding.
*/

mExpClass(Algo) Gridder2D
{
public:
			mDefineFactoryInClass(Gridder2D,factory);


    virtual		~Gridder2D();
    virtual Gridder2D*	clone() const				= 0;
    virtual bool	operator==(const Gridder2D&) const;
			/*!Only checks the name and trend. Should be
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

    bool		setPoints(const TypeSet<Coord>&);
			/*<!Points are assumed to remain in mem through
			    getValue(). Points should correspond to the
			    values in setValues. Don't re-set it unless they
			    have changed, as it may lead to substantial
			    computations. */
    bool		setPoints(const TypeSet<Coord>&,TaskRunner*);
    const TypeSet<Coord>* getPoints() const { return points_; }
    virtual void	setTrend(PolyTrend::Order);
			/*<!Not a processing parameter, requires the input data
			  (points & values) to be set. Needs to be called again
			  each time either the values are changed */
    bool		setValues(const TypeSet<float>&);
			/*<!Values are assumed to remain in mem at
			    getValue(). Values should correspond to the
			    coords in setPoints */
    virtual float	getValue(const Coord&,const TypeSet<double>* weights=0,
				 const TypeSet<int>* relevantpoints=0) const;
			//!<Does the gridding
    const TypeSet<float>* getValues() const		{ return values_; }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    virtual bool	allPointsAreRelevant() const			   =0;

    virtual bool	getWeights(const Coord&,
				   TypeSet<double>& weights,
				   TypeSet<int>& relevantpoints) const	   =0;
				/*!<Only use this if multiple setValues()
				    are called for the same setPoints()
				    The output weights and pointset must then
				    be provided to the getValue function */
    virtual bool	areWeightsValuesDependent() const	{ return false;}

protected:
				Gridder2D();
				Gridder2D(const Gridder2D&);

    const TypeSet<float>*	values_;
    const TypeSet<Coord>*	points_;
    PolyTrend*			trend_;

    TypeSet<int>		usedpoints_;

    virtual bool		pointsChangedCB(CallBacker*)	{ return true; }
    virtual void		valuesChangedCB(CallBacker*)	{}
    float			getDetrendedValue(int idx) const;
				/*<!Input values corrected from the trend*/
    bool			isAtInputPos(const Coord&,int&idx) const;
};


/*!
\brief Uses inverse distance method for 2D gridding.
*/

mExpClass(Algo) InverseDistanceGridder2D : public Gridder2D
{ mODTextTranslationClass(InverseDistanceGridder2D);
public:
    mDefaultFactoryInstantiation( Gridder2D,
				InverseDistanceGridder2D,
				"InverseDistance", tr("Inverse distance") );

		InverseDistanceGridder2D();
		InverseDistanceGridder2D(const InverseDistanceGridder2D&);

    Gridder2D*		clone() const;

    static uiString	searchRadiusErrMsg();
    static const char*	sKeySearchRadius()	{ return "SearchRadius"; }

    bool		operator==(const Gridder2D&) const;

    void		setSearchRadius(float);
    float		getSearchRadius() const { return radius_; }

    bool		wantsAllPoints() const { return false; }
    bool		allPointsAreRelevant() const;
    bool		isPointUsable(const Coord&,const Coord&) const;
    bool		getWeights(const Coord&,TypeSet<double>& weights,
				   TypeSet<int>& relevantpoints) const;

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

protected:

    float		radius_;
};


/*!
\brief Uses Delaunay triangulation to find a points neighbors and does
inverse distance between the neighbors.
*/

mExpClass(Algo) TriangulatedGridder2D : public Gridder2D
{ mODTextTranslationClass(TriangulatedGridder2D);
public:
    mDefaultFactoryInstantiation( Gridder2D,
				TriangulatedGridder2D,
				"Triangulated", tr("Triangulation") );

			TriangulatedGridder2D();
			TriangulatedGridder2D(
				const TriangulatedGridder2D&);
			~TriangulatedGridder2D();
    Gridder2D*		clone() const;


    void		setGridArea(const Interval<float>&,
				    const Interval<float>&);

    bool		allPointsAreRelevant() const;
    bool		getWeights(const Coord&,TypeSet<double>& weights,
				   TypeSet<int>& relevantpoints) const;

protected:

    DAGTriangleTree*		triangles_;
    Triangle2DInterpolator*	interpolator_;
    Interval<float>		xrg_;
    Interval<float>		yrg_;

    Coord			center_;

    bool		pointsChangedCB(CallBacker*);
};



/*!
\brief Uses Radial Basic Function to predict the values
*/

mExpClass(Algo) RadialBasisFunctionGridder2D : public Gridder2D
{ mODTextTranslationClass(RadialBasisFunctionGridder2D)
public:
    mDefaultFactoryInstantiation( Gridder2D,
				RadialBasisFunctionGridder2D,
				"RadialBasicFunction",
				tr("Radial Basis Function") );

			RadialBasisFunctionGridder2D();
			RadialBasisFunctionGridder2D(
				const RadialBasisFunctionGridder2D&);
			~RadialBasisFunctionGridder2D();
    Gridder2D*		clone() const;

    static const char*	sKeyIsMetric()		{ return "IsMetric"; }

    bool		operator==(const Gridder2D&) const;

    void		setMetricTensor(double m11,double m12,double m22);

    bool		allPointsAreRelevant() const;
    bool		getWeights(const Coord&,TypeSet<double>& weights,
				   TypeSet<int>& relevantpoints) const;
    bool		areWeightsValuesDependent() const      { return true; }
    float		getValue(const Coord&,const TypeSet<double>* weights=0,
				 const TypeSet<int>* relevantpoints=0) const;

protected:

    bool		ismetric_;
    double		m11_;
    double		m12_;
    double		m22_;

    TypeSet<double>*	globalweights_;
    LinSolver<double>*	solv_;

    bool		updateSolver();
			//will be removed after 6.2

    bool		updateSolver(TaskRunner*);
    bool		updateSolution();
    double		getRadius(const Coord& pos1,const Coord& pos2) const;
    static double	evaluateRBF(double radius,double scale=1.);

    bool		pointsChangedCB(CallBacker*);
    void		valuesChangedCB(CallBacker*);
    bool		setWeights(const Coord&,TypeSet<double>& weights,
				   TypeSet<int>* usedpoints=0) const;
};

