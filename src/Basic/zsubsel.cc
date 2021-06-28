/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "zsubsel.h"

#include "iopar.h"
#include "keystrs.h"
#include "odjson.h"
#include "posidxsubsel.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "survsubsel.h"
#include "uistrings.h"

mUseType( Pos::ZSubSelData, idx_type );
mUseType( Pos::ZSubSelData, z_type );
mUseType( Pos::IdxSubSelData, pos_steprg_type );


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


bool Pos::ZSubSelData::sameOutputPosRange( const ZSubSelData& oth ) const
{
    const z_steprg_type thisrg( outputZRange() );
    const z_steprg_type othrg( oth.outputZRange() );
    const z_type eps = mIsZero( thisrg.step, mDefEpsF )
		     ? ( mIsZero( thisrg.width(), mDefEpsF )
			     ? thisrg.width() * 1e-5f : mDefEpsF )
		     : thisrg.step * 1e-5f;
    return thisrg.isEqual( othrg, eps );
}


bool Pos::ZSubSelData::includes( z_type z ) const
{
    const auto zrg = outputZRange();
    const auto fidx = zrg.getfIndex( z );
    const auto idx = mNINT32( fidx );
    return mIsZero( fidx - idx, zEps() );
}


bool Pos::ZSubSelData::includes( const ZSubSelData& oth ) const
{
    const auto zstep = zStep();
    const auto othzstep = oth.zStep();
    const auto fstepratio = othzstep / zstep;
    const auto stepratio = mNINT32( fstepratio );
    return stepratio >= 1 && mIsZero(stepratio-fstepratio,zEps())
	&& includes( oth.zStart() ) && includes( oth.zStop() );
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


const Pos::ZSubSel& Pos::ZSubSel::surv3D( const SurveyInfo* si )
{
    static ZSubSel ret( z_steprg_type(0.f,0.f,1.f) );
    ret.setInputZRange( SI(si).zRange() );
    return ret;
}


Pos::ZSubSel& Pos::ZSubSel::dummy()
{
    static ZSubSel ret( z_steprg_type(0.f,0.f,1.f) );
    ret = surv3D();
    return ret;
}


Pos::ZSubSel::ZSubSel( GeomID gid )
    : ssdata_(Survey::Geometry::get(gid).zRange())
{
}


bool Pos::ZSubSel::usePar( const IOPar& iop )
{
    z_steprg_type zrg;
    if ( !iop.get(sKey::ZRange(),zrg) )
	return false;

    ssdata_.setOutputZRange( zrg );
    return true;
}


bool Pos::ZSubSel::useJSON( const OD::JSON::Object& obj )
{
    z_steprg_type zrg;
    if ( !obj.get(sKey::ZRange(),zrg) )
	return false;

    ssdata_.setOutputZRange( zrg );
    return true;
}


void Pos::ZSubSel::fillPar( IOPar& iop ) const
{
    iop.set( sKey::ZRange(), zStart(), zStop(), zStep() );
}


void Pos::ZSubSel::fillJSON( OD::JSON::Object& obj ) const
{
    obj.set( sKey::ZRange(), zRange() );
}


Survey::FullZSubSel::FullZSubSel( const SurveyInfo* si )
    : geomids_(GeomID::get3D())
    , zsss_(ZSubSel(SI(si).zRange()))
    , si_(si)
{
}


Survey::FullZSubSel::FullZSubSel( const ZSubSel& zss )
    : geomids_(GeomID::get3D())
    , zsss_(zss)
{
}


Survey::FullZSubSel::FullZSubSel( const z_steprg_type& zrg )
    : geomids_(GeomID::get3D())
    , zsss_(ZSubSel(zrg))
{
}


Survey::FullZSubSel::FullZSubSel( GeomID gid, const SurveyInfo* si )
    : FullZSubSel( GeomIDSet(gid), si )
{
}


Survey::FullZSubSel::FullZSubSel( const GeomIDSet& gids, const SurveyInfo* si )
    : si_(si)
{
    for ( auto gid : gids )
	setFull( gid, si );
}


Survey::FullZSubSel::FullZSubSel( GeomID gid, const z_steprg_type& zrg,
				  const SurveyInfo* si )
    : si_(si)
{
    set( gid, ZSubSel(zrg) );
}


Survey::FullZSubSel::FullZSubSel( const FullZSubSel& oth )
    : geomids_(oth.geomids_)
    , zsss_(oth.zsss_)
    , si_(oth.si_)
{
}


Survey::FullZSubSel& Survey::FullZSubSel::operator =( const FullZSubSel& oth )
{
    if ( this != &oth )
    {
	geomids_ = oth.geomids_;
	zsss_ = oth.zsss_;
	si_ = oth.si_;
    }
    return *this;
}


bool Survey::FullZSubSel::operator ==( const FullZSubSel& oth ) const
{
    return geomids_ == oth.geomids_ && zsss_ == oth.zsss_;
}


const SurveyInfo& Survey::FullZSubSel::survInfo() const
{
    return SI( si_ );
}


bool Survey::FullZSubSel::is2D() const
{
    return geomids_.isEmpty() || geomids_.first().is2D();
}


Pos::ZSubSel& Survey::FullZSubSel::getFor( GeomID gid )
{
    const auto idx = indexOf( gid );
    return idx < 0 ? ZSubSel::dummy() : zsss_.get( idx );
}


bool Survey::FullZSubSel::isAll() const
{
    GeomIDSet gids;
    Survey::Geometry2D::getGeomIDs( gids );
    for ( auto gid : gids )
	if ( !isPresent(gid) || !getFor(gid).isAll() )
	    return false;

    return true;
}


bool Survey::FullZSubSel::hasFullRange() const
{
    for ( auto zss : zsss_ )
	if ( !zss.hasFullRange() )
	    return false;
    return true;
}


void Survey::FullZSubSel::setFull( GeomID gid, const SurveyInfo* si )
{
    if ( !gid.isValid() )
	return;
    si_ = si;
    if ( gid.is3D() )
	set( gid, ZSubSel(SI(si_).zRange()) );
    else
    {
	const auto& geom = Survey::Geometry::get( gid );
	if ( !geom.isEmpty() )
	    set( gid, ZSubSel(geom.zRange()) );
    }
}


void Survey::FullZSubSel::set( const ZSubSel& zss )
{
    set( GeomID::get3D(), zss );
}


void Survey::FullZSubSel::set( GeomID gid, const ZSubSel& zss )
{
    if ( !gid.isValid() )
	return;

    const auto idxof = indexOf( gid );
    if ( idxof >= 0 )
	zsss_.get(idxof) = zss;
    else
    {
	geomids_.add( gid );
	zsss_.add( zss );
    }
}


void Survey::FullZSubSel::setEmpty()
{
    geomids_.setEmpty();
    zsss_.setEmpty();
}


void Survey::FullZSubSel::remove( idx_type idx )
{
    if ( !geomids_.validIdx(idx) )
	{ pErrMsg("idx bad"); return; }
    geomids_.removeSingle( idx );
    zsss_.removeSingle( idx );
}


void Survey::FullZSubSel::remove( GeomID gid )
{
    const auto idx = indexOf( gid );
    if ( idx >= 0 )
	remove( idx );
}


void Survey::FullZSubSel::merge( const FullZSubSel& oth )
{
    for ( int idx=0; idx<oth.geomids_.size(); idx++ )
    {
	const auto gid = oth.geomids_.get( idx );
	const auto& othzss = oth.zsss_.get( idx );
	const auto idxof = indexOf( gid );
	if ( idxof >= 0 )
	    zsss_.get(idxof).merge( othzss );
	else
	    set( gid, othzss );
    }
}


void Survey::FullZSubSel::limitTo( const FullZSubSel& oth )
{
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	const auto gid = geomids_.get( idx );
	const auto idxof = oth.indexOf( gid );
	if ( idxof >= 0 )
	    zsss_.get(idx).limitTo( oth.get(idxof) );
	else
	{
	    geomids_.removeSingle( idx );
	    zsss_.removeSingle( idx );
	    idx--;
	}
    }
}



