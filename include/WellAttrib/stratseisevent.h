#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			SeisEvent( Strat::LevelID
					lvlid =Strat::LevelID::udf(),
					VSEvent::Type evtyp=VSEvent::None )
			    : levelid_(lvlid)
			    , evtype_(evtyp)
			    , extrwin_(0.f,0.f)				{}

    float		snappedTime(const SeisTrc&) const;
    bool		snapPick(SeisTrc&) const;

    void		setLevelID( Strat::LevelID id ) { levelid_ = id; }
    void		setEvType( const VSEvent::Type evtyp )
						{ evtype_ = evtyp; }
    void		setOffset( float off )	{ offs_ = off; }
    void		setExtrWin( const Interval<float>& win )
						{ extrwin_ = win; }
    void		setExtrStep(float step ) { extrstep_ = step; }
    void		setDownToLevelID( Strat::LevelID id )
						{ downtolevel_ = id; }

    Strat::LevelID	levelID() const		{ return levelid_; }
    VSEvent::Type	evType() const		{ return evtype_; }
    float		offset() const		{ return offs_; }
    const Interval<float>& extrWin() const	{ return extrwin_; }
    float		extrStep() const	{ return extrstep_; }
    Strat::LevelID	downToLevelID() const	{ return downtolevel_; }

protected:

    Strat::LevelID	levelid_;
    VSEvent::Type	evtype_;
    float		offs_ = 0.f;
    Interval<float>	extrwin_;
    float		extrstep_ = mUdf(float);
    Strat::LevelID	downtolevel_;

};

} // namespace Strat
