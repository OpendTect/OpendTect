#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2019
________________________________________________________________________

-*/

#include "posinfo.h"
class CubeHorSubSel;


namespace PosInfo
{

typedef LineCollDataPos CubeDataPos;
typedef LineCollDataIterator CubeDataIterator;
class LinesData;


/*!\brief LineCollData for a 3D cube. The LineData's are not automatically
  sorted. */

mExpClass(Basic) CubeData : public LineCollData
{
public:

			CubeData()		{}
			CubeData( BinID start, BinID stop, BinID step )
						{ generate(start,stop,step); }
			CubeData(pos_steprg_type,pos_steprg_type);
			CubeData( const CubeData& oth )
						{ copyContents(oth); }
    explicit		CubeData( const LineCollData& oth )
						{ copyContents(oth); }
			CubeData(const LinesData&)	= delete;

    bool		isCubeData() const override { return true; }
    bool		isFullyRegular() const override;
    CubeData*		clone() const override	{ return new CubeData(*this); }
    CubeData&		operator =( const CubeData& oth )
			{ copyContents(oth); return *this; }
    CubeData&		operator =( const LineCollData& lcd )
			{ copyContents(lcd); return *this; }
    bool		operator ==( const CubeData& oth ) const
			{ return LineCollData::operator ==( oth ); }
    bool		operator ==( const LineCollData& oth ) const
			{ return LineCollData::operator ==( oth ); }
			mImplSimpleIneqOper(CubeData)
			mImplSimpleIneqOper(LineCollData)

    bool		includes(const Bin2D&) const	    = delete;
    Bin2D		bin2D(const LineCollDataPos&) const = delete;
    LineCollDataPos	lineCollPos(const Bin2D&) const	    = delete;

    glob_size_type	totalSizeInside(const CubeHorSubSel&) const;
				/*!<Only count positions that are inside */
    bool		hasPosition(const CubeHorSubSel&,glob_idx_type) const;

    void		getRanges(pos_rg_type& inl,pos_rg_type& crl) const;
    bool		getInlRange(pos_steprg_type&,bool sorted=true) const;
			    //!< Returns whether fully regular.
    bool		getCrlRange(pos_steprg_type&,bool sorted=true) const;
			    //!< Returns whether fully regular.
    bool		getCubeHorSubSel(CubeHorSubSel&) const;
			    //!< Returns whether fully regular.
    void		getCubeFullHorSubSel( FullHorSubSel& fss ) const
			{ return getFullHorSubSel( fss, false ); }

    BinID		minStep() const;
    BinID		nearestBinID(const BinID&, idx_type maxoffset=2) const;
    BinID		centerPos() const;  //!< not exact

    bool		includes( const BinID& bid ) const
                        { return LineCollData::includes(bid); }
    LineCollDataPos	lineCollPos( const BinID& bid ) const
                        { return LineCollData::lineCollPos(bid); }
    CubeDataPos		cubeDataPos( const BinID& bid ) const
			{ return LineCollData::lineCollPos(bid); }

    bool		haveInlStepInfo() const	{ return size() > 1; }
    bool		haveCrlStepInfo() const;
    bool		isAll(const CubeHorSubSel&) const;
    bool		isCrlReversed() const;

    void		generate(BinID start,BinID stop,BinID step,
				 bool allowreversed=false);
    void		fillBySI(OD::SurvLimitType slt=OD::FullSurvey);

};


/*!\brief CubeData in which the LineData's added are automatically sorted. */

mExpClass(Basic) SortedCubeData : public CubeData
{
public:
			SortedCubeData()				{}
			SortedCubeData( const BinID& start, const BinID& stop,
					const BinID& step )
						{ generate(start,stop,step); }
			SortedCubeData( const SortedCubeData& oth )
						{ copyContents( oth ); }
			SortedCubeData( const LineCollData& lcd )
						{ copyContents(lcd); }
    bool		isLineSorted() const override	{ return true; }
    SortedCubeData*	clone() const override
			{ return new SortedCubeData(*this); }
    SortedCubeData&	operator =( const SortedCubeData& scd )
			{ copyContents(scd); return *this; }
    SortedCubeData&	operator =( const LineCollData& lcd )
			{ copyContents(lcd); return *this; }

    virtual idx_type	lineIndexOf(pos_type inl,idx_type* newidx=0) const;

    SortedCubeData&	add(LineData*);

protected:

    virtual SortedCubeData& doAdd(LineData*) override;

};


} // namespace PosInfo
