/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: attribsel.cc,v 1.34 2009-03-27 15:37:35 cvshelene Exp $";

#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribstorprovider.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "ptrman.h"
#include "ctxtioobj.h"
#include "nlamodel.h"
#include "nladesign.h"
#include "bufstringset.h"
#include "keystrs.h"
#include "linekey.h"
#include "globexpr.h"

namespace Attrib
{
#define mDescIDRet(val) static DescID res(val,true); return res


const DescID& SelSpec::cOtherAttrib()	{ mDescIDRet(-1); }
const DescID& SelSpec::cNoAttrib()	{ mDescIDRet(-2); }
const DescID& SelSpec::cAttribNotSel()	{ mDescIDRet(-3); }

const char* SelSpec::sKeyRef()		{ return "Attrib Reference"; }
const char* SelSpec::sKeyID()		{ return "Attrib ID"; }
const char* SelSpec::sKeyIsNLA()	{ return "Is attrib NLA Model"; }
const char* SelSpec::sKeyObjRef()	{ return "Object Reference"; }
const char* SelSpec::sKeyDefStr()	{ return "Definition"; }
const char* SelSpec::sKeyIs2D()		{ return "Is 2D"; }
static const char* isnnstr = "Is attrib NN"; // for backward compatibility

bool SelSpec::operator==( const SelSpec& ss ) const
{
    return id()==ss.id() && isNLA()==ss.isNLA() && 
	   ss.ref_==ref_ && ss.objref_==objref_;
}


bool SelSpec::operator!=( const SelSpec& ss ) const
{
    return !(*this==ss);
}


void SelSpec::setDepthDomainKey( const Desc& desc )
{
    zdomainkey_ = "";

    BufferString storedid = desc.getStoredID();
    if ( storedid.isEmpty() ) return;

    PtrMan<IOObj> ioobj = IOM().get( MultiID(storedid.buf()) );
    if ( ioobj )
    {
	if ( !ioobj->pars().get( sKey::ZDomain, zdomainkey_ ) )
	{
	    //Legacy. Can be removed in od4
	    ioobj->pars().get( "Depth Domain", zdomainkey_ );
	}
    }
}


void SelSpec::set( const Desc& desc )
{
    isnla_ = false;
    is2d_ = desc.is2D();
    id_ = desc.id();
    ref_ = desc.userRef();
    desc.getDefStr( defstring_ );
    setDepthDomainKey( desc );
    setDiscr( *desc.descSet() );
}


void SelSpec::set( const NLAModel& nlamod, int nr )
{
    isnla_ = true;
    id_ = DescID(nr,true);
    setRefFromID( nlamod );
}


void SelSpec::setDiscr( const NLAModel& nlamod )
{
    if ( nlamod.design().isSupervised() )
	discrspec_ = StepInterval<int>(0,0,0);
    else
    {
	discrspec_.start = discrspec_.step = 1;
	discrspec_.stop = nlamod.design().hiddensz;
    }
}


void SelSpec::setDiscr( const DescSet& ds )
{
    discrspec_ = StepInterval<int>(0,0,0);
}


void SelSpec::setIDFromRef( const NLAModel& nlamod )
{
    isnla_ = true;
    const int nrout = nlamod.design().outputs.size();
    id_ = cNoAttrib();
    for ( int idx=0; idx<nrout; idx++ )
    {
	if ( ref_ == *nlamod.design().outputs[idx] )
	    { id_ = DescID(idx,true); break; }
    }
    setDiscr( nlamod );
}


void SelSpec::setIDFromRef( const DescSet& ds )
{
    isnla_ = false;
    id_ = ds.getID( ref_, true );
    BufferString attribname;
    if ( Desc::getAttribName( defstring_.buf(), attribname ) )
    {
	if ( ds.getDesc(id_) && 
	     strcmp( attribname, ds.getDesc(id_)->attribName() ) )
	    id_ = ds.getID( defstring_, false );
    }
     /*TODO: make it work 100% : doesn't work if attribute and stored data 
    have the same name and the stored data is the FIRST thing you try
    to display*/
    const Desc* desc = ds.getDesc( id_ );
    if ( desc )
	setDepthDomainKey( *desc );
    setDiscr( ds );
}


void SelSpec::setRefFromID( const NLAModel& nlamod )
{
    ref_ = id_.asInt() >= 0 ? nlamod.design().outputs[id_.asInt()]->buf() : "";
    setDiscr( nlamod );
}


void SelSpec::setRefFromID( const DescSet& ds )
{
    const Desc* desc = ds.getDesc( id_ );
    ref_ = "";
    if ( desc )
    {
	ref_ = desc->userRef();
	desc->getDefStr( defstring_ );
	setDepthDomainKey( *desc );
    }
    setDiscr( ds );
}


void SelSpec::fillPar( IOPar& par ) const
{
    par.set( sKeyRef(), ref_ );
    par.set( sKeyID(), id_.asInt() );
    par.setYN( sKeyIsNLA(), isnla_ );
    par.set( sKeyObjRef(), objref_ );
    par.set( sKeyDefStr(), defstring_ );
    par.set( sKey::ZDomain, zdomainkey_ );
    par.setYN( sKeyIs2D(), is2d_ );
}


bool SelSpec::usePar( const IOPar& par )
{
    ref_ = ""; 			par.get( sKeyRef(), ref_ );
    id_ = cNoAttrib();		par.get( sKeyID(), id_.asInt() );
    isnla_ = false; 		par.getYN( sKeyIsNLA(), isnla_ );
    				par.getYN( isnnstr, isnla_ );
    objref_ = "";		par.get( sKeyObjRef(), objref_ );
    defstring_ = "";		par.get( sKeyDefStr(), defstring_ );
    zdomainkey_ = "";		if ( !par.get( sKey::ZDomain, zdomainkey_ ) )
				    par.get( "Depth Domain", zdomainkey_);
    is2d_ = false;		par.getYN( sKeyIs2D(), is2d_ );
    		
    return true;
}



SelInfo::SelInfo( const DescSet* attrset, const NLAModel* nlamod, 
		  bool is2d, const DescID& ignoreid, bool usesteering,
       		  bool onlysteering )
    : is2d_( is2d )
    , usesteering_( usesteering )
    , onlysteering_( onlysteering )
{
    fillStored();

    if ( attrset )
    {
	for ( int idx=0; idx<attrset->nrDescs(); idx++ )
	{
	    const DescID descid = attrset->getID( idx );
	    const Desc* desc = attrset->getDesc( descid );
	    if ( !desc || 
		 !strcmp(desc->attribName(),StorageProvider::attribName()) || 
		 attrset->getID(*desc) == ignoreid || desc->isHidden() )
		continue;

	    attrids += descid;
	    attrnms.add( desc->userRef() );
	}
    }

    if ( nlamod )
    {
	const int nroutputs = nlamod->design().outputs.size();
	for ( int idx=0; idx<nroutputs; idx++ )
	{
	    BufferString nm( *nlamod->design().outputs[idx] );
	    if ( IOObj::isKey(nm) )
		nm = IOM().nameOf( nm, false );
	    nlaoutnms.add( nm );
	}
    }
}


void SelInfo::fillStored( const char* filter )
{
    ioobjnms.erase(); ioobjids.erase();

    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    GlobExpr* ge = filter && *filter ? new GlobExpr( filter ) : 0;

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( *ioobj.group() == 'W' ) continue;
	if ( SeisTrcTranslator::isPS( ioobj ) ) continue;
	const bool is2d = SeisTrcTranslator::is2D(ioobj,true);
	const bool isvalid3d = !is2d && ioobj.isReadDefault();
	const bool isz = ioobj.pars().find(sKey::ZDomain) ||
	    		 ioobj.pars().find( "Depth Domain" );
	if ( isz || (is2d_ != is2d) || (!is2d && !isvalid3d) )
	    continue;

	const char* res = ioobj.pars().find( sKey::Type );
	if ( res && ( (!usesteering_ && !strcmp(res,sKey::Steering) )
	         || ( onlysteering_ && strcmp(res,sKey::Steering) ) ) )
	    continue;

	if ( !res && onlysteering_ ) continue;

	const char* ioobjnm = ioobj.name().buf();
	if ( ge && !ge->matches(ioobjnm) )
	    continue;

	ioobjnms.add( ioobjnm );
	ioobjids.add( (const char*)ioobj.key() );
	if ( ioobjnms.size() > 1 )
	{
	    for ( int icmp=ioobjnms.size()-2; icmp>=0; icmp-- )
	    {
		if ( ioobjnms.get(icmp) > ioobjnms.get(icmp+1) )
		{
		    ioobjnms.swap( icmp, icmp+1 );
		    ioobjids.swap( icmp, icmp+1 );
		}
	    }
	}
    }
}


