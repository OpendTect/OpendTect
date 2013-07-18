/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinstpkgselmgr.cc 7574 2013-03-26 13:56:34Z kristofer.tingdahl@dgbes.com $";

#include "odinstpkgselmgr.h"
#include "odinstpkgprops.h"
#include "odinstappdata.h"

DefineEnumNames(ODInst::PkgSelMgr,ReqAction,1,"Required action")
{ "None", "Install", "Upgrade", "Remove", "Reinstall", 0 };


ODInst::PkgSelMgr::PkgSelMgr( const ODInst::AppData& ad,
	const ODInst::PkgGroupSet& pps, const ODInst::Platform& plf )
	: platform_(plf)
{
    for ( int igrp=0; igrp<pps.size(); igrp++ )
    {
	const PkgGroup& pg = *pps[igrp];
	for ( int ipkg=0; ipkg<pg.size(); ipkg++ )
	{
	    const PkgProps& pp = *pg[ipkg];
	    const PkgKey* pk = ad.installedPkgs().get( pp.pkgnm_, plf );
	    if ( !pk )
		*this += new PkgSelection( pp, false, Version() );
	    else
		*this += new PkgSelection( pp, true, pk->ver_ );
	}
    }
}


int ODInst::PkgSelMgr::idxOf( const PkgProps& pp ) const
{
    for ( int ipkg=0; ipkg<size(); ipkg++ )
	if ( &((*this)[ipkg]->pp_) == &pp )
	    return ipkg;
    return -1;
}


int ODInst::PkgSelMgr::idxOf( const char* pkgnm ) const
{
    const FixedString nm( pkgnm );
    for ( int ipkg=0; ipkg<size(); ipkg++ )
	if ( (*this)[ipkg]->pp_.pkgnm_==nm )
	    return ipkg;
    return -1;
}


int ODInst::PkgSelMgr::nrSelections() const
{
    int nr = 0;
    for ( int ipkg=0; ipkg<size(); ipkg++ )
	if ( (*this)[ipkg]->issel_ )
	    nr++;
    return nr;
}


bool ODInst::PkgSelMgr::isNeeded( ODInst::PkgSelMgr::ReqAction act ) const
{
    for ( int ipkg=0; ipkg<size(); ipkg++ )
    {
	const PkgSelection& ps = *(*this)[ipkg];
	if ( reqAction(ps) == act )
	    return true;
    }
    return false;
}


ODInst::PkgSelMgr::ReqAction ODInst::PkgSelMgr::reqAction(
					const ODInst::PkgProps& pp ) const
{
    const int idxof = idxOf( pp );
    return idxof < 0 ? None : reqAction( *(*this)[idxof] );
}


ODInst::PkgSelMgr::ReqAction ODInst::PkgSelMgr::reqAction(
					const ODInst::PkgSelection& ps ) const
{
    if ( !ps.issel_ )
	return ps.isinst_ ? Remove : None;
    else if ( !ps.isinst_ )
	return Install;

    return ps.pp_.ver_ > ps.instver_ ? Upgrade
				     : (ps.doforceinstall_? Reinstall : None);
}


int ODInst::PkgSelMgr::nrFiles2Get( bool inclhidden ) const
{
    int nr = 0;
    for ( int ipkg=0; ipkg<size(); ipkg++ )
    {
	const PkgSelection& ps = *(*this)[ipkg];
	const ReqAction act = reqAction( ps );
	if ( act == Install || act == Upgrade || act == Reinstall )
	{
	    if ( inclhidden || !ps.pp_.internal_ )
		nr++;
	}
    }
    return nr;
}


bool ODInst::PkgSelMgr::isSelected( const PkgProps& pp ) const
{
    const int idxof = idxOf( pp );
    return idxof < 0 ? false : (*this)[idxof]->issel_;
}


