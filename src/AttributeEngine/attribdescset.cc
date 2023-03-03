/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribdescset.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "bufstringset.h"
#include "compoundkey.h"
#include "datacoldef.h"
#include "datapack.h"
#include "datapointset.h"
#include "gendefs.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "linekey.h"
#include "mathexpression.h"
#include "odver.h"
#include "od_ostream.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "separstr.h"
#include "survinfo.h"
#include "uistrings.h"

namespace Attrib
{

uiString DescSet::sFactoryEntryNotFound(const char* attrnm)
{
    return uiStrings::phrCannotCreate( tr("an instance of attribute %1")
					.arg(attrnm) );
}


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
	    idstr = mGetPar( sKeyDefault2D );
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
    const int idx = ids_.indexOf( id );
    if ( !descs_.validIdx(idx) )
	return 0;

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
    while ( gotoidx>=0 && gotoidx<sz && descs_[gotoidx]
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

    ConstArrPtrMan<int> sortindexes = userrefs.getSortIndexes();
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
	if ( !dsc.getDefStr(defstr) )
	    continue;

	if ( dsc.isStored() || dsc.nrInputs()>0 )
	{
	    const MultiID storeid( dsc.getStoredID(true).buf() );
	    if ( storeid.isDatabaseID() )
	    {
		PtrMan<IOObj> ioobj = IOM().get( storeid );
		if ( !ioobj )
		    continue;
	    }
	}

	apar.set( definitionStr(), defstr );

	BufferString userref( dsc.userRef() );
	apar.set( userRefStr(), userref );

	apar.setYN( hiddenStr(), dsc.isHidden() );
	apar.set( sKey::DataType(), Seis::nameOf(dsc.dataType()) );
	apar.set( indexStr(), idx );

	for ( int input=0; input<dsc.nrInputs(); input++ )
	{
	    if ( !dsc.getInput(input) )
		continue;

	    const char* key = IOPar::compKey( inputPrefixStr(), input );
	    apar.set( key, getID( *dsc.getInput(input) ).asInt() );
	}

	par.mergeComp( apar, BufferString("",ids_[idx].asInt()) );

	if ( ids_[idx].asInt() > maxid )
	    maxid = ids_[idx].asInt();
    }

    par.set( highestIDStr(), maxid );
    if ( descs_.size() > 0 )
	par.set( sKey::Type(), couldbeanydim_ ? "AnyD" : is2D() ? "2D" : "3D" );
}


void DescSet::handleStorageOldFormat( IOPar& descpar )
{
    const BufferString typestr = descpar.find( "Type" );
    if ( typestr.isEmpty() || !typestr.isEqual("Stored") )
	return;

    BufferString olddef = descpar.find( definitionStr() );
    if ( olddef.isEmpty() )
	return;

    BufferString newdef = StorageProvider::attribName();
    newdef += " ";
    newdef += Attrib::StorageProvider::keyStr();
    newdef += "=";
    newdef += olddef;
    descpar.set( definitionStr(), newdef );
}


void DescSet::handleOldAttributes( BufferString& attribname, IOPar& descpar,
				   BufferString& defstring,
				   int odversion ) const
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
    if ( attribname == "Math" && odversion<500 )
	handleOldMathExpression( descpar, defstring, odversion );
    if ( attribname == "Frequency" )
    {
	BufferStringSet keys;
	BufferStringSet vals;
	Attrib::Desc::getKeysValsPublic( defstring.buf(), keys, vals );
	if ( !keys.isPresent( "smoothspect" ) )
	    defstring.add( " smoothspect=No" );
    }
}