SelInfo::SelInfo( const SelInfo& asi )
	: ioobjnms(asi.ioobjnms)
	, ioobjids(asi.ioobjids)
	, attrnms(asi.attrnms)
	, attrids(asi.attrids)
	, nlaoutnms(asi.nlaoutnms)
	, is2d_(asi.is2d_)
	, usesteering_(asi.usesteering_)
	, onlysteering_(asi.onlysteering_)
{
}


SelInfo& SelInfo::operator=( const SelInfo& asi )
{
    ioobjnms = asi.ioobjnms;
    ioobjids = asi.ioobjids;
    attrnms = asi.attrnms;
    attrids = asi.attrids;
    nlaoutnms = asi.nlaoutnms;
    is2d_ = asi.is2d_;
    usesteering_ = asi.usesteering_;
    onlysteering_ = asi.onlysteering_;
    return *this;
}


bool SelInfo::is2D( const char* defstr )
{
    PtrMan<IOObj> ioobj = IOM().get( MultiID(LineKey(defstr).lineName().buf()));
    return ioobj ? SeisTrcTranslator::is2D(*ioobj,true) : false;
}


void SelInfo::getAttrNames( const char* defstr, BufferStringSet& nms, 
			    bool issteer )
{
    nms.erase();
    PtrMan<IOObj> ioobj = IOM().get( MultiID(LineKey(defstr).lineName().buf()));
    if ( !ioobj || !SeisTrcTranslator::is2D(*ioobj,true) )
	return;

    Seis2DLineSet ls( ioobj->fullUserExpr(true) );
    ls.getAvailableAttributes( nms, sKey::Steering, !issteer, issteer );
}


void SelInfo::getSpecialItems( const char* zdomainkey,
			       BufferStringSet& nms )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    BufferStringSet ioobjnms;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	const char* zres = ioobj.pars().find( sKey::ZDomain );
	if ( zres && !strcmp(zres,zdomainkey) )
	    nms.add( ioobj.name() );
	else //Legacy
	{
	    const char* dres = ioobj.pars().find( "Depth Domain" );
	    if ( dres && !strcmp(dres,zdomainkey) )
		nms.add( ioobj.name() );
	}
    }

    nms.sort();
}


}; // namespace Attrib
