/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribdescset.cc,v 1.66 2008-04-11 12:05:01 cvsbert Exp $";

#include "attribdescset.h"
#include "attribstorprovider.h"
#include "attribparam.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribsel.h"
#include "bufstringset.h"
#include "keystrs.h"
#include "separstr.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "separstr.h"
#include "gendefs.h"
#include "seisioobjinfo.h"

namespace Attrib
{

DescSet* DescSet::clone() const
{
    DescSet* descset = new DescSet(is2d_,is2dset_);
    for ( int idx=0; idx<nrDescs(); idx++ )
    {
	Desc* nd = new Desc( *descs[idx] );
	nd->setDescSet( descset );
	descset->addDesc( nd, ids[idx] );
    }

    descset->updateInputs();
    return descset;
}


void DescSet::updateInputs()
{
    for ( int idx=0; idx<nrDescs(); idx++ )
    {
	Desc& dsc = *descs[idx];
	for ( int inpidx=0; inpidx<dsc.nrInputs(); inpidx++ )
	{
	    const Desc* oldinpdesc = dsc.getInput( inpidx );
	    if ( !oldinpdesc ) continue;
	    Desc* newinpdesc = getDesc( oldinpdesc->id() );
	    dsc.setInput( inpidx, newinpdesc );
	}
    }
}


DescID DescSet::addDesc( Desc* nd, DescID id )
{
    nd->setDescSet( this );
    nd->ref();
    descs += nd;
    const DescID newid = id < 0 ? getFreeID() : id;
    ids += newid;
    return newid;
}


DescID DescSet::insertDesc( Desc* nd, int idx, DescID id )
{
    nd->setDescSet( this );
    nd->ref();
    descs.insertAt( nd, idx );
    const DescID newid = id < 0 ? getFreeID() : id;
    ids.insert( idx, newid );
    return newid;
}


Desc* DescSet::getDesc( const DescID& id )
{
    const int idx = ids.indexOf(id);
    if ( idx==-1 ) return 0;
    return descs[idx];
}


const Desc* DescSet::getDesc( const DescID& id ) const
{
    return const_cast<DescSet*>(this)->getDesc(id);
}


int DescSet::nrDescs( bool incstored, bool inchidden ) const
{
    int ret = descs.size();
    if ( !incstored || !inchidden )
    {
	for ( int idx=0; idx<descs.size(); idx++ )
	{
	    const Desc& dsc = *descs[idx];
	    if ( !incstored && dsc.isStored() )
		ret--;
	    else if ( !inchidden && dsc.isHidden() )
		ret--;
	}
    }
    return ret;
}


DescID DescSet::getID( const Desc& dsc ) const
{
    const int idx = descs.indexOf( &dsc );
    return idx==-1 ? DescID::undef() : ids[idx];
}


DescID DescSet::getID( int idx ) const
{
    if ( idx < 0 || idx >= ids.size() ) return DescID::undef();
    return ids[idx];
}


void DescSet::getIds( TypeSet<DescID>& attribids ) const
{ attribids = ids; }


void DescSet::getStoredIds( TypeSet<DescID>& attribids ) const
{
    for ( int idx=0; idx<descs.size(); idx++ )
    {
	if ( descs[idx]->isStored() )
	    attribids += ids[idx];
    }
}


DescID DescSet::getID( const char* str, bool isusrref ) const
{
    if ( !str || !*str ) return DescID::undef();

    for ( int idx=0; idx<nrDescs(); idx++ )
    {
	const Desc& dsc = *descs[idx];
	if ( isusrref && dsc.isIdentifiedBy(str) )
	    return ids[idx];
	else if ( !isusrref )
	{
	    BufferString defstr;
	    dsc.getDefStr( defstr );
	    if ( defstr == str )
		return ids[idx];
	}
    }

    return DescID::undef();
}


void DescSet::removeDesc( const DescID& id )
{
    const int idx = ids.indexOf(id);
    if ( idx==-1 ) return;

    descToBeRemoved.trigger( id );
    if ( descs[idx]->descSet()==this )
	descs[idx]->setDescSet(0);

    descs[idx]->unRef();
    descs.remove(idx);
    ids.remove(idx);
}


void DescSet::removeAll()
{ while ( ids.size() ) removeDesc(ids[0]); }


void DescSet::fillPar( IOPar& par ) const
{
    int maxid = 0;

    for ( int idx=0; idx<descs.size(); idx++ )
    {
	const Desc& dsc = *descs[idx];
	IOPar apar;
	BufferString defstr;
	if ( !dsc.getDefStr(defstr) ) continue;
	apar.set( definitionStr(), defstr );

	BufferString userref( dsc.userRef() );
	apar.set( userRefStr(), userref );

	apar.setYN( hiddenStr(), dsc.isHidden() );

	for ( int input=0; input<dsc.nrInputs(); input++ )
	{
	    if ( !dsc.getInput(input) ) continue;

	    const char* key = IOPar::compKey( inputPrefixStr(), input );
	    apar.set( key, getID( *dsc.getInput(input) ).asInt() );
	}

	BufferString subkey = ids[idx].asInt();
	par.mergeComp( apar, subkey );

	if ( ids[idx]>maxid ) maxid = ids[idx].asInt();
    }

    par.set( highestIDStr(), maxid );
    if ( descs.size() > 0 )
	par.set( sKey::Type, is2D() ? "2D" : "3D" );
}


void DescSet::handleStorageOldFormat( IOPar& descpar )
{
    const char* typestr = descpar.find( "Type" );
    if ( !typestr || strcmp(typestr,"Stored") )
	return;

    const char* olddef = descpar.find( definitionStr() );
    if ( !olddef ) return;
    BufferString newdef = StorageProvider::attribName();
    newdef += " ";
    newdef += Attrib::StorageProvider::keyStr();
    newdef += "=";
    newdef += olddef;
    descpar.set( definitionStr(), newdef );
}


void DescSet::handleOldAttributes( BufferString& attribname, IOPar& descpar,
	                           BufferString& defstring )
{
    if ( attribname == "RefTime" )
    {
	attribname = "Reference";
	defstring = attribname;
	descpar.set( "Selected Attrib", "2" );
    }

    if ( attribname == "Hash" )
    {
	attribname = "Shift";
	const char* ptr = defstring.buf();
	ptr += 4;
	BufferString bstr = attribname;
	bstr += ptr;
	defstring = bstr;
    }
}


#define mHandleParseErr( str ) \
{ \
    errmsg = str; \
    if ( !errmsgs ) \
	return false; \
\
    (*errmsgs) += new BufferString(errmsg); \
    continue; \
}


#define mHandleDescErr( str ) \
{ \
    if ( !errmsgs ) \
	return 0; \
\
    (*errmsgs) += new BufferString(str); \
    return 0;\
}


Desc* DescSet::createDesc( const BufferString& attrname, const IOPar& descpar,
			   const BufferString& defstring,
			   BufferStringSet* errmsgs )
{
    Desc* dsc = PF().createDescCopy( attrname );
    if ( !dsc )
    {
	BufferString err = "Cannot find factory-entry for ";
	err += attrname;
	mHandleDescErr(err);
    }

    if ( !dsc->parseDefStr(defstring.buf()) )
    {
	if ( !dsc->isStored() )
	{
	    BufferString err = "Cannot parse: ";
	    err += defstring;
	    mHandleDescErr(err);
	}
    }

    BufferString userref;
    if ( !dsc->isStored() )
	userref = descpar.find( userRefStr() );
    else
    {
	const ValParam* keypar = dsc->getValParam( StorageProvider::keyStr() );
	const LineKey lk( keypar->getStringValue() );
	PtrMan<IOObj> ioobj = IOM().get( MultiID(lk.lineName().buf()) );
	userref = ioobj.ptr() ? (BufferString)ioobj->name()
	    		      : (BufferString)descpar.find( userRefStr() );
    }
    dsc->setUserRef( userref );

    bool ishidden = false;
    descpar.getYN( hiddenStr(), ishidden );
    dsc->setHidden( ishidden );

    int selout = dsc->selectedOutput();
    bool selectout = descpar.get("Selected Attrib",selout);
    if ( dsc->isStored() )
    {
	const char* type = descpar.find( "Datatype" ); 
	if ( type && !strcmp( type, "Dip" ) )
	    dsc->setNrOutputs( Seis::Dip, 2 );
	else
	    dsc->changeOutputDataType( selout, Seis::dataTypeOf( type ) );
    }

    if ( selectout )
	dsc->selectOutput(selout);

    return dsc;
}


Desc* DescSet::createDesc( const BufferString& attrname, const IOPar& descpar,
			   const BufferString& defstring )
{
    BufferStringSet* errmsgs = new BufferStringSet();
    Desc* newdesc = createDesc( attrname , descpar, defstring, errmsgs );
    errmsg = errmsgs && !errmsgs->isEmpty() ? errmsgs->get(0) : "";
    delete errmsgs;
    return newdesc;
}


void DescSet::handleReferenceInput( Desc* dsc )
{
    if ( dsc->isSatisfied() == Desc::Error )
    {
	Desc* inpdesc = getFirstStored( false );
	if ( !inpdesc ) return;

	dsc->setInput( 0, inpdesc );
    }
}


bool DescSet::setAllInputDescs( int nrdescsnosteer, const IOPar& copypar, 
				BufferStringSet* errmsgs )
{
    for ( int idx=0; idx<nrdescsnosteer; idx++ )
    {
	const BufferString idstr( ids[idx].asInt() );
	PtrMan<IOPar> descpar = copypar.subselect(idstr);
	if ( !descpar )
	    { pErrMsg("Huh?"); continue; }

	Desc& dsc = *descs[idx];
	for ( int input=0; input<dsc.nrInputs(); input++ )
	{
	    const char* key = IOPar::compKey( inputPrefixStr(), input );

	    int inpid;
	    if ( !descpar->get(key,inpid) ) continue;

	    Desc* inpdesc = getDesc( DescID(inpid,true) );
	    if ( !inpdesc ) continue;

	    dsc.setInput( input, inpdesc );
	}

	if ( !strcmp( dsc.attribName(), "Reference" ) )
	    handleReferenceInput( &dsc );
	
	if ( dsc.isSatisfied() == Desc::Error )
	{
	    BufferString err = dsc.errMsg(); err += " for ";
	    err += dsc.userRef(); err += " attribute ";
	    mHandleParseErr(err);
	}
    }

    return true;
}


//TODO use 2D/3D info
bool DescSet::usePar( const IOPar& par, BufferStringSet* errmsgs )
{
    removeAll();
    int maxid = 1024;
    par.get( highestIDStr(), maxid );
    IOPar copypar(par);
    bool res = true;

    for ( int id=0; id<=maxid; id++ )
    {
	const BufferString idstr( id );
	PtrMan<IOPar> descpar = par.subselect(idstr);
	if ( !descpar ) continue;

	handleStorageOldFormat( *descpar );

	BufferString defstring = descpar->find( definitionStr() );
	if ( defstring.isEmpty() )
	    mHandleParseErr( "No attribute definition string specified" );

	BufferString attribname;
	if ( !Desc::getAttribName( defstring.buf(), attribname ) )
	    mHandleParseErr( "Cannot find attribute name" );

	handleOldAttributes( attribname, *descpar, defstring );
	
	RefMan<Desc> dsc;
	dsc = errmsgs ? createDesc( attribname, *descpar, defstring, errmsgs )
	    	      : createDesc( attribname, *descpar, defstring );
	if ( !dsc )
	    { res = false; continue; }

	const char* emsg = Provider::prepare( *dsc );
	if ( emsg )
	 { 
	     if ( errmsgs )
		 errmsgs->add( emsg );
	     
	     res = false; 
	     continue;
	 }
	
	dsc->updateParams();
	addDesc( dsc, DescID(id,true) );
    }
    
    ObjectSet<Desc> newsteeringdescs;
    useOldSteeringPar(copypar, newsteeringdescs, errmsgs);

    for( int idx=0 ; idx<newsteeringdescs.size() ; idx++ )
	addDesc( newsteeringdescs[idx], DescID( maxid+idx+1, true ) );

    int nrdescsnosteer = ids.size()-newsteeringdescs.size();
    if ( !setAllInputDescs( nrdescsnosteer, copypar, errmsgs ) )
	res = false;

    return res;
}


bool DescSet::useOldSteeringPar( IOPar& par, ObjectSet<Desc>& newsteeringdescs,
				 BufferStringSet* errmsgs )
{
    int maxid = 1024;
    par.get( highestIDStr(), maxid );
    for ( int id=0; id<=maxid; id++ )
    {
	const BufferString idstr( id );
	PtrMan<IOPar> descpar = par.subselect(idstr);
	if ( !descpar ) continue;
					
	int steeringdescid = -1;
	const IOPar* steeringpar = descpar->subselect( "Steering" );
	if ( steeringpar )
	{
	    const char* defstring = descpar->find( definitionStr() );
	    if ( !defstring )
		mHandleParseErr( "No attribute definition string specified" );
	    if ( !createSteeringDesc(*steeringpar,defstring,newsteeringdescs,
				     steeringdescid) )
	        mHandleParseErr( "Cannot create steering desc" );
	    
	    Desc* dsc = getDesc( DescID(id,true) );
	    for ( int idx=0; idx<dsc->nrInputs(); idx++ )
	    {
		BufferString inputstr = IOPar::compKey( "Input", idx );
		if ( !strcmp(descpar->find(inputstr),"-1") )
		{
		    BufferString newinput = id;
		    newinput = IOPar::compKey( newinput, inputstr );
		    par.set( newinput, maxid + steeringdescid +1 );
		}
	    }
	}
    }
    return true;
}


#define mHandleSteeringParseErr( str ) \
{ \
    errmsg = str; \
    if ( !errmsgs ) \
	return false; \
\
    (*errmsgs) += new BufferString(errmsg); \
}


bool DescSet::createSteeringDesc( const IOPar& steeringpar, 
				  BufferString defstring,
				  ObjectSet<Desc>& newsteeringdescs, int& id,
				  BufferStringSet* errmsgs )
{
    BufferString steeringtype = steeringpar.find("Type");
    BufferString steeringdef = steeringtype;
    if ( steeringtype == "ConstantSteering" )
    {
	steeringdef += " ";
	steeringdef += "dip=";
	steeringdef += steeringpar.find("AppDip");
	steeringdef += " ";
	steeringdef += "azi=";
	steeringdef += steeringpar.find("Azimuth");
    }
    else
    {   
	steeringdef += " phlock=";
	bool phaselock = false;
	steeringpar.getYN( "PhaseLock", phaselock );
	steeringdef += phaselock ? "Yes" : "No";
	if ( phaselock )
	{
	    steeringdef += " aperture=";
	    const char* aperture = steeringpar.find("Aperture");
	    steeringdef += aperture ? aperture : "-5,5";
	}
    }

    BufferString attribname;
    if ( !Desc::getAttribName(steeringdef,attribname) )
	mHandleSteeringParseErr("Cannot find attribute name");

    RefMan<Desc> stdesc = PF().createDescCopy(attribname);
    if ( !stdesc )
    {
	BufferString err = "Cannot find factory-entry for ";
	err += attribname;
	mHandleSteeringParseErr(err);
    }

    if ( !stdesc->parseDefStr(steeringdef) )
    {
	BufferString err = "Cannot parse: ";
	err += steeringdef;
	mHandleSteeringParseErr(err);
    }

    BufferString usrrefstr = "steering input ";
    usrrefstr += newsteeringdescs.size();
    stdesc->setUserRef( usrrefstr );
    stdesc->setSteering(true);
    stdesc->setHidden(true);
    
    const char* inldipstr = steeringpar.find("InlDipID");
    if ( inldipstr )
    {
	DescID inldipid( atoi(inldipstr), true );
	stdesc->setInput( 0, getDesc(inldipid) );
    }

    const char* crldipstr = steeringpar.find("CrlDipID");
    if ( crldipstr )
    {
	DescID crldipid( atoi(crldipstr), true );
	stdesc->setInput( 1, getDesc(crldipid) );
    }	

//TODO see what's going on for the phase input	
    for ( int idx=0; idx<newsteeringdescs.size(); idx++ )
    {
	if ( stdesc->isIdenticalTo(*newsteeringdescs[idx]) )
	{
	    id = idx;
	    return true;
	}
    }
    stdesc->ref();
    newsteeringdescs += stdesc;
    id = newsteeringdescs.size()-1;
    
    return true;
}


const char* DescSet::errMsg() const
{ return errmsg[0] ? (const char*) errmsg : 0; }


DescID DescSet::getFreeID() const
{
    DescID id(0,true);
    while ( ids.indexOf(id)!=-1 )
	id.asInt()++;

    return id;
}


bool DescSet::is2D() const
{
    if ( is2dset_ ) return is2d_;

    bool hasstoreddesc = false;
    bool is2dsetinallstoreddescs = true;    
    for ( int idx=0; idx<descs.size(); idx++ )
    {
	const Desc& dsc = *descs[idx];
	if ( !dsc.isStored() )
	    continue;
	
	hasstoreddesc = true;
	if ( is2dsetinallstoreddescs ) 
	    is2dsetinallstoreddescs = dsc.is2DSet();
	
	if ( dsc.is2D() )
	{
	    const_cast<DescSet*>(this)->is2d_ = true;
	    const_cast<DescSet*>(this)->is2dset_ = true;
	    break;
	}
    }

    if ( is2dsetinallstoreddescs && hasstoreddesc )
	const_cast<DescSet*>(this)->is2dset_ = true;
    
    return is2dset_ ? is2d_ : false;
}


DescID DescSet::getStoredID( const char* lk, int selout, bool create )
{
    for ( int idx=0; idx<descs.size(); idx++ )
    {
	const Desc& dsc = *descs[idx];
	if ( !dsc.isStored() || dsc.selectedOutput()!=selout )
	    continue;

	const ValParam* keypar = dsc.getValParam( StorageProvider::keyStr() );
	const char* curlk = keypar->getStringValue();
	if ( !strcmp(lk,curlk) ) return dsc.id();
    }

    if ( !create ) return DescID::undef();

    LineKey newlk( lk );
    MultiID mid = newlk.lineName().buf();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) return DescID::undef();

