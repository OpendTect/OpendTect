#ifndef stratseisevent_h
#define stratseisevent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "valseriesevent.h"
#include "ranges.h"
class SeisTrc;


namespace Strat
{
class Level;

/*!\brief Event tied to a stratigraphic level. */

mExpClass(WellAttrib) SeisEvent
{
public:
			SeisEvent(const Level* lvl=0,
				   VSEvent::Type evtyp=VSEvent::None)
			    : level_(lvl)
			    , evtype_(evtyp)
			    , offs_(0)
			    , extrwin_(0,0)
			    , extrstep_(mUdf(float))
			    , downtolevel_(0)		{}

    float		snappedTime(const SeisTrc&) const;
    bool		snapPick(SeisTrc&) const;

    void		setLevel( const Strat::Level* lvl ) { level_ = lvl; }
    void		setEvType( const VSEvent::Type evtyp )
						{ evtype_ = evtyp; }
    void		setOffset( float off )	{ offs_ = off; }
    void		setExtrWin( const Interval<float>& win )
						{ extrwin_ = win; }
    void		setExtrStep(float step ) { extrstep_ = step; }
    void		setDownToLevel(  const Strat::Level* downtolevel )
						{ downtolevel_ = downtolevel; }

    const Strat::Level* level() const		{ return level_; }
    const VSEvent::Type& evType() const 	{ return evtype_; }
    float		offset() const		{ return offs_; }
    const Interval<float>& extrWin() const	{ return extrwin_; }
    float		extrStep() const	{ return extrstep_; }
    const Strat::Level* downToLevel() const	{ return downtolevel_; }

protected:

    const Strat::Level*	level_;
    VSEvent::Type	evtype_;
    float		offs_;
    Interval<float>	extrwin_;
    float		extrstep_;
    const Strat::Level*	downtolevel_;

};

} // namespace

#endif

