/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "attribdescset.h"
#include "attribstorprovider.h"
#include "attribparam.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribsel.h"
#include "bufstringset.h"
#include "datacoldef.h"
#include "datapack.h"
#include "datapointset.h"
#include "gendefs.h"
#include "keystrs.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "mathexpression.h"
#include "separstr.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "odver.h"

namespace Attrib
{

DescSet::DescSet( bool is2d )
    : is2d_(is2d)
    , storedattronly_(false)
    , couldbeanydim_(false)
    , descToBeRemoved(this)
{
    ensureDefStoredPresent();
}


DescSet::DescSet( const DescSet& ds )
    : is2d_(ds.is2d_)
    , storedattronly_(ds.storedattronly_)
    , couldbeanydim_(ds.couldbeanydim_)
    , descToBeRemoved(this)
{
    *this = ds;
}


int DescSet::indexOf( const char* nm, bool isusrref ) const
{
    if ( !nm || !*nm ) return -1;

    for ( int idx=0; idx<size(); idx++ )
    {
	const Desc& dsc = *descs_[idx];
	if ( isusrref && dsc.isIdentifiedBy(nm) )
	    return idx;
	else if ( !isusrref )
	{
	    BufferString defstr; dsc.getDefStr( defstr );
	    if ( defstr == nm )
		return idx;
	}
    }
    return -1;
}


bool DescSet::hasStoredInMem() const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( descs_[idx]->isStoredInMem() )
	    return true;
    return false;
}


#define mGetPar(key) \
    defpars->find(SeisTrcTranslatorGroup::key())

DescID DescSet::ensureDefStoredPresent() const
{
    BufferString idstr; DescID retid;

    PtrMan<IOPar> defpars = SI().pars().subselect( sKey::Default() );
    if ( defpars )
    {
	if ( is2d_ )
	{
	    const FixedString lsid = mGetPar( sKeyDefault2D );
	    PtrMan<IOObj> lsobj = IOM().get( MultiID(lsid) );
	    BufferString attrnm = mGetPar( sKeyDefaultAttrib );
	    if ( lsobj && attrnm.isEmpty() )
	    {
		SeisIOObjInfo seisinfo( lsobj );
		BufferStringSet attrnms;
		SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = 0;
		seisinfo.getAttribNames( attrnms, o2d );
		if ( !attrnms.isEmpty() )
		    attrnm = attrnms.get(0);
	    }

	    idstr = LineKey( lsid, attrnm );
	}
	else
	    idstr = mGetPar( sKeyDefault3D );
    }

    if ( defidstr_ == idstr && defattribid_ != DescID::undef() )
	return defattribid_;

    if ( !idstr.isEmpty() )
    {
	// Hack to get rid of 'old' IDs
	bool allstored = true;
	for ( int idx=0; idx<descs_.size(); idx++ )
	{
	    if ( !descs_[idx]->isStored() )
		{ allstored = false; break; }
	}
	if ( allstored )
	    const_cast<DescSet*>(this)->removeAll( false );

	retid = const_cast<DescSet*>(this)->getStoredID( idstr.buf(), 0, true,
	       						 true );
    }

    defidstr_ = idstr;
    defattribid_ = retid;
    return retid;
}


DescSet& DescSet::operator =( const DescSet& ds )
{
    if ( &ds != this )
    {
	removeAll( false );
	is2d_ = ds.is2d_;
	storedattronly_ = ds.storedattronly_;
	couldbeanydim_ = ds.couldbeanydim_;
	for ( int idx=0; idx<ds.size(); idx++ )
	    addDesc( new Desc( *ds.descs_[idx] ), ds.ids_[idx] );
	updateInputs();
    }
    return *this;
}


