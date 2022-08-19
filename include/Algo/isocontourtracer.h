#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "arraynd.h"
#include "arrayndimpl.h"
#include "ranges.h"
#include "polygon.h"

/*!
\brief Tracing the z-level contour of a 2D scalar field.
*/

mExpClass(Algo) IsoContourTracer
{
public:
		
		IsoContourTracer(const Array2D<float>&);
		~IsoContourTracer();

    void	setSampling(const StepInterval<int>& xsamp,
	    		    const StepInterval<int>& ysamp);
    void	selectRectROI(const Interval<int>& xintv,
			      const Interval<int>& yintv);
    void	selectPolyROI(const ODPolygon<float>*);

    void	setBendPointsOnly(float eps);
    void	setMinNrVertices(int);
    void	setNrLargestOnly(int);
    void	setEdgeValue(float); //! To close contours along (RectROI) edge

    bool	getContours(ObjectSet<ODPolygon<float> >&,
	    		    float z,bool closedonly=false) const;

protected:

    void			findCrossings(Array3DImpl<float>& crossings,
					      float z) const;

    void			traceContours(Array3DImpl<float>& crossings,
					      ObjectSet<ODPolygon<float> >&,
					      bool closedonly) const;

    void			addVertex(ODPolygon<float>& contour,
					  bool headinsert,int idx,int idy,
					  int hor,float frac) const;
    
    const Array2D<float>&	field_;

    StepInterval<int>		xsampling_;
    StepInterval<int>		ysampling_;
    Interval<int>		xrange_;
    Interval<int>		yrange_;

    int				minnrvertices_;
    int				nrlargestonly_;
    float			edgevalue_;
    float			bendpointeps_;
    unsigned int		edgepar_;

    const ODPolygon<float>*	polyroi_;
};