    Desc* newdesc = PF().createDescCopy( StorageProvider::attribName() );
    if ( !newdesc ) return DescID::undef(); // "Cannot create desc"

    BufferString userref = LineKey( ioobj->name(), newlk.attrName() );
    newdesc->setUserRef( userref );
    newdesc->selectOutput( selout );
    ValParam* keypar = newdesc->getValParam( StorageProvider::keyStr() );
    keypar->setValue( lk );
    newdesc->updateParams();
    return addDesc( newdesc );
}


DescSet* DescSet::optimizeClone( const DescID& targetnode ) const
{
    TypeSet<DescID> needednodes( 1, targetnode );
    return optimizeClone( needednodes );
}


DescSet* DescSet::optimizeClone( const TypeSet<DescID>& targets ) const
{
    DescSet* res = new DescSet(is2d_,is2dset_);
    TypeSet<DescID> needednodes = targets;
    while ( needednodes.size() )
    {
	const DescID needednode = needednodes[0];
	needednodes.remove( 0 );
	const Desc* dsc = getDesc( needednode );
	if ( !dsc )
	{
	    delete res;
	    return 0;
	}

	Desc* nd = new Desc( *dsc );
	nd->setDescSet( res );
	res->addDesc( nd, needednode );

	for ( int idx=0; idx<dsc->nrInputs(); idx++ )
	{
	    const Desc* inpdesc = dsc->getInput(idx);
	    const DescID inputid = inpdesc ? inpdesc->id() : DescID::undef();
	    if ( inputid!=DescID::undef() && !res->getDesc(inputid) )
		needednodes += inputid;
	}
    }

    if ( res->nrDescs() == 0 )
	{ delete res; res = clone(); }

    res->updateInputs();
    return res;
}


