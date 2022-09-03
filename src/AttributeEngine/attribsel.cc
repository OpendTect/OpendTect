/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribsel.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attribstorprovider.h"
#include "bindatadesc.h"
#include "bufstringset.h"
#include "ctxtioobj.h"
#include "globexpr.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seispreload.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "survinfo.h"
#include "zdomain.h"

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
const char* SelSpec::sKeyOnlyStoredData()
				      { return "DescSet only for stored data"; }
static const char* isnnstr = "Is attrib NN"; // for backward compatibility

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
    BufferString storedid = desc.getStoredID();
    if ( storedid.isEmpty() ) return;

    PtrMan<IOObj> ioobj = IOM().get( MultiID(storedid.buf()) );
    if ( !ioobj )
	return;

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
    id_ = DescID(nr,false);
    setRefFromID( nlamod );
}


void SelSpec::setDiscr( const NLAModel& nlamod )
{
    if ( nlamod.design().isSupervised() )
	discrspec_ = StepInterval<int>(0,0,0);
    else
    {
	discrspec_.start = discrspec_.step = 1;
	discrspec_.stop = nlamod.design().hiddensz_.first();
    }
}


void SelSpec::setDiscr( const DescSet& )
{
    discrspec_ = StepInterval<int>(0,0,0);
}


void SelSpec::setIDFromRef( const NLAModel& nlamod )
{
    isnla_ = true;
    const int nrout = nlamod.design().outputs_.size();
    id_ = cNoAttrib();
    for ( int idx=0; idx<nrout; idx++ )
    {
	if ( ref_ == *nlamod.design().outputs_[idx] )
	    { id_ = DescID(idx,false); break; }
    }
    setDiscr( nlamod );
}


void SelSpec::setIDFromRef( const DescSet& ds )
{
    isnla_ = false;
    const bool isstored = isStored();
    id_ = ds.getID( ref_, true );
    if ( id_ == DescID::undef() )
	id_ = ds.getID( objref_, true );

    if ( id_ == DescID::undef() && isstored )
    {
	// ref_ may have changed, identify with MultiID in defstring_
	BufferString midstr;
	if ( Desc::getParamString(defstring_,StorageProvider::keyStr(),midstr) )
	{
	    int compnr = 0;
	    BufferString compstr;
	    if ( Desc::getParamString(defstring_,Desc::sKeyOutput(),compstr) )
		getFromString( compnr, compstr, 0 );

	    id_ = ds.getStoredID( MultiID(midstr.buf()), compnr );
	    if ( id_ != DescID::undef() )
		setRefFromID( ds );
	}
    }

    BufferString attribname;
    if ( Desc::getAttribName( defstring_.buf(), attribname ) )
    {
	if ( ds.getDesc(id_) &&
	     attribname!=ds.getDesc(id_)->attribName() )
	    id_ = ds.getID( defstring_, ds.containsStoredDescOnly() );
    }
    const Desc* desc = ds.getDesc( id_ );
    if ( desc )
	setZDomainKey( *desc );
    setDiscr( ds );
}


