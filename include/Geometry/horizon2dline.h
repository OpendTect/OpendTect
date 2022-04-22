#pragma once

/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl
Date:	       March 2006
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "rowcolsurface.h"
#include "samplingdata.h"
#include "posinfo2dsurv.h"

class Plane3;
class RowCol;

namespace PosInfo { class Line2DData; }

namespace Geometry
{

/*!A curve that goes along a fixed set of x,y coordinates with a varying
   z. */

mExpClass(Geometry) Horizon2DLine : public RowColSurface
{
public:
			Horizon2DLine();
			Horizon2DLine(int lineid,const TypeSet<Coord>&,
				      int start,int step);
			Horizon2DLine(const Horizon2DLine&);
			~Horizon2DLine();

    Horizon2DLine*	clone() const override;
    bool		isEmpty() const override { return rows_.isEmpty(); }

    int			nrLines() const		 { return geomids_.size(); }
    Pos::GeomID		geomID(int rowidx) const { return geomids_[rowidx]; }
    void		getGeomIDs(TypeSet<Pos::GeomID>&) const;
    bool		hasLine(Pos::GeomID) const;

    bool		addRow(Pos::GeomID,const TypeSet<Coord>&,
			       int start,int step);
			/*!<\returns id of new path. */
    bool		addUdfRow(Pos::GeomID,int start,int stop,int step);
			/*!<\returns id of new path. */

    void		setRow(Pos::GeomID,const StepInterval<int>* trcrg);
    void		setRow(Pos::GeomID,const TypeSet<Coord>&,
			       int start,int step);
    bool		reassignRow(Pos::GeomID from,Pos::GeomID to);

    void		syncRow(Pos::GeomID,
				const PosInfo::Line2DData&);

    void		removeRow(Pos::GeomID);

    void		removeCols(Pos::GeomID,int start,int stop);

    int			getRowIndex(Pos::GeomID) const;

    StepInterval<int>	rowRange() const override;
    StepInterval<int>	colRange(int rowindex) const override;
    StepInterval<int>	colRangeForGeomID(Pos::GeomID) const;
    StepInterval<int>	colRange() const override
			{ return RowColSurface::colRange(); }
    Interval<float>	zRange(Pos::GeomID) const;

    void		geometry(Pos::GeomID,PosInfo::Line2DData&)const;

    Coord3		getKnot(const RowCol& rc) const override;
			// rc.row() = rowindex

    bool		setKnot(const RowCol&,const Coord3&) override;

    bool		isKnotDefined(const RowCol&) const override;
    Coord3		computePosition(Pos::GeomID,int trcnr) const;

    void		trimUndefParts() override;
    bool		hasSupport(const RowCol&) const;
    void		checkSupport(bool yn)	{ checksupport_ = yn; }
    bool		checksSupport() const	{ return checksupport_; }

    bool		setPosition(GeomPosID pid,const Coord3& pos) override;
    Coord3		getPosition(GeomPosID pid) const override;
    bool		isDefined(GeomPosID pid) const override;

protected:

    int			colIndex(int rowidx,int colid) const;
    int			rowIndex(int rowid) const;

    bool		checksupport_;

    ObjectSet<TypeSet<Coord3> > rows_;
    TypeSet<SamplingData<int> > colsampling_;
    TypeSet<Pos::GeomID>	geomids_;
};

} // namespace Geometry

