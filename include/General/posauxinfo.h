#ifndef posauxinfo_h
#define posauxinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2002
 Contents:	PAuxiliary info on position
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "position.h"


/*!\brief Auxiliray data possibly needed at location. Mostly a seismic thing. */

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
                    , binid( trckey_.pos() )
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
                    , binid( trckey_.pos() )
                {}

    void	clear()
		{
                    trckey_.setPos(BinID(0,0));
                    trckey_.setSurvID( mUdf(Pos::SurvID) );
		    coord_.x = coord_.y = 0;
		    startpos_ = offset_ = azimuth_ = 0;
		    pick_ = refnr_ = mUdf(float);
		}

    TrcKey      trckey_;
    Coord	coord_;
    float	startpos_;
    float	offset_;
    float	azimuth_;
    float	pick_;
    float	refnr_;

    BinID&      binid;      //!<Old syntax. Will be deprecated
    Coord&	coord;      //!<Old syntax. Will be deprecated
    float&	startpos;   //!<Old syntax. Will be deprecated
    float&	offset;     //!<Old syntax. Will be deprecated
    float&	azimuth;    //!<Old syntax. Will be deprecated
    float&	pick;       //!<Old syntax. Will be deprecated
    float&	refnr;      //!<Old syntax. Will be deprecated

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

    bool	startpos_;
    bool	coord_;
    bool	offset_;
    bool	azimuth_;
    bool	pick_;
    bool	refnr_;

    bool&	startpos;   //!<Old syntax. Will be deprecated
    bool&	coord;      //!<Old syntax. Will be deprecated
    bool&	offset;     //!<Old syntax. Will be deprecated
    bool&	azimuth;    //!<Old syntax. Will be deprecated
    bool&	pick;       //!<Old syntax. Will be deprecated
    bool&	refnr;      //!<Old syntax. Will be deprecated
};


#endif