static const char* sKeyZSS = "Z Subsel";


void Survey::FullZSubSel::fillPar( IOPar& iop ) const
{
    const int sz = size();
    iop.set( IOPar::compKey(sKeyZSS,sKey::Size()), sz );
    iop.setYN( IOPar::compKey(sKeyZSS,sKey::Is2D()), is2D() );
    for ( int idx=0; idx<sz; idx++ )
    {
	const BufferString baseky( IOPar::compKey(sKeyZSS,idx) );
	iop.set( IOPar::compKey(baseky,sKey::GeomID()), geomids_.get(idx) );
	iop.set( IOPar::compKey(baseky,sKey::Range()),
				    zsss_.get(idx).outputZRange() );
    }
}


void Survey::FullZSubSel::fillJSON( OD::JSON::Object& obj ) const
{
    const int sz = size();
    if ( sz == 1 )
    {
	if ( is2D() )
	    obj.set( sKey::GeomID(), geomID() );
	else
	    obj.remove( sKey::GeomID() );
	obj.set( sKey::ZRange(), zRange() );
	return;
    }

    auto* zssobj = obj.set( sKeyZSS, new OD::JSON::Object );
    zssobj->set( sKey::Is2D(), is2D() );
    auto* zsssarr = zssobj->set( sKey::Subsel(), new OD::JSON::Array(true) );
    for ( int idx=0; idx<sz; idx++ )
    {
	auto* zsssobj = zsssarr->add( new OD::JSON::Object );
	zsssobj->set( sKey::GeomID(), geomids_.get( idx ) );
	zsssobj->set( sKey::Range(), zsss_.get(idx).outputZRange() );
    }
}


