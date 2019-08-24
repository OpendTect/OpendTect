#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2002
 Contents:	PAuxiliary info on position
________________________________________________________________________

-*/

#include "generalmod.h"
#include "position.h"
#include "trckey.h"

mStartAllowDeprecatedSection

/*!\brief Auxiliary data possibly needed at location. Mostly a seismic thing. */

mExpClass(General) PosAuxInfo
{
public:

		PosAuxInfo()
		    : coord( coord_ )
		    , startpos( startpos_ )
		    , offset( offset_)
		    , azimuth( azimuth_ )
		    , pick( pick_ )
		    , refnr( refnr_ )
		    , binid( const_cast<BinID&>(trckey_.binID()) )
		{ clear(); }

		PosAuxInfo( const PosAuxInfo& b)
		    : coord_( b.coord_ )
		    , startpos_( b.startpos_ )
		    , offset_( b.offset_)
		    , azimuth_( b.azimuth_ )
		    , pick_( b.pick_ )
		    , refnr_( b.refnr_ )
		    , coord( coord_ )
		    , startpos( startpos_ )
		    , offset( offset_)
		    , azimuth( azimuth_ )
		    , pick( pick_ )
		    , refnr( refnr_ )
		    , binid( const_cast<BinID&>(trckey_.binID()) )
		{}

    void	clear()
		{
		    trckey_.setPos( BinID(0,0) );
		    coord_.x_ = coord_.y_ = 0;
		    startpos_ = offset_ = azimuth_ = 0;
		    pick_ = refnr_ = mUdf(float);
		}

    TrcKey	trckey_;
    Coord	coord_;
    float	startpos_;
    float	offset_;
    float	azimuth_;
    float	pick_;
    float	refnr_;

    mDeprecated BinID&	binid;		//!<Old syntax. Will be deprecated
    mDeprecated Coord&	coord;		//!<Old syntax. Will be deprecated
    mDeprecated float&	startpos;	//!<Old syntax. Will be deprecated
    mDeprecated float&	offset;		//!<Old syntax. Will be deprecated
    mDeprecated float&	azimuth;	//!<Old syntax. Will be deprecated
    mDeprecated float&	pick;		//!<Old syntax. Will be deprecated
    mDeprecated float&	refnr;		//!<Old syntax. Will be deprecated

};


/*!\brief Selection of aux info at location.
	  Note that BinID is always selected */

mExpClass(General) PosAuxInfoSelection
{
public:
		PosAuxInfoSelection()
		    : startpos_(false), coord_(false)
		    , offset_(false), azimuth_(false)
		    , pick_(false), refnr_(false)
		    , startpos(startpos_), coord(coord_)
		    , offset(offset_), azimuth(azimuth_)
		    , pick(pick_), refnr(refnr_)
		{}

		PosAuxInfoSelection( const PosAuxInfoSelection& b)
		    : startpos_(b.startpos_), coord_(b.coord_)
		    , offset_(b.offset_), azimuth_(b.azimuth_)
		    , pick_(b.pick_), refnr_(b.refnr_)
		    , startpos(startpos_), coord(coord_)
		    , offset(offset_), azimuth(azimuth_)
		    , pick(pick_), refnr(refnr_)
		{}

    PosAuxInfoSelection&    operator=(const PosAuxInfoSelection& b)
			    {
				startpos_ = b.startpos_;
				coord_ = b.coord_;
				offset_ = b.offset_;
				azimuth_ = b.azimuth_;
				pick_ = b.pick_;
				refnr_ = b.refnr_;
				return *this;
			    }

		void setAll( bool yn )
		{startpos_ = coord_ = offset_ = azimuth_ = pick_ = refnr_ = yn;}

    bool		startpos_;
    bool		coord_;
    bool		offset_;
    bool		azimuth_;
    bool		pick_;
    bool		refnr_;

    mDeprecated bool&	startpos;   //!<Old syntax. Will be deprecated
    mDeprecated bool&	coord;      //!<Old syntax. Will be deprecated
    mDeprecated bool&	offset;     //!<Old syntax. Will be deprecated
    mDeprecated bool&	azimuth;    //!<Old syntax. Will be deprecated
    mDeprecated bool&	pick;       //!<Old syntax. Will be deprecated
    mDeprecated bool&	refnr;      //!<Old syntax. Will be deprecated
};

mStopAllowDeprecatedSection
