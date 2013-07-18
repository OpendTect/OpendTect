/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinstpkgprops.cc 7930 2013-05-31 11:40:13Z ranojay.sen@dgbes.com $";

#include "odinstpkgprops.h"
#include "odinstpkgkey.h"

#include "strmprov.h"
#include "strmoper.h"
#include "ascstream.h"
#include "iopar.h"
#include "keystrs.h"
#include "separstr.h"

static const char* sKeyDesc = "Desc";
static const char* sKeyLabels = "Labels";
static const char* sKeyLabel = "Label";
static const char* sKeyShortDesc = "Short Desc";
static const char* sKeyDep = "Depends on";
static const char* sKeyURL = "URL";
static const char* sKeyPkgFnmBase = "PKG";
static const char* sKeyCreator = "Creator";
static const char* sKeyGroup = "Group";
static const char* sKeyPlfs = "Platforms";


bool ODInst::PkgProps::isPlfIndep( const Platform& reqplf ) const
{
    if ( reqplf.isMac() )
    return false;

    return plfs_.size() == 1 && plfs_[0].isIndep();
}


bool ODInst::PkgProps::isDepOn( const ODInst::PkgProps& pp ) const
{
    if ( deps_[&pp] ) return true;

    for ( int idx=0; idx<deps_.size(); idx++ )
    {
	if ( deps_[idx]->isDepOn( pp ) )
	    return true;
    }

    return false;
}


bool ODInst::PkgProps::isDoc() const
{
    const int sz = pkgnm_.size();
    return sz >=3 && pkgnm_[sz-1] == 'c' && pkgnm_[sz-2] == 'o'
		  && pkgnm_[sz-3] == 'd';
}


void ODInst::PkgProps::getPlfsFromString( const char* str )
{
    FileMultiString fms( str );
    const int nr = fms.size();
    plfs_.erase();
    for ( int idx=0; idx<nr; idx++ )
    {
	BufferString plfstr( fms[idx] );
	if ( plfstr == "*" )
	{
	    plfs_ += mInstPlf(Lin32); plfs_ += mInstPlf(Lin64);
	    plfs_ += mInstPlf(Win32); plfs_ += mInstPlf(Win64);
	    plfs_ += mInstPlf(Mac);
	}
	else if ( plfstr == "Windows" )
	    { plfs_ += mInstPlf(Win32); plfs_ += mInstPlf(Win64); }
	else if ( plfstr == "Linux" )
	    { plfs_ += mInstPlf(Lin32); plfs_ += mInstPlf(Lin64); }
	else if ( plfstr == "-" )
	    plfs_ += Platform();
	else if ( OD::Platform::isValidName(plfstr,true) )
	{
	    OD::Platform plf; plf.set( plfstr, true );
	    plfs_ += plf;
	}
    }
}


ODInst::PkgKey ODInst::PkgProps::pkgKey( const Platform& plf ) const
{
    Platform pl( plf );
    pl.setIsIndep( isPlfIndep(plf) );
    return PkgKey( pkgnm_, pl, ver_ );
}


BufferString ODInst::PkgProps::getURL() const
{
    BufferString ret( creator_->url_ );
    if ( !webpage_.isEmpty() )
	ret.add( "/" ).add( webpage_ );
    return ret;
}


int ODInst::PkgGroup::pkgIdx( const char* pkgnm, bool usr ) const
{
    for ( int ipp=0; ipp<size(); ipp++ )
    {
	const PkgProps& pp( *((*this)[ipp]) );
	const BufferString& cmpstr = usr ? pp.usrname_ : pp.pkgnm_;
	if ( cmpstr == pkgnm )
	    return ipp;
    }

    return -1;
}


ODInst::PkgGroupSet::PkgGroupSet( const IOPar& iop )
{
    BufferStringSet strs; getBSSFromIOP( sKeyCreator, iop, strs );
    for ( int idx=0; idx<strs.size(); idx++ )
    {
	FileMultiString fms( strs.get(idx) );
	const BufferString nm( fms[0] ); const BufferString url( fms[1] );
	cd_ += new CreatorData( nm, url, fms[2] );
    }
    
    strs.erase(); getBSSFromIOP( sKeyLabel, iop, strs );
    for ( int idx=0; idx<strs.size(); idx++ )
    {
	FileMultiString fms( strs.get(idx) );
	double sortval;
	int id;
	if ( getFromString(sortval,fms[2] ) && getFromString( id, fms[0] ) )
	    pkglabels_ += new PkgLabelData( id, fms[1], (float) sortval );
    }

    strs.erase(); getBSSFromIOP( sKeyGroup, iop, strs );
    for ( int idx=0; idx<strs.size(); idx++ )
	*this += new PkgGroup( strs.get(idx) );
}


ODInst::PkgGroupSet::~PkgGroupSet()
{
    deepErase( cd_ );
    deepErase( pkglabels_ );
    deepErase( *this );
}