void DescSet::handleOldMathExpression( IOPar& descpar,
				       BufferString& defstring,
				       int odversion ) const
{
    RefMan<Desc> tmpdesc = PF().createDescCopy("Math");
    if ( !tmpdesc || !tmpdesc->parseDefStr(defstring.buf()) ) return;
    ValParam* expr = tmpdesc->getValParam( "expression" );
    if ( !expr ) return;
    Math::ExpressionParser mep( expr->getStringValue() );
    PtrMan<Math::Expression> formula = mep.parse();
    if ( !formula ) return;

    if ( odversion<340 )
    {
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
	    if ( Math::ExpressionParser::varTypeOf( formula->uniqueVarName(idx))
		    != Math::Expression::Variable ) continue;

	    const BufferString varnm( formula->uniqueVarName(idx) );
	    const char* ptr = varnm.buf();
	    while ( *ptr && !iswdigit(*ptr) )
		ptr++;

	    int varxidx = toInt( ptr );
	    if ( varxidx >= oldinputs.size() )
	    {
		const_cast<DescSet*>(this)->errmsg_ =
					tr("Cannot use old Math expression");
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

    for ( int idx=0; idx<formula->nrUniqueVarNames(); idx++ )
    {
	BufferString varnm ( formula->uniqueVarName(idx) );

	if ( varnm == BufferString("DZ") || varnm == BufferString("Inl" )
	  || varnm == BufferString("Crl") || varnm == BufferString("XCoord" )
	  || varnm == BufferString("YCoord") || varnm == BufferString("Z" ) )
	{
	    BufferString alternativenm ( varnm, "Input" );
	    defstring.replace( varnm.buf(), alternativenm.buf() );
	}
    }


}


#define mHandleParseErr( str ) \
{ \
    errmsg_ = str; \
    if ( !errmsgs ) \
	return false; \
\
    (*errmsgs) += errmsg_; \
    continue; \
}


#define mHandleDescErr( str ) \
{ \
    if ( !errmsgs ) \
	return 0; \
\
    (*errmsgs) += str; \
    return 0;\
}


Desc* DescSet::createDesc( const BufferString& attrname, const IOPar& descpar,
			   const BufferString& defstring,
			   uiStringSet* errmsgs )
{
    Desc* dsc = PF().createDescCopy( attrname );
    if ( !dsc )
	mHandleDescErr( sFactoryEntryNotFound(attrname) );

    if ( !dsc->parseDefStr(defstring.buf()) )
    {
	if ( !dsc->isStored() )
	{
	    uiString err = tr("Cannot parse: %1").arg( defstring );
	    mHandleDescErr(err);
	}
    }

    int selout = dsc->selectedOutput();
    BufferString userref = descpar.find( userRefStr() );
    if ( dsc->isStored() )
    {
	const MultiID key( dsc->getStoredID().buf() );
	PtrMan<IOObj> ioobj = IOM().get( key );
	if ( ioobj )
	{
	    const BufferString tentativeuserref = ioobj->name();
	    if ( !tentativeuserref.isStartOf(userref) )
	    {
		BufferStringSet compnms;
		SeisIOObjInfo::getCompNames( key, compnms );
		if ( compnms.size()>1 )
		{
		    BufferString compstr( Desc::sKeyAll() );
		    if ( compnms.validIdx(selout) )
			compstr = compnms.get( selout );

		    userref = StringPair(ioobj->name(),compstr).second();
		}
		else
		    userref = tentativeuserref;
	    }
	}
    }
    dsc->setUserRef( userref );

    bool ishidden = false;
    descpar.getYN( hiddenStr(), ishidden );
    dsc->setHidden( ishidden );

    const bool selectout = descpar.get("Selected Attrib",selout);
    if ( dsc->isStored() )
    {
	const BufferString type = descpar.find( sKey::DataType() );
	if ( type.isEqual("Dip") )
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
    errmsg_.setEmpty();
    PtrMan<uiStringSet > errmsgs = new uiStringSet;
    Desc* newdesc = createDesc( attrname , descpar, defstring, errmsgs );
    if ( errmsgs && !errmsgs->isEmpty() )
	errmsg_ = (*errmsgs)[0];

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
				uiStringSet* errmsgs )
{
    TypeSet<int> toberemoved;
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
	    uiString err;
	    StringView dscerr = dsc.errMsg();
	    const bool storagenotfound = dscerr==DescSet::storedIDErrStr();
	    if ( storagenotfound && dsc.isStoredInMem() )
		continue;

	    if ( storagenotfound && dsc.isStored() )
	    {
		IOPar tmpcpypar( copypar );
		BufferString depattribnm;
		bool found = true;
		while ( found )
		{
		    const BufferString compstr(
			tmpcpypar.findKeyFor(toString(ids_[idx].asInt())) );
		    CompoundKey compkey( compstr.buf() );
		    found = !compkey.isEmpty();
		    if ( found )
		    {
			if ( compkey.nrKeys()>1
			  && compkey.key(1)==sKey::Input() )
			{
			    CompoundKey usrrefkey(compkey.key(0));
			    usrrefkey += userRefStr();
			    copypar.get( usrrefkey.buf(), depattribnm );
			    if ( depattribnm.contains("FullSteering")
				 || depattribnm.contains("CentralSteering") )
			    {
				depattribnm.setEmpty();
				bool foundsecondorder = true;
				while ( foundsecondorder )
				{
				    const BufferString compkey2str(
					tmpcpypar.findKeyFor(
					    toString(compkey.key(0))) );
				    const CompoundKey compkey2(
							compkey2str.buf() );
				    foundsecondorder = !compkey2.isEmpty();
				    if ( foundsecondorder )
				    {
					if ( compkey2.nrKeys()>1
					  && compkey2.key(1) == sKey::Input() )
					{
					    CompoundKey urkey(compkey2.key(0));
					    urkey += userRefStr();
					    copypar.get( urkey.buf(),
							 depattribnm );
					    break;
					}
					else
					    tmpcpypar.removeWithKey(
							    compkey2.buf() );
				    }
				}
			    }
			    break;
			}
			else
			    tmpcpypar.removeWithKey( compkey.buf() );
		    }
		}
		err = tr( "Impossible to find stored data '%1' "
			"used as input for another attribute %2 '%3'. \n"
			"Data might have been deleted or corrupted.\n"
			"Please check your attribute set "
			"and select valid stored data as input." )
			.arg( dsc.userRef() )
			.arg( depattribnm.isEmpty() ? uiString::emptyString()
						    : tr("called"))
			.arg( depattribnm.isEmpty() ? uiString::emptyString()
					    : mToUiStringTodo(depattribnm));
	    }
	    else
	    {
		err = tr( "Attribute '%1'\n\n%2")
			.arg( dsc.userRef() ).arg( dsc.errMsg() );
	    }

	    toberemoved += idx;
	    mHandleParseErr(err);
	}
    }

    for ( int idx=toberemoved.size()-1; idx>=0; idx-- )
	removeDesc( descs_[toberemoved[idx]]->id() );

    return true;
}


bool DescSet::usePar( const IOPar& par, uiStringSet* errmsgs )
{
    const BufferString typestr = par.find( sKey::Type() );
    if ( !typestr.isEmpty() )
    {
	const BufferString firstchar( typestr.firstChar() );
	is2d_ = firstchar.isEqual("2");
	couldbeanydim_ = firstchar.isEqual("A");
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

	BufferString defstring = descpar->find( definitionStr() );
	if ( defstring.isEmpty() )
	    mHandleParseErr( tr("No attribute definition string specified") );

	BufferString attribname;
	if ( !Desc::getAttribName( defstring.buf(), attribname ) )
	    mHandleParseErr( uiStrings::sCantFindAttrName() );

	handleOldAttributes( attribname, *descpar, defstring, par.odVersion() );
	if ( errmsgs && !errmsg_.isEmpty() )
	    errmsgs->add( errmsg_ );

	RefMan<Desc> dsc;
	dsc = errmsgs ? createDesc( attribname, *descpar, defstring, errmsgs )
		      : createDesc( attribname, *descpar, defstring );
	if ( !dsc )
	    { res = false; continue; }

	uiString emsg = Provider::prepare( *dsc );
	if ( !emsg.isEmpty() )
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
				 uiStringSet* errmsgs )
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
	    const BufferString defstring = descpar->find( definitionStr() );
	    if ( defstring.isEmpty() )
		mHandleParseErr(tr("No attribute definition string specified"));
	    if ( !createSteeringDesc(*steeringpar,defstring,newsteeringdescs,
				     steeringdescid) )
		mHandleParseErr( tr("Cannot create steering definition"));

	    Desc* dsc = getDesc( DescID(id,false) );
	    for ( int idx=0; idx<dsc->nrInputs(); idx++ )
	    {
		BufferString inputstr = IOPar::compKey( sKey::Input(), idx );
		if ( descpar->find(inputstr).isEqual("-1") )
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
    (*errmsgs) += errmsg_; \
}


bool DescSet::createSteeringDesc( const IOPar& steeringpar,
				  BufferString defstring,
				  ObjectSet<Desc>& newsteeringdescs, int& id,
				  uiStringSet* errmsgs )
{
    const BufferString steeringtype = steeringpar.find( sKey::Type() );
    BufferString steeringdef( steeringtype );
    if ( steeringtype.isEqual("ConstantSteering") )
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
	    const BufferString aperture = steeringpar.find("Aperture");
	    steeringdef += aperture.isEmpty() ? "-5,5" : aperture;
	}
    }

    BufferString attribname;
    if ( !Desc::getAttribName(steeringdef,attribname) )
	mHandleSteeringParseErr(uiStrings::sCantFindAttrName());

    RefMan<Desc> stdesc = PF().createDescCopy(attribname);
    if ( !stdesc )
	mHandleSteeringParseErr( sFactoryEntryNotFound(attribname) );

    if ( !stdesc->parseDefStr(steeringdef) )
    {
	uiString err = tr("Cannot parse: %1").arg(steeringdef);
	mHandleSteeringParseErr(err);
    }

    BufferString usrrefstr = "steering input ";
    usrrefstr += newsteeringdescs.size();
    stdesc->setUserRef( usrrefstr );
    stdesc->setSteering(true);
    stdesc->setHidden(true);

    const BufferString inldipstr = steeringpar.find("InlDipID");
    if ( !inldipstr.isEmpty() )
    {
	DescID inldipid( inldipstr.toInt(), false );
	stdesc->setInput( 0, getDesc(inldipid) );
    }

    const BufferString crldipstr = steeringpar.find("CrlDipID");
    if ( !crldipstr.isEmpty() )
    {
	DescID crldipid( crldipstr.toInt(), false );
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


uiString DescSet::errMsg() const
{
    return errmsg_;
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


DescID DescSet::getStoredID( const MultiID& multiid, int selout ) const
{
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	const Desc& dsc = *descs_[idx];
	if ( !dsc.isStored() )
	    continue;

	const ValParam& keypar = *dsc.getValParam( StorageProvider::keyStr() );
	if ( multiid.isEqualTo(keypar.getStringValue()) )
	{
	    if ( selout < 0 || selout == dsc.selectedOutput() )
		return dsc.id();
	}
    }

    return DescID::undef();
}


DescID DescSet::getStoredID( const MultiID& multiid, int selout, bool create,
			     bool blindcomp, const char* blindcompnm )
{
    TypeSet<int> outsreadyforthislk;
    TypeSet<DescID> outsreadyids;
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	const Desc& dsc = *descs_[idx];
	const bool outnrisok = dsc.selectedOutput() == selout;
	if ( !dsc.isStored() || ( !outnrisok && selout>=0 ) )
	    continue;

	const ValParam& keypar = *dsc.getValParam( StorageProvider::keyStr() );
	if ( multiid.isEqualTo(keypar.getStringValue()) )
	{
	    if ( selout>=0 ) return dsc.id();
	    outsreadyforthislk += dsc.selectedOutput();
	    outsreadyids += dsc.id();
	}
    }

    if ( !create )
	return DescID::undef();

    if ( blindcomp )
	return createStoredDesc( multiid, selout, BufferString(
					    blindcompnm ? blindcompnm :"") );

    const int out0idx = outsreadyforthislk.indexOf( 0 );
    BufferStringSet bss; SeisIOObjInfo::getCompNames( multiid, bss );
    const int nrcomps = bss.size();
    if ( nrcomps < 2 )
	return out0idx != -1 ? outsreadyids[out0idx]
			     : createStoredDesc( multiid, 0, BufferString("") );

    const int startidx = selout<0 ? 0 : selout;
    const int stopidx = selout<0 ? nrcomps : selout;
    const BufferString& curstr = bss.validIdx(startidx)
				? bss.get(startidx) : BufferString::empty();
    const DescID retid = out0idx != -1
			? outsreadyids[out0idx]
			: createStoredDesc( multiid, startidx, curstr );
    for ( int idx=startidx+1; idx<stopidx; idx++ )
	if ( !outsreadyforthislk.isPresent(idx) )
	    createStoredDesc( multiid, idx, *bss[idx] );

    return retid;
}


DescID DescSet::createStoredDesc( const MultiID& multiid, int selout,
				  const BufferString& compnm )
{
    BufferString objnm;
    if ( multiid.isDatabaseID() )
    {
	PtrMan<IOObj> ioobj = IOM().get( multiid );
	if ( !ioobj )
	    return DescID::undef();

	objnm = ioobj->name();
    }
    else if ( multiid.isInMemoryID() )
    {
	DataPack::FullID fid( multiid );
	if ( !DPM(fid).isPresent(fid) )
	    return DescID::undef();

	objnm = DataPackMgr::nameOf( fid );
    }
    else
	return DescID::undef();

    Desc* newdesc = PF().createDescCopy( StorageProvider::attribName() );
    if ( !newdesc )
	return DescID::undef(); // "Cannot create desc"

    if ( compnm.isEmpty() && selout>0 )
	return DescID::undef();	// "Missing component name"

    newdesc->setUserRef( StringPair(objnm,compnm).getCompString() );
    newdesc->selectOutput( selout );
    ValParam& keypar = *newdesc->getValParam( StorageProvider::keyStr() );
    keypar.setValue( multiid );
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
    BufferString tmpstr;
    return isAttribUsed( id, tmpstr );
}


bool DescSet::isAttribUsed( const DescID& id, BufferString& depdescnm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Desc& dsc = *descs_[idx];
	for ( int inpnr=0; inpnr<dsc.nrInputs(); inpnr++ )
	{
	    if ( dsc.inputId(inpnr) == id )
	    {
		depdescnm = dsc.userRef();
		return true;
	    }
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
		if ( !isAttribUsed( descid ) )
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
	BufferString res;
	if ( ioobj )
	    res = ioobj->pars().find( "Type" );

	BufferString firstchar;
	if ( !res.isEmpty() )
	    firstchar = res[0];

	const bool issteer = firstchar.isEqual("S");
	if ( !usesteering && issteer )
	    continue;

	if ( (dsc.is2D() == is2D()) ) //TODO backward compatibility with 2.4
	    return const_cast<Desc*>( &dsc );
    }

    return 0;
}


MultiID DescSet::getStoredKey( const DescID& did ) const
{
    const Desc* dsc = getDesc( did );
    if ( !dsc || !dsc->isStored() )
	return MultiID::udf();

    return MultiID(dsc->getStoredID().buf());
}


void DescSet::getStoredNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	const Desc* dsc = desc( idx );
	if ( !dsc->isStored() )
	    continue;

	PtrMan<IOObj> ioobj = IOM().get( MultiID(dsc->getStoredID().buf()) );
	if ( !ioobj )
	{
	    BufferString usrref = dsc->userRef();
	    usrref.embed( '{', '}' );
	    nms.addIfNew( usrref );
	}
	else
	   nms.addIfNew( ioobj->name() );
    }
}