void DescSet::updateInputs()
{
    for ( int idx=0; idx<size(); idx++ )
    {
	Desc& dsc = *descs_[idx];
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
    nd->setDescSet( this ); nd->ref();
    descs_ += nd;
    const DescID newid = id.isValid() ? id : getFreeID();
    ids_ += newid;
    return newid;
}


DescID DescSet::insertDesc( Desc* nd, int idx, DescID id )
{
    nd->setDescSet( this ); nd->ref();
    descs_.insertAt( nd, idx );
    const DescID newid = id.isValid() ? id : getFreeID();
    ids_.insert( idx, newid );
    return newid;
}


Desc* DescSet::gtDesc( const DescID& id ) const
{
    const int idx = ids_.indexOf(id);
    if ( idx < 0 ) return 0;
    return const_cast<Desc*>( descs_[idx] );
}


int DescSet::nrDescs( bool incstored, bool inchidden ) const
{
    int ret = descs_.size();
    if ( !incstored || !inchidden )
    {
	for ( int idx=0; idx<descs_.size(); idx++ )
	{
	    const Desc& dsc = *descs_[idx];
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
    const int idx = descs_.indexOf( &dsc );
    return idx==-1 ? DescID::undef() : ids_[idx];
}


DescID DescSet::getID( int idx ) const
{
    if ( idx < 0 || idx >= ids_.size() ) return DescID::undef();
    return ids_[idx];
}


void DescSet::getIds( TypeSet<DescID>& attribids ) const
{ attribids = ids_; }


void DescSet::getStoredIds( TypeSet<DescID>& attribids ) const
{
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	if ( descs_[idx]->isStored() )
	    attribids += ids_[idx];
    }
}


DescID DescSet::getID( const char* str, bool isusrref, bool isdescstored,
       		       bool usestorinfo	) const
{
    if ( !str || !*str ) return DescID::undef();

    for ( int idx=0; idx<size(); idx++ )
    {
	const Desc& dsc = *descs_[idx];
	if ( isusrref && dsc.isIdentifiedBy(str) )
	{
	    if ( !usestorinfo )
		return ids_[idx];

	    bool isstored = dsc.isStored();
	    if ( (isdescstored && isstored) || (!isdescstored && !isstored) )
		return ids_[idx];
	    }
	else if ( !isusrref )
	{
	    BufferString defstr;
	    dsc.getDefStr( defstr );
	    if ( defstr == str )
		return ids_[idx];
	}
    }

    return DescID::undef();
}


void DescSet::removeDesc( const DescID& id )
{
    const int idx = ids_.indexOf(id);
    if ( idx==-1 ) return;

    descToBeRemoved.trigger( id );
    if ( descs_[idx]->descSet()==this )
	descs_[idx]->setDescSet(0);

    descs_.removeSingle(idx)->unRef();
    ids_.removeSingle(idx);
}


void DescSet::moveDescUpDown( const DescID& id, bool moveup )
{
    const int sz = ids_.size();
    const int selidx = ids_.indexOf( id );
    int gotoidx = moveup ? selidx-1 : selidx+1;
    while ( gotoidx>=0 && gotoidx<sz
	    && ( descs_[gotoidx]->isHidden() || descs_[gotoidx]->isStored() ) )
	gotoidx += moveup ? -1 : 1;

    if ( selidx==-1 || gotoidx==-1 ) return;
    ids_.swap( selidx, gotoidx );
    descs_.swap( selidx, gotoidx );
}


void DescSet::sortDescSet()
{
    const int nrdescs = descs_.size();
    BufferStringSet userrefs;
    for ( int idx=0; idx<nrdescs; idx++ )
    	userrefs.add( descs_[idx]->userRef() );
 
    int* sortindexes = userrefs.getSortIndexes();
    ObjectSet<Desc> descscopy( descs_ );
    TypeSet<DescID> idscopy( ids_ );
    descs_.erase();
    ids_.erase();
    for ( int idx=0; idx<nrdescs; idx++ )
    {
	Attrib::Desc* newdesc = descscopy[ sortindexes[idx] ];
	descs_ += newdesc;
	ids_ += idscopy[ sortindexes[idx] ];
    }

    descscopy.erase();
    delete sortindexes;
}


void DescSet::removeAll( bool kpdef )
{
    while ( ids_.size() )
	removeDesc( ids_[0] ); 
    if ( kpdef )
	ensureDefStoredPresent();
}


//As we do not store DescSets with storedattronly_=true it is useless to check
//for this in usePar and fillPar 
void DescSet::fillPar( IOPar& par ) const
{
    int maxid = 0;

    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	const Desc& dsc = *descs_[idx];
	IOPar apar;
	BufferString defstr;
	if ( !dsc.getDefStr(defstr) ) continue;
	apar.set( definitionStr(), defstr );

	BufferString userref( dsc.userRef() );
	apar.set( userRefStr(), userref );

	apar.setYN( hiddenStr(), dsc.isHidden() );
	apar.set( sKey::DataType(), Seis::nameOf(dsc.dataType()) );
	apar.set( indexStr(), idx );

	for ( int input=0; input<dsc.nrInputs(); input++ )
	{
	    if ( !dsc.getInput(input) ) continue;

	    const char* key = IOPar::compKey( inputPrefixStr(), input );
	    apar.set( key, getID( *dsc.getInput(input) ).asInt() );
	}

	par.mergeComp( apar, BufferString("",ids_[idx].asInt()) );

	if ( ids_[idx].asInt() > maxid ) maxid = ids_[idx].asInt();
    }

    par.set( highestIDStr(), maxid );
    if ( descs_.size() > 0 )
	par.set( sKey::Type(), couldbeanydim_ ? "AnyD" : is2D() ? "2D" : "3D" );
}


void DescSet::handleStorageOldFormat( IOPar& descpar )
{
    FixedString typestr = descpar.find( "Type" );
    if ( typestr.isEmpty() || typestr!="Stored" )
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
	                           BufferString& defstring,
				   float versionnr ) const
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
    if ( attribname == "Math" && versionnr<3.4 )
	handleOldMathExpression( descpar, defstring );
}


