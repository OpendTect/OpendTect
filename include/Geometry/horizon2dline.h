#ifndef horizon2dline_h
#define horizon2dline_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl
Date:          March 2006
RCS:           $Id$
________________________________________________________________________

-*/

#include "rowcolsurface.h"
#include "samplingdata.h"
#include "surv2dgeom.h"

class Plane3;
class RowCol;

namespace PosInfo { class Line2DData; }

namespace Geometry
{

/*!A curve that goes along a fixed set of x,y coordinates with a varying
   z. */

mClass Horizon2DLine : public RowColSurface
{
public:
    			Horizon2DLine();
    			Horizon2DLine(int lineid,const TypeSet<Coord>&,
				      int start,int step);
    			Horizon2DLine(const Horizon2DLine&);
    			~Horizon2DLine();

    Horizon2DLine*	clone() const;
    bool		isEmpty() const { return rows_.isEmpty(); }

    bool		addRow(const PosInfo::GeomID&,const TypeSet<Coord>&,
	    		       int start,int step);
    			/*!<\returns id of new path. */
    bool		addUdfRow(const PosInfo::GeomID&,int start,int stop,
	    			  int step);
    			/*!<\returns id of new path. */
    void		setRow(const PosInfo::GeomID&,const TypeSet<Coord>&,
	    		       int start,int step);
    bool		reassignRow(const PosInfo::GeomID& from,
	    			    const PosInfo::GeomID& to);
    void		syncRow(const PosInfo::GeomID&,
	    			const PosInfo::Line2DData&);
    void		removeRow(const PosInfo::GeomID&);
    void		removeCols(const PosInfo::GeomID&,int start,int stop);

    int 		getRowIndex(const PosInfo::GeomID&) const;

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange(int rowindex) const;
    StepInterval<int>	colRange(const PosInfo::GeomID&) const;
    virtual StepInterval<int> colRange() const
			{ return RowColSurface::colRange(); }
    Interval<float>	zRange(const PosInfo::GeomID&) const;

    void		geometry(const PosInfo::GeomID&,
	    			 PosInfo::Line2DData&) const;

    Coord3		getKnot(const RowCol& rc) const; // rc.row = rowindex
    bool		setKnot(const RowCol&,const Coord3&);
    bool		isKnotDefined(const RowCol&) const;
    Coord3		computePosition(const PosInfo::GeomID&,int trcnr) const;

    virtual void	trimUndefParts();
    bool		hasSupport(const RowCol&) const;
    void		checkSupport(bool yn)	{ checksupport_ = yn; }
    bool		checksSupport() const	{ return checksupport_; }

protected:
    int			colIndex(int rowidx,int colid) const;

    bool		checksupport_;
    int			firstrow_;

    ObjectSet<TypeSet<Coord3> >	rows_;
    TypeSet<SamplingData<int> >	colsampling_;
    TypeSet<PosInfo::GeomID>	geomids_;
};

} // namespace Geometry

#endif
