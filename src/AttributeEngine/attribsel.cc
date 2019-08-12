/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/

#include "attribsel.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribstorprovider.h"
#include "bindatadesc.h"
#include "bufstringset.h"
#include "ioobjctxt.h"
#include "globexpr.h"
#include "datapackbase.h"
#include "dbdir.h"
#include "dbman.h"
#include "iopar.h"
#include "keystrs.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seispreload.h"
#include "seis2ddata.h"
#include "survinfo.h"
#include "zdomain.h"

namespace Attrib
{

const char* SelSpec::sKeyRef()		{ return "Attrib Reference"; }
const char* SelSpec::sKeyID()		{ return "Attrib ID"; }
const char* SelSpec::sKeyIsNLA()	{ return "Is attrib NLA Model"; }
const char* SelSpec::sKeyObjRef()	{ return "Object Reference"; }
const char* SelSpec::sKeyDefStr()	{ return "Definition"; }
const char* SelSpec::sKeyIs2D()		{ return "Is 2D"; }
static const char* sKeyIsNN_old =	"Is attrib NN";

bool SelSpec::operator==( const SelSpec& ss ) const
{
    return id()==ss.id() && isNLA()==ss.isNLA() && ss.ref_==ref_ &&
	ss.objref_==objref_ && ss.defstring_==defstring_ && is2D()==ss.is2D();
}


bool SelSpec::operator!=( const SelSpec& ss ) const
{
    return !(*this==ss);
}


void SelSpec::setZDomainKey( const Desc& desc )
{
    zdomainkey_.setEmpty();
    const DBKey storedid = desc.getStoredID();
    if ( storedid.isInvalid() )
	return;

    PtrMan<IOObj> ioobj = storedid.getIOObj();
    if ( ioobj )
	ioobj->pars().get( ZDomain::sKey(), zdomainkey_ );
}


bool SelSpec::isZTransformed() const
{
    return !zdomainkey_.isEmpty() && zdomainkey_!=ZDomain::SI().key();
}


void SelSpec::set( const Desc& desc )
{
    isnla_ = false;
    is2d_ = desc.is2D();
    id_ = desc.id();
    ref_ = desc.userRef();
    desc.getDefStr( defstring_ );
    setZDomainKey( desc );
    setDiscr( *desc.descSet() );
}


void SelSpec::set( const NLAModel& nlamod, int nr )
{
    isnla_ = true;
    id_ = DescID( nr );
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
    id_ = cNoAttribID();
    for ( int idx=0; idx<nrout; idx++ )
    {
	if ( ref_ == *nlamod.design().outputs[idx] )
	    { id_ = DescID(idx); break; }
    }
    setDiscr( nlamod );
}


void SelSpec::setIDFromRef( const DescSet& ds )
{
    isnla_ = false;
    id_ = ds.getID( ref_, true );
    if ( !id_.isValid() )
	id_ = ds.getID( objref_, true );
    BufferString attribname;
    if ( Desc::getAttribName( defstring_.buf(), attribname ) )
    {
	if ( ds.getDesc(id_) &&
	     attribname != ds.getDesc(id_)->attribName() )
	    id_ = ds.getID( defstring_, false );
    }
    const Desc* desc = ds.getDesc( id_ );
    if ( desc )
	setZDomainKey( *desc );
    setDiscr( ds );
}


void SelSpec::setRefFromID( const NLAModel& nlamod )
{
    ref_ = id_.getI() >= 0 ? nlamod.design().outputs[id_.getI()]->buf() : "";
    setDiscr( nlamod );
}


void SelSpec::setRefFromID( const DescSet& ds )
{
    const Desc* desc = ds.getDesc( id_ );
    ref_ = "";
    if ( desc )
    {
	if ( desc->isStored() )
	{
	    const DBKey dbky = desc->getStoredID( false );
	    PtrMan<IOObj> ioobj = dbky.getIOObj();
	    if ( ioobj )
	    {
		Desc* ncdesc = const_cast<Desc*>( desc );
		const StringPair usrref( desc->userRef() );
		const BufferString component = usrref.second();
		if ( component.isEmpty() )
		    ncdesc->setUserRef( ioobj->name() );
		else
		{
		    const StringPair compstr( ioobj->name(), component );
		    ncdesc->setUserRef( compstr.getCompString() );
		}
	    }
	}

	ref_ = desc->userRef();
	desc->getDefStr( defstring_ );
	setZDomainKey( *desc );
    }
    setDiscr( ds );
}


void SelSpec::fillPar( IOPar& par ) const
{
    par.set( sKeyID(), id_ );
    par.setYN( sKeyIs2D(), is2d_ );
    par.setYN( sKeyIsNLA(), isnla_ );
    par.set( sKeyRef(), ref_ );
    par.set( sKeyObjRef(), objref_ );
    par.set( sKeyDefStr(), defstring_ );
    par.set( ZDomain::sKey(), zdomainkey_ );
}


void SelSpec::usePar( const IOPar& par )
{
    par.get( sKeyID(), id_ );
    par.getYN( sKeyIs2D(), is2d_ );
    if ( !par.getYN( sKeyIsNLA(), isnla_ ) )
	par.getYN( sKeyIsNN_old, isnla_ );
    par.get( sKeyRef(), ref_ );
    par.get( sKeyObjRef(), objref_ );
    par.get( sKeyDefStr(), defstring_ );
    if ( !par.get( ZDomain::sKey(), zdomainkey_ ) )
	par.get( "Depth Domain", zdomainkey_);
}


const Desc* SelSpec::getDesc( const DescSet* descset ) const
{
    return descset ? descset->getDesc( id_ ) : DescSet::getGlobalDesc( *this );
}


bool SelSpec::isStored( const DescSet* descset ) const
{
    const Desc* desc = getDesc( descset );
    return desc ? desc->isStored() : false;
}


const BinDataDesc* SelSpec::getPreloadDataDesc( Pos::GeomID geomid,
						const DescSet* descset ) const
{
    const Desc* desc = getDesc( descset );
    if ( !desc )
	return 0;

    const DBKey dbky = desc->getStoredID();
    auto vdp = Seis::PLDM().get<VolumeDataPack>( dbky, geomid );

    return vdp ? &vdp->getDataDesc() : 0;
}


bool SelSpec::isUsable() const
{
    if ( id_ == cOtherAttribID() )
    {
	return true;	// ?
    }
    else if ( id_ == cExternalAttribID() )
    {
	if ( ref_ != "VolProc" )
	    return false;

	//TODO: check mid against DBMan
	return true;
    }

    return id_.isValid();
}


// Attrib::SelInfo

SelInfo::SelInfo( const DescSet& attrset, const DescID& ignoreid,
		  const NLAModel* nlamod, const ZDomain::Info* zdominf )
    : is2d_( attrset.is2D() )
    , onlymulticomp_( false )
{
    fillStored( false, 0, zdominf );
    fillStored( true, 0, zdominf );
    fillNonStored( attrset, ignoreid, nlamod, false );
}


SelInfo::SelInfo( const DescSet* attrset, const DescID& ignoreid,
		  const NLAModel* nlamod, bool is2d, bool onlymulticomp,
		  bool usehidden )
    : is2d_( is2d )
    , onlymulticomp_( onlymulticomp )
{
    fillStored( false );
    fillStored( true );

    if ( attrset )
	fillNonStored( *attrset, ignoreid, nlamod, false );
}


SelInfo::SelInfo( const ZDomain::Info& zdominf, bool is2d )
    : is2d_( is2d )
    , onlymulticomp_( false )
{
    fillStored( false, 0, &zdominf );
}


static void transferSorted( const BufferStringSet innms, const DBKeySet inids,
			    BufferStringSet& outnms, DBKeySet& outids )
{
    outnms.erase(); outids.erase();

    const int sz = inids.size();
    if ( sz == 1 )
    {
	outids.add( inids.get(0) );
	outnms.add( innms.get(0) );
    }
    else if ( sz > 1 )
    {
	int* sortindexes = innms.getSortIndexes();
	for ( int idx=0; idx<innms.size(); idx++ )
	{
	    outids.add( inids.get(sortindexes[idx]) );
	    outnms.add( innms.get(sortindexes[idx]) );
	}

	delete [] sortindexes;
    }
}


void SelInfo::fillStored( bool steerdata, const char* filter,
			  const ZDomain::Info* zdominf )
{
    BufferStringSet ioobjnms; DBKeySet ioobjids;

    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Seis );
    if ( dbdir )
    {
	DBDirIter iter( *dbdir );
	GlobExpr* ge = filter && *filter ? new GlobExpr( filter ) : 0;
	while ( iter.next() )
	{
	    const IOObj& ioobj = iter.ioObj();
	    if ( *ioobj.group() == 'W' )
		continue;
	    const SeisIOObjInfo info( ioobj );
	    if ( info.isPS() )
		continue;
	    const bool is2d = info.is2D();
	    const bool isvalid3d = !is2d && ioobj.isUserSelectable();

	    if ( (is2d_ != is2d) || (!is2d && !isvalid3d) )
		continue;

	    if ( zdominf )
	    {
		if ( !zdominf->isCompatibleWith(ioobj.pars()) )
		    continue;
	    }
	    else if ( !ZDomain::isSI(ioobj.pars()) )
		continue;

	    FixedString res = ioobj.pars().find( sKey::Type() );
	    if ( res && ( (!steerdata && res==sKey::Steering() )
		     || ( steerdata && res!=sKey::Steering() ) ) )
		continue;
	    else if ( !res && steerdata )
		continue;

	    const char* ioobjnm = ioobj.name().buf();
	    if ( ge && !ge->matches(ioobjnm) )
		continue;

	    if ( onlymulticomp_ )
	    {
		if ( is2d )
		{
		    BufferStringSet attrnms;
		    getAttrNames( ioobj.key().toString(), attrnms,
					    steerdata, true );
		    if ( attrnms.isEmpty() )
			continue;
		}
		else
		{
		    if ( SeisIOObjInfo(ioobj).nrComponents() < 2 )
			continue;
		}
	    }

	    ioobjnms.add( ioobjnm );
	    ioobjids.add( ioobj.key() );
	}
    }