void DescSet::handleOldMathExpression( IOPar& descpar,
				       BufferString& defstring ) const
{
    RefMan<Desc> tmpdesc = PF().createDescCopy("Math");
    if ( !tmpdesc || !tmpdesc->parseDefStr(defstring.buf()) ) return;
    ValParam* expr = tmpdesc->getValParam( "expression" );
    if ( !expr ) return;
    MathExpressionParser mep( expr->getStringValue() );
    PtrMan<MathExpression> formula = mep.parse();
    if ( !formula ) return;

    TypeSet<int> oldinputs;
    TypeSet<int> correctinputs;
    int inputidx = 0;
    while( true )
    {
	int inpid;
	const char* key = IOPar::compKey( inputPrefixStr(), inputidx );
	if ( !descpar.get(key,inpid) ) break;
	oldinputs += inpid;
	inputidx++;
    }

    for ( int idx=0; idx<formula->nrUniqueVarNames(); idx++ )
    {
	if ( MathExpressionParser::varTypeOf( formula->uniqueVarName(idx) ) !=
		MathExpression::Variable ) continue;

	const BufferString varnm( formula->uniqueVarName(idx) );
	const char* ptr = varnm.buf();
	while ( *ptr && !isdigit(*ptr) )
	    ptr++;

	int varxidx = toInt( ptr );
	if ( varxidx >= oldinputs.size() )
	{
	    const_cast<DescSet*>(this)->errmsg_ +=
					"Cannot use old Math expression";
	    return;
	}
	correctinputs += oldinputs[varxidx];
    }

    for ( int idx=0; idx<correctinputs.size(); idx++ )
    {
	const char* key = IOPar::compKey( inputPrefixStr(), idx );
	descpar.set( key, correctinputs[idx] );
    }
}


#define mHandleParseErr( str ) \
{ \
    errmsg_ = str; \
    if ( !errmsgs ) \
	return false; \
\
    (*errmsgs) += new BufferString(errmsg_); \
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

    BufferString userref = descpar.find( userRefStr() ).str();
    if ( dsc->isStored() )
    {
	const ValParam* keypar = dsc->getValParam( StorageProvider::keyStr() );
	const LineKey lk( keypar->getStringValue() );
	PtrMan<IOObj> ioobj = IOM().get( MultiID(lk.lineName().buf()) );
	if ( ioobj.ptr() )
	{
	    BufferString tentativeuserref = (BufferString)ioobj->name();
	    if ( !tentativeuserref.isStartOf( userref ) )
		userref = tentativeuserref;
	}
    }
    dsc->setUserRef( userref );

    bool ishidden = false;
    descpar.getYN( hiddenStr(), ishidden );
    dsc->setHidden( ishidden );

    int selout = dsc->selectedOutput();
    bool selectout = descpar.get("Selected Attrib",selout);
    if ( dsc->isStored() )
    {
	FixedString type = descpar.find( sKey::DataType() );
	if ( type=="Dip" )
	    dsc->setNrOutputs( Seis::Dip, 2 );
	else
	    dsc->changeOutputDataType( selout, Seis::dataTypeOf(type) );
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
    errmsg_ = errmsgs && !errmsgs->isEmpty() ? errmsgs->get(0) : "";
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
	PtrMan<IOPar> descpar =
	    copypar.subselect( toString(ids_[idx].asInt()) );
	if ( !descpar )
	    { pErrMsg("Huh?"); continue; }

	Desc& dsc = *descs_[idx];
	for ( int input=0; input<dsc.nrInputs(); input++ )
	{
	    const char* key = IOPar::compKey( inputPrefixStr(), input );

	    int inpid;
	    if ( !descpar->get(key,inpid) ) continue;

	    Desc* inpdesc = getDesc( DescID(inpid,false) );
	    if ( !inpdesc ) continue;

	    dsc.setInput( input, inpdesc );
	}

	if ( dsc.attribName()=="Reference" )
	    handleReferenceInput( &dsc );
	
	if ( dsc.isSatisfied() == Desc::Error )
	{
	    BufferString err;
	    FixedString dscerr = dsc.errMsg();
	    if ( dscerr=="Parameter 'id' is not correct" &&
		 dsc.isStored() )                                              
	    {                                                                   
		err = "Impossible to find stored data '";             
		err += dsc.userRef();                                          
		err += "' \nused as input for other attribute(s). \n";
		err += "Data might have been deleted or corrupted.\n";
		err += "Please check your attribute set \n";
		err += "and select valid stored data as input.";                      
	    }                                                                   
	    else                                                                
	    {
		err = dsc.errMsg(); err += " for ";
		err += dsc.userRef(); err += " attribute ";
	    }

	    mHandleParseErr(err);
	}
    }

    return true;
}


