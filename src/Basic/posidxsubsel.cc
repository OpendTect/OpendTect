/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "posidxsubsel.h"
#include "iopar.h"

mUseType( Pos::IdxSubSelData, idx_type );
mUseType( Pos::IdxSubSelData, pos_type );
mUseType( Pos::IdxSubSelData, pos_steprg_type );


Pos::IdxSubSelData::IdxSubSelData( const pos_steprg_type& rg )
    : ArrRegSubSelData(rg.nrSteps()+1)
    , inpposrg_(rg)
{
    ensureSizeOK();
}


bool Pos::IdxSubSelData::operator ==( const Pos::IdxSubSelData& oth ) const
{
    return inpposrg_ == oth.inpposrg_
	&& ArrRegSubSelData::operator ==( oth );
}


bool Pos::IdxSubSelData::sameOutputPosRange( const Pos::IdxSubSelData& oth
									) const
{
    const pos_steprg_type thisrg( outputPosRange() );
    const pos_steprg_type othrg( oth.outputPosRange() );
    return thisrg == othrg;
}


bool Pos::IdxSubSelData::hasFullRange() const
{
    return posStart() == inpposrg_.start && posStop() == inpposrg_.stop;
}


idx_type Pos::IdxSubSelData::idx4Pos( pos_type pos ) const
{
    return (pos - posStart()) / posStep();
}


pos_type Pos::IdxSubSelData::pos4Idx( idx_type idx ) const
{
    return posStart() + posStep() * idx;
}


pos_type Pos::IdxSubSelData::posStart() const
{
    return inpposrg_.start + inpposrg_.step * offs_;
}


pos_type Pos::IdxSubSelData::posStep() const
{
    return inpposrg_.step * step_;
}


pos_type Pos::IdxSubSelData::posStop() const
{
    return posStart() + posStep() * (sz_-1);
}


bool Pos::IdxSubSelData::includes( const IdxSubSelData& oth ) const
{
    const auto othposrg = oth.outputPosRange();
    if ( othposrg.start == othposrg.stop )
	return includes( othposrg.start );

    const auto posrg = outputPosRange();
    return posrg.step <= othposrg.step
        && othposrg.step % posrg.step == 0
        && includes( othposrg.start )
        && includes( othposrg.stop );
}


bool Pos::IdxSubSelData::includes( pos_type pos ) const
{
    return outputPosRange().isPresent( pos );
}


void Pos::IdxSubSelData::setInputPosRange( const pos_steprg_type& newrg )
{
    inpposrg_ = newrg;
    sz_ = inpposrg_.nrSteps() + 1;
    ensureSizeOK();
}


void Pos::IdxSubSelData::setOutputPosRange( pos_type newstart,
				pos_type newstop, pos_type newstep )
{
    if ( newstart <= 0 || mIsUdf(newstart) )
	newstart = inpposrg_.start;
    if ( newstop <= 0 || mIsUdf(newstop) )
	newstop = inpposrg_.stop;
    if ( newstep <= 0 || mIsUdf(newstep) )
	newstep = inpposrg_.step;

    step_ = newstep / inpposrg_.step;
    if ( step_ < 1 )
	step_ = 1;
    newstep = step_ * inpposrg_.step;

    auto dstart = newstart - inpposrg_.start;
    if ( dstart < 0 )
	dstart = 0;

    offs_ = dstart / inpposrg_.step;
    if ( dstart % inpposrg_.step )
    {
	offs_++;
	dstart = offs_ * inpposrg_.step;
    }
    newstart = inpposrg_.start + dstart;

    auto dstop = newstop - newstart;
    if ( dstop < 0 )
	dstop = 0;
    auto stopoffs = dstop / newstep;
    newstop = newstart + stopoffs * newstep;

    sz_ = (newstop - newstart) / newstep + 1;
    ensureSizeOK();
}


void Pos::IdxSubSelData::setOutputStep( pos_type newstep, pos_type existingpos )
{
    if ( newstep <= 0 || mIsUdf(newstep) )
	newstep = inpposrg_.step;
    if ( newstep == posStep() )
	return;

    if ( mIsUdf(existingpos) )
	existingpos = posStart();

    auto newrg( outputPosRange() );
    newrg.step = newstep;
    if ( (existingpos-newrg.start) % newstep )
    {
	const auto existingposidx = (existingpos-newrg.start) / newstep;
	const auto posadd = existingpos - newrg.atIndex( existingposidx );
	newrg.start += posadd; newrg.stop += posadd;
    }

    setOutputPosRange( newrg );
}


void Pos::IdxSubSelData::limitTo( const IdxSubSelData& oth )
{
    auto outrg = outputPosRange();
    const auto othoutrg = oth.outputPosRange();
    outrg.limitTo( othoutrg );
    if ( othoutrg.step > outrg.step )
	outrg.step = othoutrg.step;
    setOutputPosRange( outrg );
}


void Pos::IdxSubSelData::widenTo( const IdxSubSelData& oth )
{
    auto outrg = outputPosRange();
    const auto othoutrg = oth.outputPosRange();
    if ( othoutrg.start < outrg.start )
	outrg.start = outrg.atIndex( outrg.nearestIndex(othoutrg.start) );
    if ( othoutrg.stop > outrg.stop )
	outrg.stop = outrg.atIndex( outrg.nearestIndex(othoutrg.stop) );
    setOutputPosRange( outrg );
}


void Pos::IdxSubSelData::addStepout( idx_type so )
{
    const auto posdelta = so * posStep();
    auto posrg = outputPosRange();
    posrg.start -= posdelta;
    posrg.stop += posdelta;
    setOutputPosRange( posrg );
}


void Pos::IdxSubSelData::ensureSizeOK()
{
    if ( posStop() > inpposrg_.stop )
	sz_ = (inpposrg_.stop - posStart()) / posStep() + 1;
}
