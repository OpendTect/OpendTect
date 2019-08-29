/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2003
-*/


#include "attribdescset.h"

#include "attribdesc.h"
#include "attribdescsettr.h"
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
#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "mathexpression.h"
#include "odver.h"
#include "od_ostream.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "settings.h"
#include "separstr.h"
#include "stattype.h"
#include "survinfo.h"
#include "uistrings.h"
#include "ctxtioobj.h"


namespace Attrib
{

const char* DescSet::sKeyUseAutoAttrSet	= "dTect.Auto Attribute set";
const char* DescSet::sKeyAuto2DAttrSetID = "2DAttrset.Auto ID";
const char* DescSet::sKeyAuto3DAttrSetID = "3DAttrset.Auto ID";

static ObjectSet<DescSet>	global2d_;
static ObjectSet<DescSet>	global3d_;
static DescSet*	empty2d_	= new DescSet( true );
static DescSet*	empty3d_	= new DescSet( false );
static DescSet*	dummy2d_	= new DescSet( true );
static DescSet*	dummy3d_	= new DescSet( false );
static uiRetVal autoloadresult_;

bool DescSet::globalUsed( bool is2d )
{
    return is2d ? !global2d_.isEmpty() : !global3d_.isEmpty();
}


inline static DescSet& gtGDesc( bool is2d )
{
    auto& descs = is2d ? global2d_ : global3d_;
    if ( descs.isEmpty() )
	DescSet::initGlobalSet( is2d );

    return !descs.isEmpty() ? *descs.last()
			    : (is2d ? DescSet::dummy2D() : DescSet::dummy3D());
}

const DescSet&	DescSet::global2D()	{ return gtGDesc(true); }
const DescSet&	DescSet::global3D()	{ return gtGDesc(false); }
DescSet&	DescSet::global2D4Edit(){ return gtGDesc(true); }
DescSet&	DescSet::global3D4Edit(){ return gtGDesc(false); }
const DescSet&	DescSet::empty2D()	{ return *empty2d_; }
const DescSet&	DescSet::empty3D()	{ return *empty3d_; }
DescSet&	DescSet::dummy2D()	{ return *dummy2d_; }
DescSet&	DescSet::dummy3D()	{ return *dummy3d_; }
const uiRetVal&	DescSet::autoLoadResult() { return autoloadresult_; }


mClass(Attrib) Global_DescSet_Manager : public CallBacker
{
public:
Global_DescSet_Manager()
{
    DBM().afterSurveyChange.notify(
	    mCB(this,Global_DescSet_Manager,survChgCB) );
}

void survChgCB( CallBacker* )
{
    deepErase( global2d_ ); deepErase( global3d_ );
    dummy2d_->setEmpty(); dummy3d_->setEmpty();
}

};

PtrMan<Global_DescSet_Manager> standard_manager;

void Make_Global_DescSet_Manager()
{
    standard_manager = new Global_DescSet_Manager;
}


uiString DescSet::sFactoryEntryNotFound( const char* attrnm )
{
    return uiStrings::phrCannotCreate( tr("an instance of attribute %1")
					.arg(attrnm) );
}


DescSet::DescSet( bool is2d )
    : is2d_(is2d)
    , ischanged_(false)
    , descAdded(this)
    , descUserRefChanged(this)
    , descToBeRemoved(this)
    , descRemoved(this)
    , aboutToBeDeleted(this)
{
    ensureDefStoredPresent();
}


DescSet::DescSet( const DescSet& ds )
    : is2d_(ds.is2d_)
    , ischanged_(false)
    , descAdded(this)
    , descUserRefChanged(this)
    , descToBeRemoved(this)
    , descRemoved(this)
    , aboutToBeDeleted(this)
{
    *this = ds;
}


DescSet::~DescSet()
{
    aboutToBeDeleted.trigger();
    detachAllNotifiers();
    setEmpty();
}


DescSet& DescSet::operator =( const DescSet& oth )
{
    if ( &oth != this )
    {
	setEmpty();
	const_cast<bool&>(is2d_) = oth.is2d_;
	dbky_ = oth.dbky_;
	ischanged_ = oth.ischanged_;
	for ( int idx=0; idx<oth.size(); idx++ )
	    addDesc( new Desc( *oth.descs_[idx] ), oth.ids_[idx] );
	updateInputs();
    }
    return *this;
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
    if ( DBM().isBad() )
	return DescID();
    return defStoredID();
}


DescID DescSet::defStoredID() const
{
    PtrMan<IOPar> defpars = SI().getDefaultPars().subselect( sKey::Default() );
    BufferString idstr;
    if ( defpars )
    {
	if ( is2d_ )
	    idstr = mGetPar( sKeyDefault2D );
	else
	    idstr = mGetPar( sKeyDefault3D );
    }

    return const_cast<DescSet*>(this)->getStoredID(
			    DBKey(idstr), 0, true, true );
}


DescID DescSet::ensureStoredPresent( const DBKey& dbky, int compnr ) const
{
    return const_cast<DescSet*>(this)->getStoredID(
			    dbky, compnr, true, compnr < 0 );
}



DescID DescSet::addDesc( Desc* nd, DescID id )
{
    nd->setDescSet( this );
    nd->ref();

    descs_ += nd;
    const DescID newid = id.isValid() ? id : getFreeID();
    ids_ += newid;

    descAdded.trigger( newid );
    mAttachCB( nd->userRefChanged, DescSet::usrRefChgCB );
    return newid;
}


DescID DescSet::insertDesc( Desc* nd, int idx, DescID id )
{
    nd->setDescSet( this ); nd->ref();
    descs_.insertAt( nd, idx );
    const DescID newid = id.isValid() ? id : getFreeID();
    ids_.insert( idx, newid );

    descAdded.trigger( newid );
    mAttachCB( nd->userRefChanged, DescSet::usrRefChgCB );
    return newid;
}


void DescSet::usrRefChgCB( CallBacker* cb )
{
    mDynamicCastGet( Desc*, attrdesc, cb );
    if ( !attrdesc )
	{ pErrMsg("Huh"); return; }
    const int didx = descs_.indexOf( attrdesc );
    if ( didx < 0 )
	{ pErrMsg("Probably bad"); return; }

    descUserRefChanged.trigger( ids_[didx] );
}


Desc* DescSet::gtDesc( const DescID& id ) const
{
    const int idx = indexOf( id );
    if ( !descs_.validIdx(idx) )
	return 0;

    return const_cast<Desc*>( descs_[idx] );
}


bool DescSet::hasTrueAttribute() const
{
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	if ( !descs_[idx]->isStored() )
	    return true;
    }
    return false;
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
    return idx==-1 ? DescID() : ids_[idx];
}