bool DescSet::usePar( const IOPar& par, float versionnr,
		      BufferStringSet* errmsgs )
{
    if ( mIsUdf(versionnr) )
	versionnr = mODVersion * 0.01;

    const char* typestr = par.find( sKey::Type() );
    if ( typestr )
    {
	is2d_ = *typestr == '2';
	couldbeanydim_ = *typestr == 'A';
    }

    removeAll( false );

    int maxid = 1024;
    par.get( highestIDStr(), maxid );
    IOPar copypar(par);
    bool res = true;

    TypeSet<int> indexes;
    for ( int id=0; id<=maxid; id++ )
    {
	PtrMan<IOPar> descpar = par.subselect( toString(id) );
	if ( !descpar ) continue;

	handleStorageOldFormat( *descpar );

	BufferString defstring = descpar->find( definitionStr() ).str();
	if ( defstring.isEmpty() )
	    mHandleParseErr( "No attribute definition string specified" );

	BufferString attribname;
	if ( !Desc::getAttribName( defstring.buf(), attribname ) )
	    mHandleParseErr( "Cannot find attribute name" );

	handleOldAttributes( attribname, *descpar, defstring, versionnr );
	if ( errmsgs && !errmsg_.isEmpty() )
	    errmsgs->add( errmsg_ );
	
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

	int idx=-1;
	descpar->get( indexStr(), idx );
	indexes += idx;

	dsc->updateParams();
	addDesc( dsc, DescID(id,storedattronly_) );
	copypar.mergeComp( *descpar, toString(id) );
    }
 
    // sort_coupled();
    ObjectSet<Desc> newsteeringdescs;
    useOldSteeringPar(copypar, newsteeringdescs, errmsgs);

    for( int idx=0 ; idx<newsteeringdescs.size() ; idx++ )
	addDesc( newsteeringdescs[idx], DescID( maxid+idx+1, false ) );

    int nrdescsnosteer = ids_.size()-newsteeringdescs.size();
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
	PtrMan<IOPar> descpar = par.subselect( BufferString("",id) );
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
	    
	    Desc* dsc = getDesc( DescID(id,false) );
	    for ( int idx=0; idx<dsc->nrInputs(); idx++ )
	    {
		BufferString inputstr = IOPar::compKey( "Input", idx );
		if ( descpar->find(inputstr)=="-1" )
		{
		    const char* newkey =
			IOPar::compKey(toString(id),inputstr);
		    par.set( newkey, maxid + steeringdescid +1 );
		}
	    }
	}
    }
    return true;
}


#define mHandleSteeringParseErr( str ) \
{ \
    errmsg_ = str; \
    if ( !errmsgs ) \
	return false; \
\
    (*errmsgs) += new BufferString(errmsg_); \
}


bool DescSet::createSteeringDesc( const IOPar& steeringpar, 
				  BufferString defstring,
				  ObjectSet<Desc>& newsteeringdescs, int& id,
				  BufferStringSet* errmsgs )
{
    FixedString steeringtype = steeringpar.find( sKey::Type() );
    BufferString steeringdef = steeringtype.str();
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
	DescID inldipid( toInt(inldipstr), false );
	stdesc->setInput( 0, getDesc(inldipid) );
    }

    const char* crldipstr = steeringpar.find("CrlDipID");
    if ( crldipstr )
    {
	DescID crldipid( toInt(crldipstr), false );
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
{
    return errmsg_.str();
}


DescID DescSet::getFreeID() const
{
    int highestid = -1;
    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	const int index = ids_[idx].asInt();
	if ( index > highestid )
	    highestid = index;
    }

    return DescID( highestid+1, storedattronly_ );
}


