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
namespace Survey { class Geometry; class Geometry2D; class Geometry3D; }


namespace Survey
{

mExpClass(Basic) SubGeometry
{
public:

    virtual const Geometry*	surveyGeometry() const		= 0;
    virtual SubGeometry*	clone() const			= 0;

    virtual bool		isFull() const			= 0;
    virtual bool		hasOffset() const		= 0;
    virtual bool		isSubSpaced() const		= 0;

};

/*!\brief Subselection of a 3D Geometry

  The subselection has 3 parts:
  * An offset from the 3D Geometry origin in rows/cols
  * A step in rows/cols (i.e. not in inlines/crosslines)
  * The total number of rows/cols

 */

mExpClass(Basic) SubGeometry3D : public SubGeometry
{
public:

    typedef BinID::IdxType		pos_idx_type;
    typedef pos_idx_type		idx_type;
    typedef idx_type			size_type;
    typedef StepInterval<pos_idx_type>	pos_idx_range_type;

			SubGeometry3D();	//!< default 3D geom
			SubGeometry3D(const Geometry3D&);
			SubGeometry3D(const SubGeometry3D&);
    SubGeometry3D&	operator =(const SubGeometry3D&);
    SubGeometry*	clone() const		override;
    const Geometry*	surveyGeometry() const	override;

    const Geometry3D*	surveyGeometry3D() const { return survgeom_; }
    RowCol		offset() const		{ return offs_; }
    RowCol		step() const		{ return step_; }
    RowCol		size() const		{ return sz_; }

    BinID		origin() const;
    size_type		nrRows() const		{ return sz_.row(); }
    size_type		nrCols() const		{ return sz_.col(); }

    idx_type		idx4Inl(pos_idx_type) const;
    idx_type		idx4Crl(pos_idx_type) const;
    pos_idx_type	inl4Idx(idx_type) const;
    pos_idx_type	crl4Idx(idx_type) const;
    RowCol		idxs4BinID(const BinID&) const;
    BinID		binid4Idxs(idx_type irow,idx_type icol) const;
    BinID		binid4Idxs(const RowCol&) const;

    pos_idx_type	inlStart() const;
    pos_idx_type	crlStart() const;
    pos_idx_type	inlStep() const;
    pos_idx_type	crlStep() const;
    pos_idx_type	inlStop() const;
    pos_idx_type	crlStop() const;
    pos_idx_range_type	inlRange() const;
    pos_idx_range_type	crlRange() const;
    pos_idx_range_type	survInlRange() const;
    pos_idx_range_type	survCrlRange() const;

    void		setSurvGeom( const Geometry3D& geom )
				{ survgeom_ = &geom; }
    void		setRange(const BinID& start,const BinID& stop,
				 RowCol substeps=RowCol(1,1));
    void		setRange(const BinID& start,const BinID& stop,
				 const BinID& step);

    bool		isFull() const		override;
    bool		hasOffset() const	override;
    bool		isSubSpaced() const	override;
    bool		hasFullArea() const;

protected:

    const Geometry3D*	survgeom_;
    RowCol		offs_;
    RowCol		step_;
    RowCol		sz_;

};

} // namespace Survey
