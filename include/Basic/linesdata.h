#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2019
________________________________________________________________________

-*/

#include "posinfo.h"
class LineHorSubSelSet;


namespace PosInfo
{

typedef LineCollDataPos LinesDataPos;
typedef LineCollDataIterator LinesDataIterator;
class CubeData;

/*!\brief Position info for a collection of 2D lines */

mExpClass(Basic) LinesData : public LineCollData
{
public:

    mUseType( Pos,	GeomID );

			LinesData()		{}
    explicit		LinesData( GeomID gid )	{ setTo( gid ); }
    explicit		LinesData( const GeomIDSet& gids )
						{ setTo(gids); }
			LinesData( const LinesData& oth )
						{ copyContents(oth); }
    explicit		LinesData( const LineCollData& lcd )
						{ copyContents(lcd); }
			LinesData(const CubeData&)	= delete;

    virtual LinesData*	clone() const		{ return new LinesData(*this); }

    LinesData&		operator =( const LinesData& oth )
				{ copyContents(oth); return *this; }
    bool		operator ==( const LinesData& oth ) const
				{ return LineCollData::operator==(oth); }
			mImplSimpleIneqOper(LinesData)

    virtual bool	isCubeData() const	{ return false; }
    bool		isFullyRegular() const override;

    void		setTo( GeomID gid )	{ setTo( GeomIDSet(gid) ); }
    void		setTo(const GeomIDSet&);
    void		setToAllLines();

    bool		includes( const Bin2D& b2d ) const
			{ return LineCollData::includes(b2d); }
    LineCollDataPos	lineCollPos( const Bin2D& b2d ) const
			{ return LineCollData::lineCollPos(b2d); }
    glob_size_type	totalSizeInside(const LineHorSubSelSet&) const;
    glob_size_type	totalSizeInside(const LineHorSubSel&) const;
    bool		hasPosition(const LineHorSubSel&,glob_idx_type) const;
    bool		hasPosition(const LineHorSubSelSet&,
				    glob_idx_type) const;

    bool		includes(const BinID&) const	    = delete;
    LineCollDataPos	lineCollPos(const BinID&) const	    = delete;
    BinID		binID(const LineCollDataPos&) const = delete;

};

} // namespace PosInfo
