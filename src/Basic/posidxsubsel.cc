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
    return inpposrg_.start + posStep() * offs_;
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
    step_ = newstep / inpposrg_.step;
    if ( step_ < 1 )
	{ pErrMsg("Bad pos step"); step_ = 1; }
    newstep = step_ * inpposrg_.step;

    auto dstart = newstart - inpposrg_.start;
    if ( dstart < 0 )
	dstart = 0;

    offs_ = dstart / newstep;
    if ( dstart % newstep )
    {
	offs_++;
	dstart = offs_ * newstep;
    }
    newstart = inpposrg_.start + offs_ * newstep;

    auto dstop = newstop - inpposrg_.start;
    if ( dstop < dstart )
	dstop = dstart;
    auto stopoffs = dstop / newstep;
    newstop = inpposrg_.start + stopoffs * newstep;

    sz_ = (newstop - newstart) / newstep + 1;
    ensureSizeOK();
}


void Pos::IdxSubSelData::limitTo( const IdxSubSelData& oth )
{
    auto outrg = outputPosRange();
    outrg.limitTo( oth.outputPosRange() );
    setOutputPosRange( outrg );
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
    return inpzrg_.start + zStep() * offs_;
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
    z_type fstep = newstep / inpzrg_.step;
    step_ = mRounded( idx_type, fstep );
    if ( step_ < 1 )
	{ pErrMsg("Bad z step"); step_ = 1; }
    newstep = step_ * inpzrg_.step;

    z_type dstart = newstart - inpzrg_.start;
    if ( dstart < zEps() )
	dstart = (z_type)0;

    z_type foffs = dstart / newstep;
    offs_ = mRounded( idx_type, foffs );
    if ( !mIsZero(foffs-offs_,zEps()) )
    {
	offs_++;
	dstart = offs_ * newstep;
    }
    newstart = inpzrg_.start + offs_ * newstep;

    auto dstop = newstop - inpzrg_.start;
    if ( dstop < dstart )
	dstop = dstart;
    auto stopoffs = dstop / newstep;
    newstop = inpzrg_.start + stopoffs * newstep;

    z_type fsz = (newstop - newstart) / newstep + 1;
    sz_ = (size_type)(fsz + zEps());
    ensureSizeOK();
}


void Pos::ZSubSelData::ensureSizeOK()
{
    if ( zStop() > inpzrg_.stop+zEps() )
    {
	z_type fsz = (inpzrg_.stop - zStart()) / zStep() + 1;
	sz_ = (size_type)(fsz + zEps());
    }
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
