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

/*!\brief Event tied to a stratigraphic level. */

mExpClass(WellAttrib) SeisEvent
{
public:
			SeisEvent( Strat::Level::ID
					lvlid =Strat::Level::cUndefID(),
					VSEvent::Type evtyp=VSEvent::None )
			    : levelid_(lvlid)
			    , evtype_(evtyp)
			    , extrwin_(0.f,0.f)
			    , downtolevel_(Strat::Level::cUndefID())	{}

    float		snappedTime(const SeisTrc&) const;
    bool		snapPick(SeisTrc&) const;

    void		setLevelID( Strat::Level::ID id ) { levelid_ = id; }
    void		setEvType( const VSEvent::Type evtyp )
						{ evtype_ = evtyp; }
    void		setOffset( float off )	{ offs_ = off; }
    void		setExtrWin( const Interval<float>& win )
						{ extrwin_ = win; }
    void		setExtrStep(float step ) { extrstep_ = step; }
    void		setDownToLevelID( Strat::Level::ID id )
						{ downtolevel_ = id; }

    Strat::Level::ID	levelID() const		{ return levelid_; }
    VSEvent::Type	evType() const		{ return evtype_; }
    float		offset() const		{ return offs_; }
    const Interval<float>& extrWin() const	{ return extrwin_; }
    float		extrStep() const	{ return extrstep_; }
    Strat::Level::ID	downToLevelID() const	{ return downtolevel_; }

protected:

    Strat::Level::ID	levelid_;
    VSEvent::Type	evtype_;
    float		offs_ = 0.f;
    Interval<float>	extrwin_;
    float		extrstep_ = mUdf(float);
    Strat::Level::ID	downtolevel_;

};

} // namespace

