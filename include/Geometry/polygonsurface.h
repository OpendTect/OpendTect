#ifndef polygonsurface_h
#define polygonsurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        Y.C. Liu
Date:          July 2008
RCS:           $Id: polygonsurface.h,v 1.3 2008-09-05 21:39:31 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "rowcolsurface.h"

namespace Geometry
{
/*<!Simple geometry for polygonsurface, for polygon plane, only three 
    directions are defined, i.e. along XY, XZ or YZ planes. So normals are 
    given by Coord3(0,0,1), Coord3(0,1,0) and Coord3(1,0,0) respectively.
    For Rcol variable rc, rc.r() represents polygonidx, rc.c() represents the 
    knots on the corresponding polygon. */
 
class PolygonSurface : public RowColSurface
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

    bool		insertKnot(const RCol&,const Coord3&);
    bool		removeKnot(const RCol&);

    int			nrPolygons() const { return polygons_.size(); }
    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange(int polygon) const;
    
    bool		setKnot(const RCol&,const Coord3&);
    Coord3		getKnot(const RCol&) const;
    bool		isKnotDefined(const RCol&) const;

    bool		getPolygonCrds(int polygon,TypeSet<Coord3>& pts) const;
    bool		getSurfaceCrds(TypeSet<Coord3>& pts) const;
    const Coord3&	getPolygonNormal(int polygonnr) const;
    
    void		getPolygonConcaveTriangles(int plg,TypeSet<int>&) const;
    			/*<the TypeSet has all the triangles which are concave 
			relative to the polygon. ret v0, v1, v2 the 1st 
			triangle; v3, v4, v5 the 2nd. */
    void		getNonintersectConcaveTris(int plg,TypeSet<int>&) const;			/*<ConcaveTriangles without selfintersecting. */

    void		addUdfPolygon(int polygnr,int firstknotnr,int nrknots);
    void		addEditPlaneNormal(const Coord3& normal);

protected:

    bool			linesegmentsIntersecting(const Coord3& v0, 
	    				const Coord3& v1,const Coord3& p0,
					const Coord3& p1) const;
    int				firstpolygon_;
    TypeSet<int>		firstknots_;
    ObjectSet<TypeSet<Coord3> >	polygons_;
    				/*<assume each polygon is made by connecting 
				   the pts in order. */
    TypeSet<Coord3>		polygonnormals_;
};

};

#endif