    BufferStringSet& nms = steerdata ? steernms_ : ioobjnms_;
    DBKeySet& ids = steerdata ? steerids_ : ioobjids_;
    transferSorted( ioobjnms, ioobjids, nms, ids );
}


void SelInfo::fillSynthetic( bool steerdata,
			     const TypeSet<DataPack::FullID>& dpids )
{
    BufferStringSet ioobjnms; DBKeySet ioobjids;
    for ( int idx=0; idx<dpids.size(); idx++ )
    {
	ioobjnms.add( DataPackMgr::nameOf( dpids[idx] ) );
	DBKey dbky; dpids[idx].putInDBKey( dbky );
	ioobjids.add( dbky );
    }

    BufferStringSet& nms = steerdata ? steernms_ : ioobjnms_;
    DBKeySet& ids = steerdata ? steerids_ : ioobjids_;
    transferSorted( ioobjnms, ioobjids, nms, ids );
}


void SelInfo::fillNonStored( const DescSet& attrset, const DescID& ignoreid,
			     const NLAModel* nlamod, bool usehidden )
{
    attrids_.setEmpty(); attrnms_.setEmpty(); nlaoutnms_.setEmpty();

    for ( int idx=0; idx<attrset.size(); idx++ )
    {
	DescID descid = attrset.getID( idx );
	const Desc* desc = attrset.getDesc( descid );
	if ( desc
	  && !desc->isStored()
	  && desc->id() != ignoreid
	  && (usehidden || !desc->isHidden()) )
	{
	    attrids_ += descid;
	    attrnms_.add( desc->userRef() );
	}
    }

    if ( nlamod )
    {
	const int nroutputs = nlamod->design().outputs.size();
	for ( int idx=0; idx<nroutputs; idx++ )
	{
	    BufferString nm( *nlamod->design().outputs[idx] );
	    if ( DBKey::isValidString(nm) )
		nm = DBKey(nm).name();
	    nlaoutnms_.add( nm );
	}
    }
}


void SelInfo::getAttrNames( const char* defstr, BufferStringSet& nms,
			    bool issteer, bool onlymulticomp )
{
    nms.erase();
    PtrMan<IOObj> ioobj = DBKey(defstr).getIOObj();
    if ( !ioobj || !SeisIOObjInfo(ioobj).is2D() )
	return;

    SeisIOObjInfo info( ioobj );
    SeisIOObjInfo::Opts2D opt;
    opt.steerpol_ = issteer ? 1 : 0;
    info.getLineNames( nms, opt );

    if ( onlymulticomp )
    {
	for ( int idx=nms.size()-1; idx>=0; idx-- )
	{
	    if ( SeisIOObjInfo(*ioobj).nrComponents() < 2 )
		nms.removeSingle( idx );
	}
    }
}


} // namespace Attrib