DescID DescSet::getID( int idx ) const
{
    return ids_.validIdx(idx) ? ids_[idx] : DescID();
}


void DescSet::getIds( TypeSet<DescID>& attribids ) const
{
    attribids = ids_;
}


void DescSet::getStoredIds( TypeSet<DescID>& attribids ) const
{
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	if ( descs_[idx]->isStored() )
	    attribids += ids_[idx];
    }
}


const Desc* DescSet::getGlobalDesc( const SelSpec& spec )
{
    return getGlobalDesc( spec.is2D(), spec.id() );
}


DescID DescSet::getID( const char* str, bool isusrref, bool isdescstored,
		       bool usestorinfo	) const
{
    if ( !str || !*str )
	return DescID();

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

    return DescID();
}


DescID DescSet::getDefaultTargetID() const
{
    for ( int idx=size()-1; idx!=-1; idx-- )
    {
	const Desc& ad = *descs_[idx];
	if ( !ad.isStored() )
	    return ad.id();
    }
    return ensureDefStoredPresent();
}


void DescSet::removeDesc( const DescID& id )
{
    const int idx = indexOf( id );
    if ( idx < 0 )
	return;

    descToBeRemoved.trigger( id );

    if ( descs_[idx]->descSet()==this )
	descs_[idx]->setDescSet(0);

    descs_.removeSingle(idx)->unRef();
    ids_.removeSingle(idx);

    descRemoved.trigger( id );
}


void DescSet::moveDescUpDown( const DescID& id, bool moveup )
{
    const int sz = ids_.size();
    const int selidx = indexOf( id );
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

    ObjectSet<Desc> descskeep( descs_ );
    TypeSet<DescID> idskeep( ids_ );

    int* sortindexes = userrefs.getSortIndexes();
    descs_.erase(); ids_.erase();
    for ( int idx=0; idx<nrdescs; idx++ )
    {
	descs_ += descskeep[ sortindexes[idx] ];
	ids_ += idskeep[ sortindexes[idx] ];
    }

    delete [] sortindexes;
}


