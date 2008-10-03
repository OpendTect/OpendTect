#ifndef isocontourtracer_h
#define isocontourtracer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	J.C. Glas
 Date:		November 2007	
 RCS:		$Id: isocontourtracer.h,v 1.2 2008-10-03 12:22:42 cvsjaap Exp $
________________________________________________________________________

-*/

#include "arraynd.h"
#include "arrayndimpl.h"
#include "ranges.h"
#include "polygon.h"

/*!\brief Tracing the z-level contour of a 2D scalar field. */

class IsoContourTracer
{
public:
		
		IsoContourTracer(const Array2D<float>&);
		~IsoContourTracer()					{}

    void	setSampling(const StepInterval<int>& xsamp,
	    		    const StepInterval<int>& ysamp);
    void	selectRectROI(const Interval<int>& xintv,
			      const Interval<int>& yintv);
    void	selectPolyROI(const ODPolygon<float>*); 

    void	setMinNrVertices(int);
    void	setNrLargestOnly(int);

    bool	getContours(ObjectSet<ODPolygon<float> >&,
	    		    float z,bool closedonly=false) const;

protected:

    void			findCrossings( Array3DImpl<float>& crossings,
	   				       float z ) const;

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

    const ODPolygon<float>*	polyroi_;
};


#endif