DescID DescSet::getStoredID( const char* lkstr, int selout, bool create,
			     bool blindcomp, const char* blindcompnm )
{
    FixedString lk( lkstr );
    TypeSet<int> outsreadyforthislk;
    TypeSet<DescID> outsreadyids;
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	const Desc& dsc = *descs_[idx];
	const bool outnrisok = dsc.selectedOutput() == selout;
	if ( !dsc.isStored() || ( !outnrisok && selout>=0 ) )
	    continue;

	const ValParam& keypar = *dsc.getValParam( StorageProvider::keyStr() );
	if ( lk==keypar.getStringValue() )
	{
	    if ( selout>=0 ) return dsc.id();
	    outsreadyforthislk += dsc.selectedOutput();
	    outsreadyids += dsc.id();
	}
    }

    if ( !create )
	return DescID::undef();

    if ( blindcomp )
	return createStoredDesc( lk, selout, BufferString(
					    blindcompnm ? blindcompnm :"") );

    const int out0idx = outsreadyforthislk.indexOf( 0 );
    BufferStringSet bss; SeisIOObjInfo::getCompNames( lk.str(), bss );
    const int nrcomps = bss.size();
    if ( nrcomps < 2 )
	return out0idx != -1 ? outsreadyids[out0idx] 
			     : createStoredDesc( lk, 0, BufferString("") );

    const int startidx = selout<0 ? 0 : selout; 
    const int stopidx = selout<0 ? nrcomps : selout;
    const BufferString& curstr = bss.validIdx(startidx)
				? bss.get(startidx) : BufferString::empty();
    const DescID retid = out0idx != -1
			? outsreadyids[out0idx] 
			: createStoredDesc( lk, startidx, curstr );
    for ( int idx=startidx+1; idx<stopidx; idx++ )
	if ( !outsreadyforthislk.isPresent(idx) )
	    createStoredDesc( lk, idx, *bss[idx] );

    return retid;
}


DescID DescSet::createStoredDesc( const char* lk, int selout,
				  const BufferString& compnm )
{
    LineKey newlk( lk );
    BufferString bstring = newlk.lineName();
    const char* linenm = bstring.buf();
    BufferString objnm;
    if ( linenm && *linenm == '#' )
    {
	DataPack::FullID fid( linenm+1 );
	if ( !DPM(fid).haveID( fid ) )
	    return DescID::undef();

	objnm = DataPackMgr::nameOf( fid );
    }
    else
    {
	MultiID mid = linenm;
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj ) return DescID::undef();

	objnm = ioobj->name();
    }

    Desc* newdesc = PF().createDescCopy( StorageProvider::attribName() );
    if ( !newdesc ) return DescID::undef(); // "Cannot create desc"
    if ( compnm.isEmpty() && selout>0 )
	return DescID::undef(); 	// "Missing component name"

    BufferString userref = LineKey( objnm, is2d_ ? newlk.attrName() : "" );
    if ( !compnm.isEmpty() )
    {
	if ( is2d_ ) userref = newlk.attrName();
	userref += "|";
	userref += compnm.buf();
    }
    newdesc->setUserRef( userref );
    newdesc->selectOutput( selout );
    ValParam& keypar = *newdesc->getValParam( StorageProvider::keyStr() );
    keypar.setValue( lk );
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
    DescSet* res = new DescSet(is2d_);
    res->removeAll( false );
    TypeSet<DescID> needednodes = targets;
    while ( needednodes.size() )
    {
	const DescID needednode = needednodes[0];
	needednodes.removeSingle( 0 );
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

    if ( res->isEmpty() )
	{ delete res; res = new DescSet(*this); }

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
    for ( int idx=0; idx<size(); idx++ )
    {
	const Desc& dsc = *descs_[idx];
	for ( int inpnr=0; inpnr<dsc.nrInputs(); inpnr++ )
	{
	    if ( dsc.inputId(inpnr) == id )
		return true;
	}
    }

    return false;
}