void DescSet::getAttribNames( BufferStringSet& nms, bool inclhidden ) const
{
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	const Desc* dsc = desc( idx );
	if ( (!inclhidden && dsc->isHidden()) || dsc->isStored() )
	    continue;

	nms.add( dsc->userRef() );
    }
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
	const MultiID& defkey = attrinf.ioobjids_.get(idx);
	const char* ioobjnm = attrinf.ioobjnms_.get(idx).buf();
	FileMultiString fms( BufferString("[",ioobjnm,"]") );
	fms += defkey;
	attrdefs.add( fms );
    }
}


void DescSet::fillInUIInputList( BufferStringSet& inplist ) const
{
    Attrib::SelInfo attrinf( this, 0, is2D(), DescID::undef(), false, false,
			     false, true );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
    {
	//This is horrible, trust I know it, but that's the only way to solve
	//the problem so close to the release date
	BufferString usrnm = attrinf.attrnms_.get(idx);
	if ( usrnm.startsWith("CentralSteering")
	  || usrnm.startsWith("FullSteering")
	  || usrnm.startsWith("ConstantSteering") )
	    continue;
	inplist.addIfNew( attrinf.attrnms_.get(idx) );
    }

    for ( int idx=0; idx<attrinf.ioobjnms_.size(); idx++ )
	inplist.addIfNew( BufferString("[",attrinf.ioobjnms_.get(idx),"]") );
    for ( int idx=0; idx<attrinf.steernms_.size(); idx++ )
	inplist.addIfNew( BufferString("[",attrinf.steernms_.get(idx),"]") );
}


