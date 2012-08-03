#ifndef polygonsurface_h
#define polygonsurface_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Y.C. Liu
Date:          July 2008
RCS:           $Id: polygonsurface.h,v 1.15 2012-08-03 13:00:28 cvskris Exp $
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "refcount.h"
#include "rowcolsurface.h"

namespace Geometry
{
/*<!Simple geometry for polygonsurface, for polygon plane, only three 
    directions are defined, i.e. along XY, XZ or YZ planes. So normals are 
    given by Coord3(0,0,1), Coord3(0,1,0) and Coord3(1,0,0) respectively.
    For Rcol variable rc, rc.r() represents polygonidx, rc.c() represents the 
    knots on the corresponding polygon. */
 
mClass(Geometry) PolygonSurface : public RowColSurface
{
public:
    			PolygonSurface();
    			~PolygonSurface();
    bool		isEmpty() const		{ return !polygons_.size(); }
    Element*		clone() const;
    enum ChangeTag      {PolygonChange=__mUndefIntVal+1,PolygonInsert,
			 PolygonRemove};

    bool		insertPolygon(const Coord3& firstpos,
	    			      const Coord3& normal,int polygon=0,
				      int firstknot=0);
    bool		removePolygon(int polygon);

    bool		insertKnot(const RowCol&,const Coord3&);
    bool		removeKnot(const RowCol&);

    int			nrPolygons() const { return polygons_.size(); }
    StepInterval<int>	rowRange() const;
    virtual StepInterval<int> colRange() const
			{ return RowColSurface::colRange(); }
    StepInterval<int>	colRange(int polygon) const;
    
    bool		setKnot(const RowCol&,const Coord3&);
    Coord3		getKnot(const RowCol&) const;
    bool		isKnotDefined(const RowCol&) const;

    void		setBezierCurveSmoothness(int nrpoints_on_segment);
    int			getBezierCurveSmoothness() const {return beziernrpts_;}
    void		getCubicBezierCurve(int plg,TypeSet<Coord3>& pts,
					    const float zscale) const;
    			/*<The Bezier Curve smoothes the polygon, nrknotsinsert
			   on each edge will affect the smoothness. */
    void		getAllKnots(TypeSet<Coord3>&) const;
			/*<Only get all the picked positions on the surface. */

    const Coord3&	getPolygonNormal(int polygonnr) const;
    const Coord3&	getPolygonConcaveDir(int polygonnr) const;

    void		getExceptionEdges(int plg,TypeSet<int>& edges) const;   
    void		getPolygonConcaveTriangles(int plg,TypeSet<int>&) const;
    			/*<the TypeSet has all the triangles which are concave 
			relative to the polygon. ret v0, v1, v2 the 1st 
			triangle; v3, v4, v5 the 2nd. */

    void		addUdfPolygon(int polygnr,int firstknotnr,int nrknots);
    void		addEditPlaneNormal(const Coord3& normal);

    char		bodyDimension() const;
    			/*<Return dim==3, it is a real body, otherwise, 
			   dim==2 a surface, dim==1 a line, dim==0 a point.*/

protected:

    bool			linesegmentsIntersecting(const Coord3& v0, 
	    				const Coord3& v1,const Coord3& p0,
					const Coord3& p1) const;
    bool			includesEdge(const TypeSet<int> edges,
	    				     int v0,int v1) const;
    int				firstpolygon_;
    TypeSet<int>		firstknots_;
    ObjectSet<TypeSet<Coord3> >	polygons_;
    				/*<assume each polygon is made by connecting 
				   the pts in order. */
    TypeSet<Coord3>		polygonnormals_;
    TypeSet<Coord3>		concavedirs_;
    int				beziernrpts_;
};

};

#endif

