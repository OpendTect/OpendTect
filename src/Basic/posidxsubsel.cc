/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "posidxsubsel.h"
#include "zsubsel.h"
#include "iopar.h"
#include "keystrs.h"

mUseType( Pos::IdxSubSelData, idx_type );
mUseType( Pos::IdxSubSelData, pos_type );
mUseType( Pos::IdxSubSelData, pos_steprg_type );
mUseType( Pos::ZSubSelData, z_type );


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


void Pos::IdxSubSelData::addStepout( pos_type so )
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


Pos::ZSubSelData::ZSubSelData( const z_steprg_type& rg )
    : ArrRegSubSelData(rg.nrSteps()+1)
    , inpzrg_(rg)
{
    ensureSizeOK();
}


bool Pos::ZSubSelData::operator ==( const Pos::ZSubSelData& oth ) const
{
    return inpzrg_.isEqual( oth.inpzrg_, zEps() )
	&& ArrRegSubSelData::operator ==( oth );
}


bool Pos::ZSubSelData::isAll() const
{
    return !isSubSpaced() && hasFullRange();
}


bool Pos::ZSubSelData::hasFullRange() const
{
    return mIsEqual(zStart(),inpzrg_.start,zEps())
	&& mIsEqual(zStop(),inpzrg_.stop,zEps());
}


idx_type Pos::ZSubSelData::idx4Z( z_type z ) const
{
    const auto fnrz = (z - zStart()) / zStep();
    return mNINT32( fnrz );
}


z_type Pos::ZSubSelData::z4Idx( idx_type idx ) const
{
    return zStart() + zStep() * idx;
}


z_type Pos::ZSubSelData::zStart() const
{
    return inpzrg_.start + inpzrg_.step * offs_;
}


z_type Pos::ZSubSelData::zStep() const
{
    return inpzrg_.step * step_;
}


z_type Pos::ZSubSelData::zStop() const
{
    return zStart() + zStep() * (sz_-1);
}


void Pos::ZSubSelData::setInputZRange( const z_steprg_type& newrg )
{
    inpzrg_ = newrg;
    sz_ = inpzrg_.nrSteps() + 1;
    ensureSizeOK();
}


void Pos::ZSubSelData::setOutputZRange( z_type newstart, z_type newstop,
					z_type newstep )
{
    if ( mIsUdf(newstart) )
	newstart = inpzrg_.start;
    if ( mIsUdf(newstop) )
	newstop = inpzrg_.stop;
    if ( mIsUdf(newstep) || newstep <= 0.f )
	newstep = inpzrg_.step;

    IdxSubSelData newss( pos_steprg_type(0,inpzrg_.nrSteps(),1) );
    z_type fnewstep = newstep / inpzrg_.step;
    ZSubSelData cleanzss( inpzrg_ );
    newss.setOutputPosRange( cleanzss.idx4Z(newstart), cleanzss.idx4Z(newstop),
			     mRounded(z_type,fnewstep) );
    ArrRegSubSelData::operator =( newss );
}


void Pos::ZSubSelData::ensureSizeOK()
{
    if ( zStop() > inpzrg_.stop+zEps() )
    {
	z_type fsz = (inpzrg_.stop - zStart()) / zStep() + 1;
	sz_ = (size_type)(fsz + zEps());
    }
}


void Pos::ZSubSelData::limitTo( const ZSubSelData& oth )
{
    auto outrg = outputZRange();
    outrg.limitTo( oth.outputZRange() );
    setOutputZRange( outrg );
}


void Pos::ZSubSelData::limitTo( const z_rg_type& zrg )
{
    auto outrg = outputZRange();
    outrg.limitTo( zrg );
    setOutputZRange( outrg );
}


void Pos::ZSubSelData::widenTo( const ZSubSelData& oth )
{
    auto outrg = outputZRange();
    const auto othoutrg = oth.outputZRange();
    if ( othoutrg.start < outrg.start )
	outrg.start = outrg.atIndex( outrg.nearestIndex(othoutrg.start) );
    if ( othoutrg.stop > outrg.stop )
	outrg.stop = outrg.atIndex( outrg.nearestIndex(othoutrg.stop) );
    setOutputZRange( outrg );
}


void Pos::ZSubSelData::widen( const z_rg_type& zrg )
{
    auto outrg = outputZRange();
    outrg.start += zrg.start; outrg.stop += zrg.stop;
    setOutputZRange( outrg );
}


bool Pos::ZSubSel::usePar( const IOPar& iop )
{
    z_steprg_type zrg;
    if ( !iop.get(sKey::ZRange(),zrg) )
	return false;

    data_.setOutputZRange( zrg );
    return true;
}


void Pos::ZSubSel::fillPar( IOPar& iop ) const
{
    iop.set( sKey::ZRange(), zStart(), zStop(), zStep() );
}