void DescSet::setEmpty()
{
    while ( ids_.size() )
	removeDesc( ids_[0] );
}


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

	const DBKey dbky = dsc.getStoredID( true );
	const bool isvaliddbkey = dbky.isValid();
	PtrMan<IOObj> ioobj = dbky.getIOObj();
	if ( isvaliddbkey && !ioobj )
            continue;

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
	    apar.set( key, getID( *dsc.getInput(input) ).getI() );
	}

	par.updateComp( apar, BufferString("",ids_[idx].getI()) );

	if ( ids_[idx].getI() > maxid )
	    maxid = ids_[idx].getI();
    }

    par.set( highestIDStr(), maxid );
    par.set( sKey::Type(), is2d_ ? "2D" : "3D");
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


uiRetVal DescSet::handleOldAttributes( BufferString& attribname, IOPar& descpar,
	                           BufferString& defstring,
				   int odversion ) const
{
    uiRetVal uirv;

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
	uirv.add( handleOldMathExpression(descpar,defstring,odversion) );

    if ( attribname == "FingerPrint" )
    {
	BufferStringSet keys;
	BufferStringSet vals;
	Attrib::Desc::getKeysVals( defstring.buf(), keys, vals );
	const int statkeyidx = keys.indexOf( "statstype" );
	if ( statkeyidx<0 )
	    return uiRetVal( tr("Cannot load old FingerPrint attribute") );
	if ( isNumberString( vals[statkeyidx]->buf(), true ) )
	{
	    const int var = toInt( vals[statkeyidx]->buf() );
	    int statstype = var + 1;
	    statstype += (statstype < (int)Stats::RMS ? 0 : 2);
	    statstype += (statstype < (int)Stats::NormVariance ? 0 : 1);
	    //!< Count, RMS, StdDev, NormVariance not used, so skip them
	    BufferString basestr = "statstype=";
	    BufferString initialstr( basestr, var );
	    BufferString finalstr( basestr,
				   Stats::TypeDef().getKeyForIndex(statstype) );
	    defstring.replace( initialstr, finalstr );
	}
    }
    if ( attribname == "Frequency" )
    {
	BufferStringSet keys;
	BufferStringSet vals;
	Attrib::Desc::getKeysVals( defstring.buf(), keys, vals );
	if ( !keys.isPresent( "smoothspect" ) )
	    defstring.add( " smoothspect=No" );
    }

    return uirv;
}