void SelSpec::setRefFromID( const NLAModel& nlamod )
{
    ref_ = id_.asInt() >= 0 ? nlamod.design().outputs_[id_.asInt()]->buf() : "";
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
	    const MultiID mid( desc->getStoredID(false).buf() );
	    PtrMan<IOObj> ioobj = IOM().get( mid );
	    if ( ioobj )
	    {
		Desc* ncdesc = const_cast<Desc*>( desc );
		const LineKey lk( desc->userRef() );
		const BufferString component = lk.attrName();
		if ( component.isEmpty() )
		    ncdesc->setUserRef( ioobj->name() );
		else
		    ncdesc->setUserRef( LineKey(ioobj->name(),component) );
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
    par.set( sKeyRef(), ref_ );
    par.set( sKeyID(), id_.asInt() );
    par.setYN( sKeyOnlyStoredData(), id_.isStored() );
    par.setYN( sKeyIsNLA(), isnla_ );
    par.set( sKeyObjRef(), objref_ );
    par.set( sKeyDefStr(), defstring_ );
    par.set( ZDomain::sKey(), zdomainkey_ );
    par.setYN( sKeyIs2D(), is2d_ );
}


bool SelSpec::usePar( const IOPar& par )
{
    ref_ = "";			par.get( sKeyRef(), ref_ );
    id_ = cNoAttrib();		par.get( sKeyID(), id_.asInt() );
    bool isstored = false;	par.getYN( sKeyOnlyStoredData(), isstored );
    id_.setStored( isstored );
    isnla_ = false;		par.getYN( sKeyIsNLA(), isnla_ );
				par.getYN( isnnstr, isnla_ );
    objref_ = "";		par.get( sKeyObjRef(), objref_ );
    defstring_ = "";		par.get( sKeyDefStr(), defstring_ );
    zdomainkey_ = "";		if ( !par.get( ZDomain::sKey(), zdomainkey_ ) )
				    par.get( "Depth Domain", zdomainkey_);
    is2d_ = false;		par.getYN( sKeyIs2D(), is2d_ );

    return true;
}


bool SelSpec::isStored() const
{
    return id_.isValid() && id_.isStored();
}


const BinDataDesc* SelSpec::getPreloadDataDesc( Pos::GeomID geomid ) const
{
    const DescSet* descset = DSHolder().getDescSet( false, isStored() );
    if ( !descset || descset->isEmpty() )
	return 0;

    const Desc* desc = descset->getDesc( id() );
    if ( !desc )
	return 0;

    const MultiID mid( desc->getStoredID().buf() );
    auto sdp = Seis::PLDM().get<SeisDataPack>( mid, geomid );

    return sdp ? &sdp->getDataDesc() : 0;
}


// Attrib::SelInfo
SelInfo::SelInfo( const DescSet* attrset, const NLAModel* nlamod,
		  bool is2d, const DescID& ignoreid, bool usesteering,
		  bool onlysteering, bool onlymulticomp, bool usehidden )
    : is2d_( is2d )
    , usesteering_( usesteering )
    , onlysteering_( onlysteering )
    , onlymulticomp_( onlymulticomp )
{
    fillStored( false );
    fillStored( true );

    if ( attrset )
    {
	for ( int idx=0; idx<attrset->size(); idx++ )
	{
	    const DescID descid = attrset->getID( idx );
	    const Desc* desc = attrset->getDesc( descid );
	    const BufferString usrref( desc ? desc->userRef()
					    : sKey::EmptyString().buf() );
	    if ( !desc || usrref.isEmpty()
	      || desc->attribName()==StorageProvider::attribName()
	      || attrset->getID(*desc) == ignoreid
	      || ( !usehidden && desc->isHidden() ) )
		continue;

	    attrids_ += descid;
	    attrnms_.add( usrref );
	}
    }

    if ( nlamod )
    {
	const int nroutputs = nlamod->design().outputs_.size();
	for ( int idx=0; idx<nroutputs; idx++ )
	{
	    BufferString nm( *nlamod->design().outputs_[idx] );
	    if ( IOObj::isKey(nm) )
		nm = IOM().nameOf( nm.buf() );

	    nlaoutnms_.add( nm );
	}
    }
}


void SelInfo::fillStored( bool steerdata, const char* filter )
{
    BufferStringSet& nms = steerdata ? steernms_ : ioobjnms_;
    TypeSet<MultiID>& ids = steerdata ? steerids_ : ioobjids_;
    nms.erase(); ids.erase();
    BufferStringSet ioobjnmscopy;
    TypeSet<MultiID> ioobjidscopy;

    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Seis)->id_ );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();

    BufferString filterstr = filter;
    GlobExpr::validateFilterString( filterstr );
    GlobExpr ge( filterstr );

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( *ioobj.group() == 'W' ) continue;
	if ( SeisTrcTranslator::isPS(ioobj,true) ) continue;
	const bool is2d = SeisTrcTranslator::is2D(ioobj,true);
	const bool islineset = SeisTrcTranslator::isLineSet(ioobj);
	const bool isvalid3d = !is2d  && !islineset && ioobj.isUserSelectable();

	if ( (is2d_ != is2d) || (!is2d && !isvalid3d) )
	    continue;

	if ( !ZDomain::isSI(ioobj.pars()) )
	    continue;

	const BufferString res = ioobj.pars().find( sKey::Type() );
	if ( !res.isEmpty() && ( (!steerdata && res.isEqual(sKey::Steering()) )
		 || ( steerdata && !res.isEqual(sKey::Steering()) ) ) )
	    continue;
	else if ( res.isEmpty() && steerdata )
	    continue;

	const char* ioobjnm = ioobj.name().buf();
	if ( !ge.matches(ioobjnm) )
	    continue;

	if ( onlymulticomp_ )
	{
	    if ( is2d )
	    {
		BufferStringSet attrnms;
		SelInfo::getAttrNames( ioobj.key(), attrnms, steerdata, true );
		if ( attrnms.isEmpty() )
		    continue;
	    }
	    else
	    {
		if ( SeisIOObjInfo(ioobj).nrComponents() < 2 )
		    continue;
	    }
	}

	ioobjnmscopy.add( ioobjnm );
	ioobjidscopy.add( ioobj.key() );
    }

    if ( ioobjnmscopy.size() > 1 )
    {
	int* sortindexes = ioobjnmscopy.getSortIndexes();
	for ( int idx=0; idx<ioobjnmscopy.size(); idx++ )
	{
	    nms.add( ioobjnmscopy.get(sortindexes[idx]) );
	    ids.add( ioobjidscopy.get(sortindexes[idx]) );
	}

	delete [] sortindexes;
    }
    else if ( ioobjnmscopy.size() )
    {
	nms.add( ioobjnmscopy.get(0) );
	ids.add( ioobjidscopy.get(0) );
    }
}


