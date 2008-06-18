#ifndef polygonsurface_h
#define polygonsurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl / J.C. Glas
Date:          September 2007
RCS:           $Id: polygonsurface.h,v 1.1 2008-06-18 18:24:47 cvskris Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "rowcolsurface.h"

namespace Geometry
{

class PolygonSurface : public RowColSurface
{
public:
    			PolygonSurface();
    			~PolygonSurface();
    bool		isEmpty() const		{ return !polygons_.size(); }
    Element*		clone() const;

    enum		Dimension { XY, XZ, YZ };

    bool		insertPolygon(const Coord3& firstpos,
	    			      Dimension,int polygon=0);
    bool		removePolygon(int polygon);

    bool		insertKnot(const RCol&,const Coord3&);
    bool		removeKnot(const RCol&);

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange(int polygon) const;

    void		getPolygon(int polygon,TypeSet<Coord>& pts,
	    			   BoolTypeSet& isdefpt);

    bool		setKnot(const RCol&,const Coord3&);
    Coord3		getKnot(const RCol&) const;
    bool		isKnotDefined(const RCol&) const;

    const Coord3&	getPolygonNormal(int polygon) const;				
    Dimension		getPolygonPlane(int polygon) const;

    			// To be used by surface reader only
    void		addUdfRow(int stickidx,int firstknotnr,int nrknots);
    void		addEditPlaneNormal(const Coord3&);

protected:

    int				firstrow_;

    ObjectSet<TypeSet<Coord3> >	polygons_;
    TypeSet<int>		firstcols_;
    
    TypeSet<Dimension>		polygonplanes_;
};

};

#endif
