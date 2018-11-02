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

mExpClass(Basic) SubGeometryHDimData
{
public:

    typedef Pos::IdxPair::IdxType	pos_type;
    typedef pos_type			idx_type;
    typedef idx_type			size_type;
    typedef StepInterval<pos_type>	pos_range_type;

		SubGeometryHDimData(const pos_range_type&);

    bool	operator ==(const SubGeometryHDimData&) const;

    bool	isEmpty() const		{ return sz_ < 1; }
    bool	isFull() const		{ return posRange() == *fullposrg_; }
    bool	hasOffset() const	{ return offs_ != 0; }
    bool	isSubSpaced() const	{ return step_ > 1; }
    bool	hasFullRange() const;

    idx_type	offset() const		{ return offs_; }
    idx_type	step() const		{ return step_; }
    idx_type	size() const		{ return sz_; }

    idx_type	idx4Pos(pos_type) const;
    pos_type	pos4Idx(idx_type) const;

    pos_type	posStart() const;
    pos_type	posStop() const;
    pos_type	posStep() const;

    pos_range_type posRange() const;
    pos_range_type fullPosRange() const	{ return *fullposrg_; }

    void	setPosRange(pos_type start,pos_type stop,pos_type stp);
    void	setFullPosRange(const pos_range_type&);

protected:

    idx_type			offs_			= 0;
    idx_type			step_			= 1;
    size_type			sz_;
    const pos_range_type*	fullposrg_;

};


mExpClass(Basic) SubGeometry
{
public:

    typedef SubGeometryHDimData::pos_type		pos_type;
    typedef SubGeometryHDimData::idx_type		idx_type;
    typedef SubGeometryHDimData::size_type		size_type;
    typedef SubGeometryHDimData::pos_range_type		pos_range_type;

    virtual const Geometry*	surveyGeometry() const		= 0;
    virtual SubGeometry*	clone() const			= 0;
    virtual void		setSurvGeom(const Geometry&)	= 0;

    virtual bool	isEmpty() const		{ return trcdd_.isEmpty(); }
    virtual bool	isFull() const		{ return trcdd_.isFull(); }
    virtual bool	hasOffset() const	{ return trcdd_.hasOffset(); }
    virtual bool	isSubSpaced() const	{ return trcdd_.isSubSpaced(); }
    virtual bool	hasFullRange() const	{ return trcdd_.hasFullRange();}

    SubGeometryHDimData&	trcNrDimData()		{ return trcdd_; }
    const SubGeometryHDimData&	trcNrDimData() const	{ return trcdd_; }


protected:

			SubGeometry(const Geometry&);
    bool		operator ==(const SubGeometry&) const;

    SubGeometryHDimData	trcdd_;

};


/*!\brief Subselection of a 3D Geometry

  The subselection has 3 parts:
  * An offset from the 3D Geometry origin in rows/cols
  * A step in the available geometry in rows/cols (i.e. not in inl/crl)
  * The total number of rows/cols

 */

mExpClass(Basic) SubGeometry3D : public SubGeometry
{
public:

			SubGeometry3D();	//!< default 3D geom
			SubGeometry3D(const Geometry3D&);
    bool		operator ==(const SubGeometry3D&) const;
    SubGeometry*	clone() const		override;
    const Geometry*	surveyGeometry() const	override;

    const Geometry3D*	surveyGeometry3D() const { return survgeom_; }

    RowCol		offset() const;
    RowCol		step() const;
    RowCol		size() const;

    BinID		origin() const;
    size_type		nrRows() const;
    size_type		nrCols() const;

    idx_type		idx4Inl(pos_type) const;
    idx_type		idx4Crl(pos_type) const;
    pos_type		inl4Idx(idx_type) const;
    pos_type		crl4Idx(idx_type) const;
    RowCol		idxs4BinID(const BinID&) const;
    BinID		binid4Idxs(idx_type irow,idx_type icol) const;
    BinID		binid4Idxs(const RowCol&) const;

    pos_type		inlStart() const	{ return inldd_.posStart(); }
    pos_type		crlStart() const	{ return trcdd_.posStart(); }
    pos_type		inlStop() const		{ return inldd_.posStop(); }
    pos_type		crlStop() const		{ return trcdd_.posStop(); }
    pos_type		inlStep() const		{ return inldd_.posStep(); }
    pos_type		crlStep() const		{ return trcdd_.posStep(); }
    pos_range_type	inlRange() const	{ return inldd_.posRange(); }
    pos_range_type	crlRange() const	{ return trcdd_.posRange(); }
    pos_range_type	survInlRange() const;
    pos_range_type	survCrlRange() const;

    void		setRange(const BinID& start,const BinID& stop,
				 RowCol substeps=RowCol(1,1));
    void		setRange(const BinID& start,const BinID& stop,
				 const BinID& step);

    void		setSurvGeom(const Geometry&)	override;
    bool		isEmpty() const			override;
    bool		isFull() const			override;
    bool		hasOffset() const		override;
    bool		isSubSpaced() const		override;
    bool		hasFullRange() const		override;

    SubGeometryHDimData&	inlDimData()		{ return inldd_; }
    const SubGeometryHDimData&	inlDimData() const	{ return inldd_; }
    SubGeometryHDimData&	crlDimData()		{ return trcdd_; }
    const SubGeometryHDimData&	crlDimData() const	{ return trcdd_; }

protected:

    const Geometry3D*	survgeom_;
    SubGeometryHDimData	inldd_;

};

} // namespace Survey