SelInfo::SelInfo( const SelInfo& asi )
	: ioobjnms_(asi.ioobjnms_)
	, ioobjids_(asi.ioobjids_)
	, attrnms_(asi.attrnms_)
	, attrids_(asi.attrids_)
	, nlaoutnms_(asi.nlaoutnms_)
	, is2d_(asi.is2d_)
	, usesteering_(asi.usesteering_)
	, onlysteering_(asi.onlysteering_)
{
}


SelInfo& SelInfo::operator=( const SelInfo& asi )
{
    ioobjnms_ = asi.ioobjnms_;
    ioobjids_ = asi.ioobjids_;
    attrnms_ = asi.attrnms_;
    attrids_ = asi.attrids_;
    nlaoutnms_ = asi.nlaoutnms_;
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
			    bool issteer, bool onlymulticomp )
{
    const MultiID key( LineKey(defstr).lineName().buf() );
    getAttrNames( key, nms, issteer, onlymulticomp );
}


void SelInfo::getAttrNames( const MultiID& key, BufferStringSet& nms,
			    bool issteer, bool onlymulticomp )

{
    nms.erase();
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj || !SeisTrcTranslator::is2D(*ioobj,true) )
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


void SelInfo::getZDomainItems( const ZDomain::Info& zinf,
			       BufferStringSet& nms )
{
    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Seis)->id_ );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( ioobj.isUserSelectable() && zinf.isCompatibleWith(ioobj.pars()) )
	    nms.add( ioobj.name() );
    }

    nms.sort();
}


void SelInfo::getZDomainItems( const ZDomain::Info& zinf, bool need2d,
			       BufferStringSet& nms )
{
    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Seis)->id_ );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	const bool is2d = SeisTrcTranslator::is2D( ioobj, true );
	if ( need2d==is2d && ioobj.isUserSelectable() &&
		zinf.isCompatibleWith(ioobj.pars()) )
	    nms.add( ioobj.name() );
    }

    nms.sort();
}

} // namespace Attrib