void ODInst::PkgSelMgr::setSelected( const PkgProps& pp, bool yn )
{
    const int idxof = idxOf( pp );
    if ( idxof < 0 || yn == isSelected(pp) )
	return;

    PkgSelection& tarps = *(*this)[idxof];
    tarps.issel_ = yn;
    for ( int ipkg=0; ipkg<size(); ipkg++ )
    {
	if ( ipkg == idxof ) continue;

	PkgSelection& ps = *(*this)[ipkg];
	if ( yn && pp.isDepOn( ps.pp_ ) )
	    setSelected( ps.pp_, true );
	else if ( !yn && ps.pp_.isDepOn(pp) )
	    setSelected( ps.pp_, false );
    }

    // remove hidden unnecessary packages
    for ( int ipkg=0; ipkg<size(); ipkg++ )
    {
	PkgSelection& ps = *(*this)[ipkg];
	if ( !ps.issel_ || !ps.pp_.isHidden() )
	    continue;
	bool isneeded = false;
	for ( int idep=0; idep<size(); idep++ )
	{
	    if ( idep == ipkg ) continue;
	    const PkgSelection& depps = *(*this)[idep];
	    if ( depps.issel_ && depps.pp_.isDepOn(ps.pp_) )
		{ isneeded = true; break; }
	}
	if ( !isneeded )
	    setSelected( ps.pp_, false );
    }
}


void ODInst::PkgSelMgr::setForceReinstall( const PkgProps& pp, bool yn )
{
    const int idxof = idxOf( pp );
    if ( idxof < 0 || !isSelected(pp) )
	return;

    PkgSelection& tarps = *(*this)[idxof];
    tarps.doforceinstall_ = yn;
}


bool ODInst::PkgSelMgr::shouldReInstall( const PkgProps& pp ) const
{
    const int idxof = idxOf( pp );
    if ( idxof < 0 )
	return false;

    const PkgSelection& tarps = *(*this)[idxof];
    return tarps.doforceinstall_; 
}


bool ODInst::PkgSelMgr::isInstalled( const PkgProps& pp ) const
{
    const int idxof = idxOf( pp );
    return idxof < 0 ? false : (*this)[idxof]->isinst_;
}


ODInst::Version ODInst::PkgSelMgr::version( const PkgProps& pp,
					    bool inst ) const
{
    if ( !inst )
	return pp.ver_;

    const int idxof = idxOf( pp );
    return idxof<0 ? pp.ver_ : (*this)[idxof]->instver_;
}

#define mCleanList \
    pkgstodownload.erase(); \
    if ( installedpckglist ) installedpckglist->erase(); \
    if ( updatedpckglist ) updatedpckglist->erase(); \
    if ( reinstalledpkglist ) reinstalledpkglist->erase();

void ODInst::PkgSelMgr::
		getPackageListsForAction( BufferStringSet& pkgstodownload,
					  BufferStringSet* installedpckglist,
					  BufferStringSet* updatedpckglist,
					  BufferStringSet* reinstalledpkglist ) 
{
    mCleanList;
    for ( int idx=0; idx<size(); idx++ )
    {
	PkgSelection* psel = (*this)[idx];
	ODInst::PkgSelMgr::ReqAction reqact = reqAction( *psel );
	if ( reqact == Remove || reqact == None )
	    continue;

	pkgstodownload.add( psel->pp_.pkgnm_ );
	if ( psel->pp_.isHidden() )
	    continue;

	if ( installedpckglist && reqact == Install )
	    installedpckglist->add( psel->pp_.usrname_ );
	else if ( updatedpckglist && reqact == Upgrade )
	    updatedpckglist->add( psel->pp_.usrname_ );
	else if ( reinstalledpkglist && reqact == Reinstall )
	    reinstalledpkglist->add( psel->pp_.usrname_ );
    }
}


BufferString ODInst::PkgSelMgr::getFullPackageName(
					    const BufferString& pckgnm ) const
{
    const PkgProps& pp = (*this)[ idxOf(pckgnm) ]->pp_;
    ODInst::PkgKey pk = pp.pkgKey( platform_ );
    return pk.zipFileName();
}


BufferString ODInst::PkgSelMgr::getUserName( const BufferString& pkgnm ) const
{
    return (*this)[ idxOf(pkgnm) ]->pp_.usrname_;
}


void ODInst::PkgSelMgr::getOfflineInstallPackages( BufferStringSet& list )
{
    list.erase();
    for ( int idx=0; idx<size(); idx++ )
    {
	PkgSelection* psel = (*this)[idx];
	if ( psel->issel_ )
	    list.add( psel->pp_.pkgnm_ );
    }
}
