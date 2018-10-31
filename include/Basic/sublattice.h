#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2018
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"
#include "rowcol.h"
#include "ranges.h"
namespace Survey { class Geometry3D; }


/*!\brief Subselection of a Lattcie (3D Geometry) */

mExpClass(Basic) SubLattice
{
public:

    typedef Survey::Geometry3D	Geometry3D;
    typedef StepInterval<int>	IdxRange;

			SubLattice();	//!< default 3D geom
			SubLattice(const Geometry3D&);
			SubLattice(const SubLattice&);
    SubLattice&		operator =(const SubLattice&);

    const Geometry3D*	survGeom() const    { return survgeom_; }
    RowCol		offset() const	    { return offs_; }
    RowCol		step() const	    { return step_; }
    RowCol		size() const	    { return sz_; }

    bool		isFull() const;
    bool		hasOffset() const;
    bool		isSubSpaced() const;
    bool		hasFullArea() const;

    int			nrRows() const	    { return sz_.row(); }
    int			nrCols() const	    { return sz_.col(); }
    BinID		origin() const;
    RowCol		getIndexes(const BinID&) const;
    BinID		getBinID(int irow,int icol) const;
    BinID		getBinID(const RowCol&) const;

    int			getRow(int inln) const;
    int			getCol(int crln) const;
    int			getInl(int irow) const;
    int			getCrl(int icol) const;
    int			inlStart() const;
    int			crlStart() const;
    int			inlStep() const;
    int			crlStep() const;
    int			inlStop() const;
    int			crlStop() const;
    IdxRange		inlRange() const;
    IdxRange		crlRange() const;
    IdxRange		survInlRange() const;
    IdxRange		survCrlRange() const;

    void		setSurvGeom( const Geometry3D& geom )
				{ survgeom_ = &geom; }
    void		setRange(const BinID& start,const BinID& stop,
				 RowCol substeps=RowCol(1,1));

protected:

    const Geometry3D*	survgeom_;
    RowCol		offs_;
    RowCol		step_;
    RowCol		sz_;

};
