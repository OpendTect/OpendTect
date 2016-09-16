#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2011
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "valseriesevent.h"
#include "ranges.h"
#include "stratlevel.h"
class SeisTrc;


namespace Strat
{
class Level;

/*!\brief Event tied to a stratigraphic level. */

mExpClass(WellAttrib) SeisEvent
{
public:

    typedef Strat::Level::ID	LevelID;
    typedef VSEvent::Type	EvType;

			SeisEvent( LevelID lvlid=LevelID::getInvalid(),
				   EvType evtyp=VSEvent::None )
			    : levelid_(lvlid)
			    , evtype_(evtyp)
			    , offs_(0)
			    , extrwin_(0,0)
			    , extrstep_(mUdf(float))	    {}

    float		snappedTime(const SeisTrc&) const;
    bool		snapPick(SeisTrc&) const;

    void		setLevelID( LevelID id ) { levelid_ = id; }
    void		setEvType( const EvType evtyp )
						{ evtype_ = evtyp; }
    void		setOffset( float off )	{ offs_ = off; }
    void		setExtrWin( const Interval<float>& win )
						{ extrwin_ = win; }
    void		setExtrStep(float step ) { extrstep_ = step; }
    void		setDownToLevelID( LevelID id )
						{ downtolevelid_ = id; }

    LevelID		levelID() const		{ return levelid_; }
    EvType		evType() const		{ return evtype_; }
    float		offset() const		{ return offs_; }
    const Interval<float>& extrWin() const	{ return extrwin_; }
    float		extrStep() const	{ return extrstep_; }
    LevelID		downToLevelID() const	{ return downtolevelid_; }

protected:

    LevelID		levelid_;
    VSEvent::Type	evtype_;
    float		offs_;
    Interval<float>	extrwin_;
    float		extrstep_;
    LevelID		downtolevelid_;

};

} // namespace