void Survey::FullZSubSel::usePar( const IOPar& inpiop, const SurveyInfo* si )
{
    PtrMan<IOPar> iop = inpiop.subselect( sKeyZSS );
    if ( !iop || iop->isEmpty() )
    {
	//Seis::SelData format
	bool is2d = false; SubSel::GeomID gid;
	SubSel::getInfo( inpiop, is2d, gid );
	if ( is2d )
	{
	    int idx = 0;
	    while( true )
	    {
		iop = inpiop.subselect(
				IOPar::compKey(sKey::Line(),toString(idx++) ) );
		if ( iop )
		{
		    if ( idx == 0 )
			setEmpty();
		}
		else
		    break;

		SubSel::getInfo( *iop, is2d, gid );
		{
		    z_steprg_type zrg;
		    if ( iop->get(sKey::ZRange(),zrg) )
		    {
			ZSubSel zss( survInfo().zRange() );
			zss.setOutputZRange( zrg );
			set( gid, zss );
		    }
		}
	    }
	}
	else
	{
	    ZSubSel zss( ZSubSel::surv3D(si) );
	    if ( zss.usePar(inpiop) )
	    {
		setEmpty();
		set( zss );
	    }
	}
	return;
    }

    int sz = 0;
    if ( !iop->get(sKey::Size(),sz) || sz < 1 )
	return;

    si_ = si;
    setEmpty();
    for ( int idx=0; idx<sz; idx++ )
    {
	const BufferString baseky( toString(idx) );
	GeomID gid; z_steprg_type zrg;
	iop->get( IOPar::compKey(baseky,sKey::GeomID()), gid );
	iop->get( IOPar::compKey(baseky,sKey::Range()), zrg );
	if ( gid.isValid() )
	{
	    ZSubSel zss( survInfo().zRange() );
	    zss.setOutputZRange( zrg );
	    set( gid, zss );
	}
    }
}


void Survey::FullZSubSel::useJSON( const OD::JSON::Object& inpobj,
				   const SurveyInfo* si )
{
    const OD::JSON::Object* obj = inpobj.getObject( sKeyZSS );
    if ( !obj )
    {
	//Seis::SelData format
	bool is2d; SubSel::GeomID gid;
	if ( !SubSel::getInfo(inpobj,is2d,gid) )
	    return;

	if ( is2d )
	{
	    int idx = 0;
	    const auto* lineobj = inpobj.getArray( sKey::Line() );
	    bool first = true;
	    while( lineobj && true )
	    {
		obj = &lineobj->object( idx++ );
		if ( obj )
		{
		    if ( first )
		    {
			setEmpty();
			first = false;
		    }
		}
		else
		    break;

		SubSel::getInfo( *obj, is2d, gid );
		if ( is2d && gid.isValid() )
		{
		    z_steprg_type zrg;
		    if ( obj->get(sKey::ZRange(),zrg) )
		    {
			ZSubSel zss( survInfo().zRange() );
			zss.setOutputZRange( zrg );
			set( gid, zss );
		    }
		}
	    }
	}
	else
	{
	    ZSubSel zss( ZSubSel::surv3D(si) );
	    if ( zss.useJSON(inpobj) )
	    {
		setEmpty();
		set( zss );
	    }
	}
	return;
    }

    const auto* zssarr = obj->getArray( sKey::Subsel() );
    if ( !zssarr || zssarr->size() < 1 )
	return;

    si_ = si;

    setEmpty();
    const int sz = zssarr->size();
    for ( int idx=0; idx<sz; idx++ )
    {
	GeomID gid; z_steprg_type zrg;
	const auto& zssobj = zssarr->object( idx );
	zssobj.getGeomID( sKey::GeomID(), gid );
	zssobj.get( sKey::Range(), zrg );
	if ( gid.isValid() )
	{
	    ZSubSel zss( survInfo().zRange() );
	    zss.setOutputZRange( zrg );
	    set( gid, zss );
	}
    }
}


uiString Survey::FullZSubSel::getUserSummary() const
{
    uiString ret;
    if ( isEmpty() )
	return ret;

    const auto& zss0 = get( 0 );
    z_rg_type minmaxrg = zss0.outputZRange();
    bool varying = false;
    bool isall = zss0.isAll();
    for ( auto idx=1; idx<size(); idx++ )
    {
	const auto& zss = get( idx );
	minmaxrg.include( zss.outputZRange(), false );
	isall = isall && zss.isAll();
	varying = varying || zss != zss0;
    }

    uiString rgstr = uiStrings::sRangeTemplate( true );
    rgstr.arg( minmaxrg.start ).arg( minmaxrg.stop ).arg( zss0.zStep() );
    ret.set( toUiString("%1: %2").arg( uiStrings::sZRange() ).arg( rgstr ) );
    if ( size() > 1 )
    {
	if ( isall )
	    ret.appendPhraseSameLine( uiStrings::sFull().parenthesize() );
	else if ( varying )
	    ret.appendPhraseSameLine(
			uiStrings::sVariable(false).parenthesize() );
    }

    return ret;
}