Attrib::Desc* DescSet::getDescFromUIListEntry( FileMultiString inpstr )
{
    BufferString stornm = inpstr[0];
    if ( stornm.startsWith("[") )
    {
	stornm.unEmbed( '[', ']' );
	//generate Info with the same parameters as in fillInUIInputList
	//which is supposed to be the source of the input string.
	Attrib::SelInfo attrinf( this, 0, is2D(), DescID::undef(), false, false,
				 false, true );
	MultiID mid = MultiID::udf();
	int iidx = attrinf.ioobjnms_.indexOf( stornm.buf() );
	if ( iidx >= 0 )
	    mid = attrinf.ioobjids_.get( iidx );
	else
	{
	    iidx = attrinf.steernms_.indexOf( stornm.buf() );
	    if ( iidx >= 0 )
		mid = attrinf.steerids_.get( iidx );
	}

	if ( mid.isUdf() )
	    return 0;

	int compnr = 0;
	if ( !inpstr[1].isEmpty() )
	{
	    IOObj* inpobj = IOM().get( mid );
	    if ( inpobj )
	    {
		SeisIOObjInfo seisinfo( inpobj );
		BufferStringSet nms;
		seisinfo.getComponentNames( nms );
		compnr = nms.indexOf(inpstr[1]); //ALL will be -1 as expected
	    }
	}

	Attrib::DescID retid = getStoredID( mid,
					    compnr, true, true,
					    inpstr[1] );
	return getDesc( retid );
    }
    else
	for ( int dscidx=0; dscidx<descs_.size(); dscidx++ )
	    if ( descs_[dscidx] &&
		 BufferString(stornm)==BufferString(descs_[dscidx]->userRef()) )
		return descs_[dscidx];

    return 0;
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
	StringPair userref( newdesc->userRef() );
	userref.second() = seloutnms[idx]->buf();
	newdesc->setUserRef( userref.getCompString() );
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
	if ( !tmpdsc || (tmpdsc->isHidden() && !dsu.hidden_) ||
	     (dsu.stored_ != tmpdsc->isStored())  )
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


bool DescSet::exportToDot( const char* nm, const char* fnm ) const
{
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	return false;

    strm << "digraph {" << '\n';
    strm << "graph [label=\"" << nm << "\", labelloc=t, fontsize=30];\n";
    for ( int idx=0; idx<nrDescs(true,true); idx++ )
    {
	const Desc* curdesc = desc( idx );
	if ( !curdesc ) continue;

	const int nrinputs = curdesc->nrInputs();
	for ( int inpidx=0; inpidx<nrinputs; inpidx++ )
	{
	    const Desc* inpdesc = curdesc->getInput( inpidx );
	    if ( !inpdesc ) continue;

	    strm << "\"" << inpdesc->userRef() << "\" -> \""
		 << curdesc->userRef() << "\";\n";
	}
    }
    strm << "}\n";

    return true;
}

} // namespace Attrib