void ODInst::PkgGroupSet::set( const ObjectSet<IOPar>& iops, bool comm )
{
    for ( int igrp=0; igrp<size(); igrp++ )
	deepErase( *(*this)[igrp] );

    for ( int iiop=0; iiop<iops.size(); iiop++ )
    {
	const IOPar& iop( *iops[iiop] );
	if ( iop.isEmpty() ) continue;

	const char* typstr = iop.find( mGetKeyStr(Type) );
	if ( !typstr || !*typstr ) typstr = "IC";
	if ( !comm && *(typstr+1) == 'C' )
	    continue;

	const char* grpnrstr = iop.find( sKeyGroup );
	int grpnr = grpnrstr && *grpnrstr ? toInt(grpnrstr) : -1;
	if ( grpnr < 1 || grpnr > size() )
	    grpnr = size();

	ODInst::PkgGroup& grp = *(*this)[grpnr-1];
	ODInst::PkgProps* pp = new ODInst::PkgProps;
	pp->internal_ = *typstr == 'I';
	pp->commercial_ = *(typstr+1) == 'C';
	pp->thirdparty_ = *(typstr+2) && *(typstr+2) == 'T';

	for ( int idx=0; idx<iop.size(); idx++ )
	{
	    const BufferString ky( iop.getKey(idx) );
	    const char* val = iop.getValue( idx );

	    if ( ky == mGetKeyStr(Name) )
		pp->usrname_ = val;
	    else if ( ky == sKeyURL )
		pp->webpage_ = val;
	    else if ( ky == sKeyPkgFnmBase )
		pp->pkgnm_ = val;
	    else if ( ky == sKeyPlfs )
		pp->getPlfsFromString( val );
	    else if ( ky == sKeyShortDesc )
		pp->shortdesc_ = val;
	    else if ( ky == sKeyLabels )
	    {
		const FileMultiString lbls = val;
		for ( int idy=0; idy<lbls.size(); idy++ )
		{
		    int lblid;
		    if ( getFromString( lblid, lbls[idy] ) &&
			 getPkgLabel( lblid ) )
			pp->pkglabels_ += getPkgLabel( lblid );
		}
	    }
	    else if ( ky.matches(sKeyCreator) )
	    {
		int nr = toInt(val) - 1;
		if ( nr < 0 || nr >= cd_.size() )
		    nr = 0;
		pp->creator_ = cd_[nr];
	    }
	}
	
	getBSSFromIOP( sKeyDesc, iop, pp->desc_ );
	getBSSFromIOP( sKeyDep, iop, pp->deppkgnms_ );

	grp += pp;
    }

    determineDeps();
}


const ODInst::PkgLabelData* ODInst::PkgGroupSet::getPkgLabel( int id ) const
{
    for ( int idx=0; idx<pkglabels_.size(); idx++ )
    {
	if ( pkglabels_[idx]->id_==id )
	    return pkglabels_[idx];
    }
    
    return 0;
}


void ODInst::PkgGroupSet::setVersions( const BufferStringSet& strs,
       					const ODInst::Platform& plf )
{
    IOPar iop;
    for ( int idx=0; idx<strs.size(); idx++ )
    {
	const PkgKey pk( strs.get(idx) );
	const bool ismatch = pk.plf_.isIndep() || pk.plf_ == plf;
	if ( !ismatch )
	    continue;

	for ( int igrp=0; igrp<size(); igrp++ )
	{
	    PkgGroup& pg = *(*this)[igrp];
	    for ( int ipkg=0; ipkg<pg.size(); ipkg++ )
	    {
		PkgProps& pp = *pg[ipkg];
		if ( pp.pkgnm_ == pk.nm_ )
		    pp.ver_ = pk.ver_;
	    }
	}
    }
}


void ODInst::PkgGroupSet::determineDeps()
{
    for ( int igrp=0; igrp<size(); igrp++ )
    {
	PkgGroup& pg = *(*this)[igrp];
	for ( int ipkg=0; ipkg<pg.size(); ipkg++ )
	{
	    PkgProps& pp = *pg[ipkg];
	    for ( int idep=0; idep<pp.deppkgnms_.size(); idep++ )
	    {
		const PkgProps* ppdep = getPkg(pp.deppkgnms_.get(idep),false);
		if ( ppdep && !pp.deps_[ppdep] )
		    pp.deps_ += ppdep;
		else
		    { pp.deppkgnms_.removeSingle( idep ); idep--; }
	    }
	}
    }
}


int ODInst::PkgGroupSet::groupIdx( const char* nm ) const
{
    for ( int igrp=0; igrp<size(); igrp++ )
    {
	if ( (*this)[igrp]->name_ == nm )
	    return igrp;
    }
    return -1;
}


int ODInst::PkgGroupSet::pkgGroupIdx( const char* nm, bool usrnm ) const
{
    for ( int igrp=0; igrp<size(); igrp++ )
    {
	const PkgGroup& pg = *(*this)[igrp];
	const int pkgidx = pg.pkgIdx( nm, usrnm );
	if ( pkgidx >= 0 )
	    return igrp;
    }
    return -1;
}


const ODInst::PkgProps* ODInst::PkgGroupSet::getPkg( const char* nm,
						     bool usrnm ) const
{
    for ( int igrp=0; igrp<size(); igrp++ )
    {
	const PkgGroup& pg = *(*this)[igrp];
	const int pkgidx = pg.pkgIdx( nm, usrnm );
	if ( pkgidx >= 0 )
	    return pg[pkgidx];
    }
    return 0;
}