DescSet* DescSet::optimizeClone( const BufferStringSet& targetsstr ) const
{
    TypeSet<DescID> needednodes;
    DescID id(-1,true);
    for ( int idx=0; idx<targetsstr.size(); idx++ )
    {
	id = getID( targetsstr.get( idx ), true);
	needednodes += id;
    }
    
    return optimizeClone( needednodes );
}


bool DescSet::isAttribUsed( const DescID& id ) const
{
    for ( int idx=0; idx<nrDescs(); idx++ )
    {
	const Desc& dsc = *descs[idx];
	for ( int inpnr=0; inpnr<dsc.nrInputs(); inpnr++ )
	{
	    if ( dsc.inputId(inpnr) == id )
		return true;
	}
    }

    return false;
}


int DescSet::removeUnused( bool remstored )
{
    TypeSet<DescID> torem;

    while ( true )
    {
	int count = 0;
	for ( int descidx=0; descidx<nrDescs(); descidx++ )
	{
	    DescID descid = getID( descidx );
	    if ( torem.indexOf(descid) >= 0 ) continue;

	    const Desc& dsc = *getDesc( descid );
	    bool iscandidate = false;
	    if ( dsc.isStored() )
	    {
		const ValParam* keypar = 
		    	dsc.getValParam( StorageProvider::keyStr() );
		PtrMan<IOObj> ioobj = IOM().get( keypar->getStringValue() );
		if ( remstored || !ioobj || !ioobj->implExists(true) )
		    iscandidate = true;
	    }
	    else if ( dsc.isHidden() )
		iscandidate = true;

	    if ( iscandidate )
	    {
		if ( !isAttribUsed(descid) )
		    { torem += descid; count++; }
	    }
	}

	if ( count == 0 ) break;
    }

    const int sz = torem.size();
    for ( int idx=sz-1; idx>=0; idx-- )
	removeDesc( torem[idx] );

    return sz;
}