int DescSet::removeUnused( bool remstored, bool kpdefault )
{
    TypeSet<DescID> torem;

    while ( true )
    {
	int count = 0;
	for ( int descidx=0; descidx<size(); descidx++ )
	{
	    if ( kpdefault && !descidx ) continue; //default desc always first

	    DescID descid = getID( descidx );
	    if ( torem.isPresent(descid) ) continue;

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
    for ( int idx=0; idx<size(); idx++ )
    {
	const Desc& dsc = *descs_[idx];
	if ( !dsc.isStored() ) continue;

	BufferString storedid = dsc.getStoredID();
	if ( storedid.isEmpty() ) continue;

	PtrMan<IOObj> ioobj = IOM().get( MultiID(storedid.buf()) );
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
    Attrib::SelInfo attrinf( this, 0, is2D(), DescID::undef(), true );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
    {
	BufferString defstr;
	const Attrib::Desc* mydesc = getDesc( attrinf.attrids_[idx] );
	if ( mydesc )
	    mydesc->getDefStr( defstr );
	FileMultiString fms( defstr ); fms += attrinf.attrids_[idx].asInt();
	attrdefs.add( fms );
    }
    for ( int idx=0; idx<attrinf.ioobjids_.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids_.get(idx) ) );
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinf.ioobjnms_.get(idx).buf();
	    FileMultiString fms(is2D()
		    ? SeisIOObjInfo::defKey2DispName(defkey,ioobjnm)
		    : SeisIOObjInfo::def3DDispName(defkey,ioobjnm) );
	    fms += defkey;
	    attrdefs.add( fms );
	}
    }
}


void DescSet::createAndAddMultOutDescs( const DescID& targetid,
	                                const TypeSet<int>& seloutputs,
					const BufferStringSet& seloutnms,
					TypeSet<DescID>& outdescids )
{
    const int nrseloutputs = seloutputs.size() ? seloutputs.size() : 1;
    Desc* basedesc = getDesc( targetid );
    if ( !basedesc ) return;

    for ( int idx=0; idx<nrseloutputs; idx++ )
    {
	if ( seloutputs[idx] == basedesc->selectedOutput() )
	{
	    basedesc->setUserRef( seloutnms[idx]->buf() );
	    outdescids += targetid;
	    continue;
	}

	Desc* newdesc = new Desc( *basedesc );
	newdesc->selectOutput( seloutputs[idx] );
	newdesc->setUserRef( seloutnms[idx]->buf() );
	outdescids += addDesc( newdesc );
    }
}


void DescSet::setContainStoredDescOnly( bool yn )
{
    storedattronly_ = yn;
    for ( int idx=0; idx<ids_.size(); idx++ )
	ids_[idx].setStored( yn );

    if ( defattribid_.isValid() )
	defattribid_.setStored( yn );
}


DataPointSet* DescSet::createDataPointSet( Attrib::DescSetup dsu,
					   bool withstored ) const
{
    TypeSet<DataPointSet::DataRow> pts;
    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	const Attrib::Desc* tmpdsc = desc(idx);
	if ( !tmpdsc || (tmpdsc->isHidden() && !dsu.hidden_) ||
	     (tmpdsc->isStored() && !withstored) )
	    continue;

	BufferString defstr;
	tmpdsc->getDefStr( defstr );
	dcds += new DataColDef( tmpdsc->userRef(), defstr.buf() );
    }

    return new DataPointSet( pts, dcds, is2D() );
}


void DescSet::fillInSelSpecs( Attrib::DescSetup dsu,
			      TypeSet<Attrib::SelSpec>& specs ) const
{
    //TODO check all dsu cases
    for ( int idx=0; idx<descs_.size(); idx++ )                                 
    {                                                                           
	const Attrib::Desc* tmpdsc = desc(idx);                                 
	if ( !tmpdsc || (tmpdsc->isHidden() && !dsu.hidden_) )
	    continue;

	Attrib::SelSpec sp( 0, tmpdsc->id() );
	specs += sp;
    }
}


void DescSet::cleanUpDescsMissingInputs()
{
    bool cleaning = true;

    while ( cleaning )
    {
	cleaning = false;
	for ( int idx=size()-1; idx>0; idx-- )
	{
	    Desc& dsc = *descs_[idx];
	    for ( int inpidx=0; inpidx<dsc.nrInputs(); inpidx++ )
	    {
		DescID checkid = dsc.inputId(inpidx);
		if ( ( !checkid.isValid() || !getDesc(checkid) )
			&& dsc.inputSpec(inpidx).enabled_ )
		{
		    cleaning = true;
		    removeDesc( dsc.id() );
		    break;
		}
	    }
	}
    }
}


}; // namespace Attrib