uiRetVal DescSet::handleOldMathExpression( IOPar& descpar,
				       BufferString& defstring,
				       int odversion ) const
{
    const uiRetVal erruirv = tr("Cannot use old Math expression");
    RefMan<Desc> tmpdesc = PF().createDescCopy("Math");
    if ( !tmpdesc || !tmpdesc->parseDefStr(defstring.buf()) )
	return erruirv;
    ValParam* expr = tmpdesc->getValParam( "expression" );
    if ( !expr )
	return erruirv;
    Math::ExpressionParser mep( expr->getStringValue() );
    PtrMan<Math::Expression> formula = mep.parse();
    if ( !formula )
	return erruirv;

    if ( odversion<340 )
    {
	TypeSet<int> oldinputs;
	TypeSet<int> correctinputs;
	int inputidx = 0;
	while( true )
	{
	    int inpid;
	    const char* key = IOPar::compKey( inputPrefixStr(), inputidx );
	    if ( !descpar.get(key,inpid) )
		break;
	    oldinputs += inpid;
	    inputidx++;
	}

	for ( int idx=0; idx<formula->nrUniqueVarNames(); idx++ )
	{
	    if ( Math::ExpressionParser::varTypeOf( formula->uniqueVarName(idx))
		    != Math::Expression::Variable )
		continue;

	    const BufferString varnm( formula->uniqueVarName(idx) );
	    const char* ptr = varnm.buf();
	    while ( *ptr && !iswdigit(*ptr) )
		ptr++;

	    int varxidx = toInt( ptr );
	    if ( varxidx >= oldinputs.size() )
		return erruirv;
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
	const BufferString varnm ( formula->uniqueVarName(idx) );

	if ( varnm == "DZ" || varnm == "Inl" || varnm == "Crl"
	  || varnm == "XCoord" || varnm == "YCoord" || varnm == "Z" )
	{
	    BufferString alternativenm ( varnm, "Input" );
	    defstring.replace( varnm.buf(), alternativenm.buf() );
	}
    }

    return uiRetVal::OK();
}


#define mHandleParseErr( str ) { uirv.add( str ); continue; }
#define mHandleDescErr( str ) { uirv.add( str ); return 0; }


Desc* DescSet::createDesc( const BufferString& attrname, const IOPar& descpar,
			   const BufferString& defstring, uiRetVal& uirv )
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

    BufferString userref = descpar.find( userRefStr() );
    if ( dsc->isStored() )
    {
	const ValParam* keypar = dsc->getValParam( StorageProvider::keyStr() );
	const StringPair storkey( keypar->getStringValue() );
	PtrMan<IOObj> ioobj = DBKey(storkey.first()).getIOObj();
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
    dsc->setIsHidden( ishidden );

    int selout = dsc->selectedOutput();
    bool selectout = descpar.get("Selected Attrib",selout);
    if ( dsc->isStored() )
    {
	FixedString type = descpar.find( sKey::DataType() );
	if ( type=="Dip" )
	{
	    dsc->setNrOutputs( Seis::Dip, 2 );
	    dsc->setIsSteering( true );
	}
	else
	    dsc->changeOutputDataType( selout, Seis::dataTypeOf(type) );
    }

    if ( selectout )
	dsc->selectOutput(selout);

    return dsc;
}


void DescSet::handleReferenceInput( Desc* dsc )
{
    if ( Desc::isError(dsc->satisfyLevel()) )
    {
	Desc* inpdesc = getFirstStored( false );
	if ( !inpdesc )
	    return;

	dsc->setInput( 0, inpdesc );
    }
}


uiRetVal DescSet::setAllInputDescs( int nrdescsnosteer, const IOPar& copypar )
{
    uiRetVal uirv;

    TypeSet<int> toberemoved;
    for ( int idx=0; idx<nrdescsnosteer; idx++ )
    {
	PtrMan<IOPar> descpar =
	    copypar.subselect( toString(ids_[idx].getI()) );
	if ( !descpar )
	    { pErrMsg("Huh?"); continue; }

	Desc& dsc = *descs_[idx];
	for ( int input=0; input<dsc.nrInputs(); input++ )
	{
	    const BufferString key = IOPar::compKey( inputPrefixStr(), input );

	    int inpid;
	    if ( !descpar->get(key,inpid) )
		continue;
	    Desc* inpdesc = getDesc( DescID(inpid) );
	    if ( !inpdesc )
		continue;

	    dsc.setInput( input, inpdesc );
	}

	if ( dsc.attribName()=="Reference" )
	    handleReferenceInput( &dsc );

	Desc::SatisfyLevel lvl = dsc.satisfyLevel();
	if ( Desc::isError(lvl) )
	{
	    uiString err;
	    const bool storagenotfound = lvl == Desc::StorNotFound;
	    if ( storagenotfound && dsc.isStoredInMem() )
		continue;

	    if ( storagenotfound && dsc.isStored() )
	    {
		IOPar tmpcpypar( copypar );
		BufferString depattribnm;
		bool found = true;
		while ( found )
		{
		    const CompoundKey compkey =
			tmpcpypar.findKeyFor( toString(ids_[idx].getI()) );
		    found = !compkey.isEmpty();
		    if ( found )
		    {
			if ( compkey.nrKeys()>1
			  && compkey.key(1) == sKey::Input() )
			{
			    CompoundKey usrrefkey(compkey.key(0));
			    usrrefkey += userRefStr();
			    copypar.get( usrrefkey.buf(), depattribnm );
			    if ( depattribnm.find( "FullSteering")
				 || depattribnm.find( "CentralSteering") )
			    {
				depattribnm.setEmpty();
				bool foundsecondorder = true;
				while ( foundsecondorder )
				{
				    const CompoundKey compkey2 =
					tmpcpypar.findKeyFor(
						toString(compkey.key(0)) );
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
		err = tr("Impossible to find stored data '%1' "
			"used as input for another attribute '%2'")
			.arg( dsc.userRef() )
			.arg( depattribnm.isEmpty() ? uiString::empty()
					    : toUiString(depattribnm))
		.appendPhrase(tr("Data might have been deleted or corrupted"))
		.appendPhrase(tr("Please check your attribute set "
			      "and select valid stored data as input.") );
	    }
	    else
	    {
		err = toUiString( "%1 (%2)")
			.arg( dsc.errMsg() ).arg( dsc.userRef() );
	    }

            toberemoved += idx;
	    mHandleParseErr(err);
	}
    }

    for ( int idx=toberemoved.size()-1; idx>=0; idx-- )
        removeDesc( descs_[toberemoved[idx]]->id() );

    return uirv;
}


uiRetVal DescSet::usePar( const IOPar& par )
{
    uiRetVal uirv;

    const char* typestr = par.find( sKey::Type() );
    if ( typestr )
    {
	const bool is2d = *typestr == '2';
	const bool is3d = *typestr == '3';
	if ( (is2d_ && is3d) || (!is2d_ && is2d) )
	    const_cast<bool&>(is2d_) = !is2d_;
    }

    setEmpty();

    int maxid = 1024;
    par.get( highestIDStr(), maxid );
    IOPar copypar(par);

    TypeSet<int> indexes;
    for ( int id=0; id<=maxid; id++ )
    {
	PtrMan<IOPar> descpar = par.subselect( toString(id) );
	if ( !descpar )
	    continue;

	handleStorageOldFormat( *descpar );

	BufferString defstring = descpar->find( definitionStr() );
	if ( defstring.isEmpty() )
	    mHandleParseErr( tr("No attribute definition string specified") );

	BufferString attribname;
	if ( !Desc::getAttribName( defstring.buf(), attribname ) )
	    mHandleParseErr( uiStrings::phrCannotFindAttrName() );

	uirv.add( handleOldAttributes(attribname,*descpar,defstring,
					par.odVersion()) );
	RefMan<Desc> dsc;
	dsc = createDesc( attribname, *descpar, defstring, uirv );
	if ( !dsc )
	    continue;

	uiString emsg = Provider::prepare( *dsc );
	if ( !emsg.isEmpty() )
	    { uirv.add( emsg ); continue; }

	int idx=-1;
	descpar->get( indexStr(), idx );
	indexes += idx;

	dsc->updateParams();
	addDesc( dsc, DescID(id) );
	copypar.updateComp( *descpar, toString(id) );
    }

    // sort_coupled();
    ObjectSet<Desc> newsteeringdescs;
    uirv.add( useOldSteeringPar(copypar,newsteeringdescs) );

    for( int idx=0 ; idx<newsteeringdescs.size() ; idx++ )
	addDesc( newsteeringdescs[idx], DescID( maxid+idx+1 ) );

    int nrdescsnosteer = ids_.size() - newsteeringdescs.size();
    return uirv.add( setAllInputDescs(nrdescsnosteer,copypar) );
}


uiRetVal DescSet::useOldSteeringPar( IOPar& par,
				     ObjectSet<Desc>& newsteeringdescs )
{
    uiRetVal uirv;

    int maxid = 1024;
    par.get( highestIDStr(), maxid );
    for ( int id=0; id<=maxid; id++ )
    {
	PtrMan<IOPar> descpar = par.subselect( BufferString("",id) );
	if ( !descpar )
	    continue;

	int steeringdescid = -1;
	const IOPar* steeringpar = descpar->subselect( "Steering" );
	if ( steeringpar )
	{
	    const char* defstring = descpar->find( definitionStr() );
	    if ( !defstring )
		mHandleParseErr(tr("No attribute definition string specified"));
	    uiRetVal uisteerrv = createSteeringDesc( *steeringpar, defstring,
				    newsteeringdescs, steeringdescid );
	    if ( !uisteerrv.isOK() )
		{ uirv.add( uisteerrv ); continue; }

	    Desc* dsc = getDesc( DescID(id) );
	    for ( int idx=0; idx<dsc->nrInputs(); idx++ )
	    {
		BufferString inputstr = IOPar::compKey( sKey::Input(), idx );
		if ( FixedString(descpar->find(inputstr))=="-1" )
		{
		    const char* newkey =
			IOPar::compKey(toString(id),inputstr);
		    par.set( newkey, maxid + steeringdescid +1 );
		}
	    }
	}
    }

    return uirv;
}


void DescSet::pushGlobal( bool is2d, DescSet* ds )
{
    (is2d ? global2d_ : global3d_) += ds;
}


DescSet* DescSet::popGlobal( bool is2d )
{
    ObjectSet<DescSet>& globs = is2d ? global2d_ : global3d_;
    const int sz = globs.size();
    if ( sz < 2 )
	{ pFreeFnErrMsg("never pop the base set"); return 0; }
    return globs.removeSingle( sz-1 );
}


void DescSet::initGlobalSet( bool is2d )
{
    static Threads::Lock lock_;
    Threads::Locker locker( lock_ );

    auto& descs = is2d ? global2d_ : global3d_;
    if ( !descs.isEmpty() )
	return; // another thread has beat us to it

    autoloadresult_.setEmpty();

    DescSet* gds = new DescSet( is2d );
    descs += gds;

    bool loadauto = false;
    Settings::common().getYN( sKeyUseAutoAttrSet, loadauto );
    if ( loadauto )
    {
	const char* ky = is2d ? sKeyAuto2DAttrSetID : sKeyAuto3DAttrSetID;
	DBKey dbky;
	if ( SI().getDefaultPars().get(ky,dbky) )
	    autoloadresult_.add( gds->load(dbky) );
    }
}


IOObjContext* DescSet::getIOObjContext( bool forread )
{
    IOObjContext* ctxt = new IOObjContext( mIOObjContext(AttribDescSet) );
    ctxt->forread_ = forread;
    return ctxt;
}


IOObjContext* DescSet::getIOObjContext( bool forread, bool is2d )
{
    IOObjContext* ctxt = getIOObjContext( forread );
    ctxt->toselect_.dontallow_.set( sKey::Type(), is2d ? "3D" : "2D" );
    return ctxt;
}


CtxtIOObj* DescSet::getCtxtIOObj( bool forread ) const
{
    IOObjContext* ctxt = getIOObjContext( forread, is2D() );
    CtxtIOObj* ctio = new CtxtIOObj( *ctxt );
    delete ctxt;
    ctio->setObj( storeID() );
    return ctio;
}


BufferString DescSet::name() const
{
    return dbky_.name();
}


uiRetVal DescSet::store( const DBKey& dbky ) const
{
    PtrMan<IOObj> ioobj = dbky.getIOObj();
    uiRetVal uirv;
    if ( !ioobj )
	uirv.add( uiStrings::phrCannotFindDBEntry(dbky_) );
    else
    {
	DescSet stords( *this );
	stords.removeUnused( true );
	uirv = AttribDescSetTranslator::store( stords, ioobj );
	if ( uirv.isOK() )
	    const_cast<DescSet*>(this)->dbky_ = dbky;
    }
    return uirv;
}


uiRetVal DescSet::load( const DBKey& dbky, uiRetVal* warns )
{
    PtrMan<IOObj> ioobj = dbky.getIOObj();
    if ( !ioobj )
	return uiRetVal( uiStrings::phrCannotFindDBEntry(dbky) );

    DescSet loadset( is2D() );
    uiRetVal uirv = AttribDescSetTranslator::retrieve( loadset, ioobj, warns );
    if ( uirv.isOK() )
    {
	loadset.removeUnused( true );
	*this = loadset;
	setIsChanged( false );
	dbky_ = dbky;
    }

    return uirv;
}


uiRetVal DescSet::load( const char* fnm, uiRetVal* warns )
{
    DescSet loadset( false );
    uiRetVal uirv = AttribDescSetTranslator::retrieve( loadset, fnm, warns );
    if ( uirv.isOK() )
    {
	*this = loadset;
	setIsChanged( false );
	dbky_.setInvalid();
    }
    return uirv;
}


uiRetVal DescSet::createSteeringDesc( const IOPar& steeringpar,
				      BufferString defstring,
				  ObjectSet<Desc>& newsteeringdescs, int& id )
{
    uiRetVal uirv;

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
	uirv.add( uiStrings::phrCannotFindAttrName() );

    RefMan<Desc> stdesc = PF().createDescCopy(attribname);
    if ( !stdesc )
	uirv.add( sFactoryEntryNotFound(attribname) );

    if ( !stdesc->parseDefStr(steeringdef) )
	uirv.add( tr("Cannot parse: %1").arg(steeringdef) );

    BufferString usrrefstr = "steering input ";
    usrrefstr += newsteeringdescs.size();
    stdesc->setUserRef( usrrefstr );
    stdesc->setIsSteering(true);
    stdesc->setIsHidden(true);

    const char* inldipstr = steeringpar.find("InlDipID");
    if ( inldipstr )
    {
	DescID inldipid( toInt(inldipstr) );
	stdesc->setInput( 0, getDesc(inldipid) );
    }

    const char* crldipstr = steeringpar.find("CrlDipID");
    if ( crldipstr )
    {
	DescID crldipid( toInt(crldipstr) );
	stdesc->setInput( 1, getDesc(crldipid) );
    }

//TODO see what's going on for the phase input
    for ( int idx=0; idx<newsteeringdescs.size(); idx++ )
    {
	if ( stdesc->isIdenticalTo(*newsteeringdescs[idx]) )
	{
	    id = idx;
	    return uirv;
	}
    }
    stdesc->ref();
    newsteeringdescs += stdesc;
    id = newsteeringdescs.size()-1;

    return uirv;
}


DescID DescSet::getFreeID() const
{
    int highestid = -1;
    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	const int index = ids_[idx].getI();
	if ( index > highestid )
	    highestid = index;
    }

    return DescID( highestid+1 );
}


DescID DescSet::getStoredID( const DBKey& dbkey, int selout, bool add_if_absent,
			     bool blindcomp, const char* blindcompnm ) const
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
	if ( dbkey.toString() == keypar.getStringValue() )
	{
	    if ( selout>=0 )
		return dsc.id();
	    outsreadyforthislk += dsc.selectedOutput();
	    outsreadyids += dsc.id();
	}
    }

    if ( !add_if_absent )
	return DescID();

    DescSet& self = *const_cast<DescSet*>( this );

    if ( blindcomp )
	return self.createStoredDesc( dbkey, selout, BufferString(
					    blindcompnm ? blindcompnm :"") );

    const int out0idx = outsreadyforthislk.indexOf( 0 );
    SeisIOObjInfo ioobjinf( dbkey ); BufferStringSet compnms;
    ioobjinf.getComponentNames( compnms );
    const int nrcomps = compnms.size();
    if ( nrcomps < 2 )
	return out0idx != -1 ? outsreadyids[out0idx]
			 : self.createStoredDesc( dbkey, 0, BufferString("") );

    const int startidx = selout<0 ? 0 : selout;
    const int stopidx = selout<0 ? nrcomps : selout;
    const BufferString& curstr = compnms.validIdx(startidx)
				? compnms.get(startidx) : BufferString::empty();
    const DescID retid = out0idx != -1
			? outsreadyids[out0idx]
			: self.createStoredDesc( dbkey, startidx, curstr );
    for ( int idx=startidx+1; idx<stopidx; idx++ )
	if ( !outsreadyforthislk.isPresent(idx) )
	    self.createStoredDesc( dbkey, idx, compnms.get(idx) );

    return retid;
}


DescID DescSet::createStoredDesc( const DBKey& dbkey, int selout,
				  const BufferString& compnm )
{
    BufferString objnm;
    if ( DataPack::FullID::isInDBKey(dbkey) )
    {
	DataPack::FullID fid = DataPack::FullID::getFromDBKey( dbkey );
	if ( !DPM(fid).isPresent( fid ) )
	    return DescID();

	objnm = DataPackMgr::nameOf( fid );
    }
    else
    {
	PtrMan<IOObj> ioobj = dbkey.getIOObj();
	if ( !ioobj )
	    return DescID();

	objnm = ioobj->name();
    }

    Desc* newdesc = PF().createDescCopy( StorageProvider::attribName() );
    if ( !newdesc ) return DescID(); // "Cannot create desc"
    if ( compnm.isEmpty() && selout>0 )
	return DescID();	// "Missing component name"

    const StringPair strpair( objnm, compnm );
    newdesc->setUserRef( strpair.getCompString() );
    newdesc->selectOutput( selout );
    ValParam& keypar = *newdesc->getValParam( StorageProvider::keyStr() );
    keypar.setValue( dbkey.toString() );
    newdesc->updateParams();
    return addDesc( newdesc );
}


DescSet* DescSet::optimizeClone( const DescID& targetnode ) const
{
    return optimizeClone( TypeSet<DescID>(targetnode) );
}


DescSet* DescSet::optimizeClone( const TypeSet<DescID>& targets ) const
{
    DescSet* res = new DescSet( is2d_ );
    res->setEmpty();
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
	    const DescID inputid = inpdesc ? inpdesc->id() : DescID();
	    if ( inputid.isValid() && !res->getDesc(inputid) )
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
    for ( int idx=0; idx<targetsstr.size(); idx++ )
	needednodes += getID( targetsstr.get( idx ), true );

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
	    if ( kpdefault && !descidx )
		continue; //default desc always first

	    DescID descid = getID( descidx );
	    if ( torem.isPresent(descid) ) continue;

	    const Desc& dsc = *getDesc( descid );
	    bool iscandidate = false;
	    if ( dsc.isStored() )
	    {
		const ValParam* keypar =
			dsc.getValParam( StorageProvider::keyStr() );
		const DBKey dbky( keypar->getStringValue() );
		if ( remstored || implExists(dbky) )
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

	if ( count == 0 )
	    break;
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

	DBKey storedid = dsc.getStoredID();
	if ( storedid.isInvalid() ) continue;

	PtrMan<IOObj> ioobj = storedid.getIOObj();
	const char* res = ioobj ? ioobj->pars().find( "Type" ) : 0;
	const bool issteer = res && *res == 'S';
	if ( !usesteering && issteer )
	    continue;

	if ( (dsc.is2D() == is2D()) )
	    return const_cast<Desc*>( &dsc );
    }

    return 0;
}


DBKey DescSet::getStoredKey( const DescID& did ) const
{
    const Desc* dsc = getDesc( did );
    if ( !dsc || !dsc->isStored() )
	return DBKey::getInvalid();

    return dsc->getStoredID();
}


void DescSet::getStoredNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	const Desc* dsc = desc( idx );
	if ( !dsc->isStored() )
	    continue;

	PtrMan<IOObj> ioobj = dsc->getStoredID().getIOObj();
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
    Attrib::SelInfo attrinf( *this );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
    {
	BufferString defstr;
	const Attrib::Desc* mydesc = getDesc( attrinf.attrids_[idx] );
	if ( mydesc )
	    mydesc->getDefStr( defstr );
	FileMultiString fms( defstr ); fms += attrinf.attrids_[idx].getI();
	attrdefs.add( fms );
    }
    for ( int idx=0; idx<attrinf.ioobjids_.size(); idx++ )
    {
	const BufferString defkey = attrinf.ioobjids_.get(idx).toString();
	const char* ioobjnm = attrinf.ioobjnms_.get(idx).buf();
	FileMultiString fms( BufferString("[",ioobjnm,"]") );
	fms += defkey;
	attrdefs.add( fms );
    }
}


void DescSet::fillInUIInputList( BufferStringSet& inplist ) const
{
    // list including hidden attributes. Bert has no idea why.
    Attrib::SelInfo attrinf( this, DescID(), 0, is2D(), false, true );
    for ( int idx=0; idx<attrinf.attrnms_.size(); idx++ )
    {
	// Handle legacy stuff
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


Attrib::Desc* DescSet::getDescFromUIListEntry( const StringPair& inpstr )
{
    BufferString stornm = inpstr.first();
    if ( stornm.startsWith("[") )
    {
	stornm.unEmbed( '[', ']' );
	//generate Info with the same parameters as in fillInUIInputList
	//which is supposed to be the source of the input string.
	// Bert says: so, list including hidden attributes.
	Attrib::SelInfo attrinf( this, DescID(), 0, is2D(), false, true);
	DBKey dbky;
	int iidx = attrinf.ioobjnms_.indexOf( stornm.buf() );
	if ( iidx >= 0 )
	    dbky = attrinf.ioobjids_.get( iidx );
	else
	{
	    iidx = attrinf.steernms_.indexOf( stornm.buf() );
	    if ( iidx >= 0 )
		dbky = attrinf.steerids_.get( iidx );
	}

	if ( dbky.isInvalid() )
	    return 0;

	int compnr = 0;
	if ( !inpstr.second().isEmpty() )
	{
	    PtrMan<IOObj> inpobj = dbky.getIOObj();
	    if ( inpobj )
	    {
		SeisIOObjInfo seisinfo( inpobj );
		BufferStringSet nms;
		seisinfo.getComponentNames( nms );
		compnr = nms.indexOf( inpstr.second() );
			 //ALL will be -1 as expected
	    }
	}

	const Attrib::DescID retid = getStoredID( dbky, compnr, true, true,
						  inpstr.second() );
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
	newdesc->setUserRef( seloutnms[idx]->buf() );
	outdescids += addDesc( newdesc );
    }
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


void DescSet::fillInSelSpecs( DescSetup dsu, SelSpecList& specs ) const
{
    //TODO check all dsu cases
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	const Desc* tmpdsc = desc(idx);
	if ( !tmpdsc || (tmpdsc->isHidden() && !dsu.hidden_) ||
	     (dsu.stored_ != tmpdsc->isStored())  )
	    continue;

	specs.add( SelSpec(0,tmpdsc->id()) );
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