Desc* DescSet::getFirstStored( bool usesteering ) const
{
    for ( int idx=0; idx<nrDescs(); idx++ )
    {
	const Desc& dsc = *descs[idx];
	if ( !dsc.isStored() ) continue;

	MultiID mid;
	if ( !dsc.getMultiID(mid) ) continue;

	PtrMan<IOObj> ioobj = IOM().get( mid );
	const char* res = ioobj ? ioobj->pars().find( "Type" ) : 0;
	const bool issteer = res && *res == 'S';
	if ( !usesteering && issteer ) continue;

	if ( (dsc.is2D() == is2D()) ) //TODO backward compatibility with 2.4
	    return const_cast<Desc*>( &dsc );
    }

    return 0;
}


void DescSet::fillInAttribColRefs( BufferStringSet& attrdefs ) const
{
    Attrib::SelInfo attrinf( this, 0, is2D() );
    for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
    {
	BufferString defstr;
	const Attrib::Desc* mydesc = getDesc( attrinf.attrids[idx] );
	if ( mydesc )
	    mydesc->getDefStr( defstr );
	FileMultiString fms( defstr ); fms += attrinf.attrids[idx].asInt();
	attrdefs.add( fms );
    }
    for ( int idx=0; idx<attrinf.ioobjids.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids.get(idx) ) );
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinf.ioobjnms.get(idx).buf();
	    FileMultiString fms(SeisIOObjInfo::defKey2DispName(defkey,ioobjnm));
	    fms += defkey;
	    attrdefs.add( fms );
	}
    }
}

}; // namespace Attrib
