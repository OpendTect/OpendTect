/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribdesc.h"

#include "attribparam.h"
#include "attribdescset.h"
#include "attribstorprovider.h"
#include "ioman.h"
#include "ioobj.h"
#include "seistrctr.h"
#include "survinfo.h"

namespace Attrib
{

DescSetup::DescSetup()
    : is2d_(false)
    , ps_(false)
    , singletraceonly_(true)
    , usingtrcpos_(true)
    , depthonly_(!SI().zIsTime())
    , timeonly_(SI().zIsTime())
    , hidden_(false)
    , steering_(false)
    , stored_(false)
{
}


DescSetup::~DescSetup()
{}



// InputSpec
InputSpec::InputSpec( const char* desc, bool enabled )
    : desc_(desc)
    , enabled_(enabled)
{}


InputSpec::~InputSpec()
{}


bool InputSpec::operator==(const InputSpec& b) const
{
    if ( desc_!=b.desc_ || enabled_!=b.enabled_ || issteering_!=b.issteering_ )
	return false;

    for ( int idx=0; idx<forbiddenDts_.size(); idx++ )
    {
	if ( !b.forbiddenDts_.isPresent(forbiddenDts_[idx]) )
	    return false;
    }

    for ( int idx=0; idx<b.forbiddenDts_.size(); idx++ )
    {
	if ( !forbiddenDts_.isPresent(b.forbiddenDts_[idx]) )
	    return false;
    }

    return true;
}



// Attrib::DescID
DescID::DescID()
    : id_(-1)
    , isstored_(false)
{}


DescID::DescID( int id, bool isstored )
    : id_(id)
    , isstored_(isstored)
{}


DescID::DescID( const DescID& id )
    : id_(id.id_)
    , isstored_(id.isstored_)
{}


DescID::~DescID()
{}



// Attrib::Desc
const char* Desc::sKeyOutput() { return "output"; }
const char* Desc::sKeyAll() { return "ALL"; }
const char* Desc::sKeyInlDipComp() { return "Inline Dip"; }
const char* Desc::sKeyCrlDipComp() { return "Crossline Dip"; }
const char* Desc::sKeyLineDipComp() { return "Line Dip"; }


Desc::Desc( const char* attribname, DescStatusUpdater updater,
	    DescDefaultsUpdater defupdater )
    : attribname_( attribname )
    , statusupdater_( updater )
    , defaultsupdater_( defupdater )
{
    attribname_.replace( ' ', '_' );
    inputs_.setNullAllowed();
}


Desc::Desc( const Desc& a )
    : issteering_( a.issteering_ )
    , hidden_( a.hidden_ )
    , needprovinit_( a.needprovinit_ )
    , is2d_(a.is2d_)
    , isps_(a.isps_)
    , locality_(a.locality_)
    , usestrcpos_(a.usestrcpos_)
    , attribname_( a.attribname_ )
    , userref_( a.userref_ )
    , seloutput_( a.seloutput_ )
    , descset_( a.descset_ )
    , statusupdater_( a.statusupdater_ )
{
    inputs_.setNullAllowed();

    for ( int idx=0; idx<a.params_.size(); idx++ )
	addParam( a.params_[idx]->clone() );

    for ( int idx=0; idx<a.inputs_.size(); idx++ )
    {
	addInput( a.inputSpec(idx) );
	if ( a.inputs_[idx] )
	    setInput( idx, a.inputs_[idx] );
    }

    for ( int idx=0; idx<a.nrOutputs(); idx++ )
	addOutputDataType( a.outputtypes_[idx] );
}


Desc::~Desc()
{
    deepErase( params_ );
}


const OD::String& Desc::attribName() const
{
    return attribname_;
}


void Desc::setDescSet( DescSet* nds )
{
    descset_ = nds;
    if ( nds )
	set2D( nds->is2D() );
}


DescSet* Desc::descSet() const
{
    return descset_;
}


DescID Desc::id() const
{
    return descset_ ? descset_->getID(*this) : DescID(-1,true);
}


bool Desc::getDefStr( BufferString& res ) const
{
    res = attribName();
    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( !params_[idx]->isEnabled() ) continue;
	res += " ";

	params_[idx]->fillDefStr( res );
    }

    if ( seloutput_!=-1 || StringPair(userref_).second() == sKeyAll() )
    {
	res += " output=";
	res += seloutput_;
    }

    return true;
}


bool Desc::parseDefStr( const char* defstr )
{
    BufferString defstrnm;
    if ( !getAttribName(defstr,defstrnm) || defstrnm!=attribname_ )
	return false;

    BufferString outputstr;
    bool res = getParamString( defstr, sKeyOutput(), outputstr );
    selectOutput( res ? outputstr.toInt() : 0 );

    BufferStringSet keys, vals;
    getKeysVals( defstr, keys, vals );

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	bool found = false;
	if ( !params_[idx]->isGroup() )
	{
	    BufferString paramval;
	    for ( int idy=0; idy<keys.size(); idy++ )
	    {
		if ( keys.get(idy) == params_[idx]->getKey() )
		{
		    paramval = vals.get(idy);
		    found = true;
		    break;
		}
	    }

	    if ( !found )
	    {
		if ( params_[idx]->isRequired() )
		     continue;
		else
		    paramval = params_[idx]->getDefaultValue();
	    }

	    if ( !params_[idx]->setCompositeValue(paramval) )
		return false;
	}
	else
	{
	    BufferStringSet paramvalset;
	    int valueidx = 0;
	    for ( int idy=0; idy<keys.size(); idy++ )
	    {
		BufferString keystring = params_[idx]->getKey();
		keystring += valueidx;
		if ( keys.get(idy) == keystring )
		{
		    paramvalset.add( vals.get(idy).buf() );
		    found =true;
		    valueidx++;
		}
	    }
	    if ( !found )
	    {
		if ( params_[idx]->isRequired() )
		    continue;
	    }

	    if ( !params_[idx]->setValues(paramvalset) )
		return false;
	}
    }

    if ( statusupdater_ )
	statusupdater_(*this);

    if ( errmsg_.size() )
	return false;

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	 if ( !params_[idx]->isOK() )
	     return false;
    }

    return true;
}


const char* Desc::userRef() const		{ return userref_; }
void Desc::setUserRef( const char* str )	{ userref_ = str; }
int Desc::nrOutputs() const			{ return outputtypes_.size(); }
void Desc::selectOutput( int outp )		{ seloutput_ = outp; }
int Desc::selectedOutput() const		{ return seloutput_; }
int Desc::nrInputs() const			{ return inputs_.size(); }


void Desc::getInputs( TypeSet<DescID>& ids ) const
{
    for ( int idx=0; idx<nrInputs(); idx++ )
	ids += inputs_[idx] ? inputs_[idx]->id() : DescID::undef();
}


void Desc::getDependencies(TypeSet<Attrib::DescID>& deps) const
{
    for ( int idx=nrInputs()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] )
	    continue;

	if ( deps.isPresent(inputs_[idx]->id()) )
	    continue;

	deps += inputs_[idx]->id();
	inputs_[idx]->getDependencies(deps);
    }
}


bool Desc::getParentID( DescID did, DescID& pid, int& dididx ) const
{
    TypeSet<DescID> tmp;
    getDependencies( tmp );
    if ( !tmp.isPresent(did) )
	return false;

    for ( int idx=nrInputs()-1; idx>=0; idx-- )
    {
	if ( inputs_[idx] && inputs_[idx]->id()==did )
	{
	    pid = id();
	    dididx = idx;
	    return true;
	}
    }

    for ( int idx=nrInputs()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] )
	    continue;

	if ( inputs_[idx]->getParentID(did,pid,dididx) )
	    return true;
    }

    return true;
}


void Desc::getAncestorIDs( DescID cid, TypeSet<Attrib::DescID>& aids,
       TypeSet<int>& pindices ) const
{
    int cidx;
    DescID pid;
    if ( !getParentID(cid,pid,cidx) )
	return;

    aids += pid;
    pindices += cidx;

    if ( pid!=id() )
	getAncestorIDs( pid, aids, pindices );
}


Seis::DataType Desc::dataType( int target ) const
{
    if ( seloutput_==-1 || outputtypes_.isEmpty() )
	return Seis::UnknowData;

    int outidx = target==-1 ? seloutput_ : target;
    if ( outidx >= outputtypes_.size() ) outidx = 0;

    const int link = outputtypelinks_[outidx];
    if ( link == -1 )
	return outputtypes_[outidx];

    return link < inputs_.size() && inputs_[link] ? inputs_[link]->dataType()
						: Seis::UnknowData;
}


bool Desc::setInput( int inp, const Desc* nd )
{
    return setInput_( inp, const_cast<Desc*>( nd ) );
}


bool Desc::setInput_( int input, Desc* nd )
{
    if ( nd && (inputspecs_[input].forbiddenDts_.isPresent(nd->dataType()) ||
		inputspecs_[input].issteering_!=nd->isSteering()) )
	return false;

    if ( nd == this )
	return false;

    inputs_.replace( input, nd );

    return true;
}


ConstRefMan<Desc> Desc::getInput( int idx ) const
{
    return getNonConst(*this).getInput( idx );
}


RefMan<Desc> Desc::getInput( int idx )
{
    return inputs_.validIdx(idx) ? inputs_[idx] : nullptr;
}



#define mErrRet(msg) \
    const_cast<Desc*>(this)->errmsg_ = msg;\
    return Error;\


Desc::SatisfyLevel Desc::isSatisfied() const
{
    if ( seloutput_==-1 && !stringEndsWith( "|ALL", userref_.buf() ) )
	    // || seloutput_>nrOutputs()  )
	    //TODO NN descs return only one output. Needs solution!
    {
	const uiString msg = tr("Selected output is not correct");
	mErrRet(msg)
    }

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( !params_[idx]->isOK() )
	{
	    const uiString msg = tr("Parameter '%1' is not correct").
			arg(params_[idx]->getKey());
	    mErrRet(msg)
	}
    }

    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputspecs_[idx].enabled_ ) continue;
	if ( !inputs_[idx] )
	{
	    const uiString msg =
		    tr("'%1' is not correct").arg(inputspecs_[idx].getDesc());
	    mErrRet(msg)
	}
	else
	{
	    TypeSet<Attrib::DescID> deps( 1, inputs_[idx]->id() );
	    inputs_[idx]->getDependencies( deps );

	    if ( deps.isPresent( id() ) )
	    {
		const uiString msg = tr("'%1' is dependent on itself").
					    arg(inputspecs_[idx].getDesc());
		mErrRet(msg);
	    }
	}
    }

    return AllOk;
}


uiString Desc::errMsg() const
{
    return isStored() ? errmsg_ : tr("Error while saving the attribute.\n"
					"Values are not correct.");
}


bool Desc::isIdenticalTo( const Desc& desc, bool cmpoutput ) const
{
    if ( this==&desc )
	return true;

    if ( params_.size() != desc.params_.size()
      || inputs_.size() != desc.inputs_.size() )
	return false;

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( *params_[idx]!=*desc.params_[idx] )
	    return false;
    }

    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx]==desc.inputs_[idx] )
	    continue;

	if ( !inputs_[idx] && !desc.inputs_[idx] )
	    continue;

	if ( !desc.inputs_[idx] ||
	     !inputs_[idx]->isIdenticalTo(*desc.inputs_[idx], true) )
	    return false;
    }

    return cmpoutput ? seloutput_==desc.seloutput_ : true;
}


DescID Desc::inputId( int idx ) const
{
    const bool valididx = idx >= 0 && idx < inputs_.size();
    return valididx && inputs_[idx] ? inputs_[idx]->id() : DescID(-1,true);
}


void Desc::addParam( Param* param )
{
    params_ += param;
}


const Param* Desc::getParam( const char* key ) const
{ return const_cast<Desc*>(this)->getParam(key); }


Param* Desc::getParam( const char* key )
{
    return findParam(key);
}


const ValParam* Desc::getValParam( const char* key ) const
{
    mDynamicCastGet(const ValParam*,valpar,getParam(key))
    return valpar;
}


ValParam* Desc::getValParam( const char* key )
{
    mDynamicCastGet(ValParam*,valpar,getParam(key))
    return valpar;
}


void Desc::setParamEnabled( const char* key, bool yn )
{
    Param* param = findParam( key );
    if ( !param )
	return;

    param->setEnabled(yn);
}


bool Desc::isParamEnabled( const char* key ) const
{
    const Param* param = getParam( key );
    if ( !param )
	return false;

    return param->isEnabled();
}



void Desc::setParamRequired( const char* key, bool yn )
{
    Param* param = findParam( key );
    if ( !param )
	return;

    param->setRequired(yn);
}


bool Desc::isParamRequired( const char* key ) const
{
    const Param* param = getParam( key );
    if ( !param )
	return false;

    return param->isRequired();
}


void Desc::updateParams()
{
    if ( statusupdater_ ) statusupdater_(*this);

    for ( int idx=0; idx<nrInputs(); idx++ )
    {
	RefMan<Desc> dsc = getInput(idx);
	if ( dsc && dsc->isHidden() )
	    dsc->updateParams();
    }
}


void Desc::updateDefaultParams()
{
    if ( defaultsupdater_ ) defaultsupdater_(*this);
}


void Desc::addInput( const InputSpec& is )
{
    inputspecs_ += is;
    inputs_ += nullptr;
}


bool Desc::removeInput( int idx )
{
    inputspecs_.removeSingle(idx);
    inputs_.removeSingle(idx);
    return true;
}



InputSpec& Desc::inputSpec( int input ) { return inputspecs_[input]; }


const InputSpec& Desc::inputSpec( int input ) const
{ return const_cast<Desc*>(this)->inputSpec(input); }


void Desc::removeOutputs()
{
    outputtypes_.erase();
    outputtypelinks_.erase();
}


void Desc::setNrOutputs( Seis::DataType dt, int nroutp )
{
    removeOutputs();
    for ( int idx=0; idx<nroutp; idx++ )
	addOutputDataType( dt );
}


void Desc::addOutputDataType( Seis::DataType dt )
{ outputtypes_+=dt; outputtypelinks_+=-1; }


void Desc::addOutputDataTypeSameAs( int input )
{
    outputtypes_ += Seis::UnknowData;
    outputtypelinks_ += input;
}


void Desc::changeOutputDataType( int input, Seis::DataType ndt )
{
    if ( outputtypes_.size()<=input || input<0 ) return;
    outputtypes_[input] = ndt;
}


bool Desc::getAttribName( const char* inpdefstr, BufferString& res )
{
    BufferString defstr( inpdefstr );
    char* startptr = defstr.getCStr();
    mSkipBlanks( startptr );
    if ( !*startptr )
	return false;
    char* stopptr = startptr;
    mSkipNonBlanks( stopptr );
    if ( iswspace(*stopptr) ) *stopptr = '\0';
    res = startptr;
    return true;
}


bool Desc::getParamString( const char* ds, const char* ky, BufferString& res )
{
    if ( !ky || !*ky )
	return false;

    BufferStringSet keys, vals;
    getKeysVals( ds, keys, vals, ky );
    if ( keys.isEmpty() )
	return false;

    res = vals.get( 0 );
    return true;
}


Param* Desc::findParam( const char* key )
{
    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( params_[idx]->getKey()==key )
	    return params_[idx];
    }

    return 0;
}


bool Desc::isStored() const
{
    return attribName()==StorageProvider::attribName();
}


MultiID Desc::getStoredID( bool recursive ) const
{
    MultiID key;
    if ( isStored() )
    {
	const ValParam* keypar = getValParam( StorageProvider::keyStr() );
	if ( keypar )
	    key = keypar->getMultiID();
    }
    else if ( recursive )
    {
	for ( int idx=0; idx<nrInputs(); idx++ )
	{
	    ConstRefMan<Desc> tmpdesc = getInput( idx );
	    if ( tmpdesc )
		key = tmpdesc->getStoredID( true );

	    if ( !key.isUdf() )
		break;
	}
    }

    return key;
}


BufferString Desc::getStoredType( bool recursive ) const
{
    BufferString typestr;
    const MultiID key = getStoredID( recursive );
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( ioobj )
	ioobj->pars().get( sKey::Type(), typestr );

    return typestr;
}


bool Desc::isIdentifiedBy( const char* str ) const
{
    if ( userref_ == str )
	return true;
    else if ( !str || !*str )
	return false;

    if ( isStored() )
    {
	BufferString lk( str );
	if ( *str == '[' && *(str+lk.size()-1) == ']' )
	{
	    lk = str + 1;
	    lk.getCStr()[ lk.size()-1 ] = '\0';
	    if ( userref_ == lk )
		return true;
	}
	BufferString defstr; getDefStr(defstr);
	BufferString parstr;
	if ( !getParamString(defstr,params_[0]->getKey(),parstr) )
	    return false;

	const bool is2ddefstr = parstr == lk;
	if ( parstr == str || is2ddefstr )
	    return true;
    }

    return false;
}


void Desc::getKeysVals( const char* ds, BufferStringSet& keys,
			BufferStringSet& vals, const char* targetky )
{
    BufferString defstr( ds );
    defstr.trimBlanks();
    if ( defstr.isEmpty() )
	return;

    const StringView onlyneedkey( targetky );
    const bool havetarget = !onlyneedkey.isEmpty();
    char* still2scan = defstr.getCStr();
    while ( *still2scan )
    {
	char* ptrval = firstOcc( still2scan, '=' );
	if ( !ptrval )
	    break;

	char* ptrkey = ptrval;
	*ptrval++ = '\0';
	while ( ptrkey != still2scan && !iswspace(*ptrkey) )
	    ptrkey--;
	mSkipBlanks(ptrkey);

	still2scan = ptrval;
	if ( *ptrval == '"' )
	    { ptrval++; still2scan = firstOcc( ptrval, '"' ); }
	else
	    { mSkipNonBlanks(still2scan); }
	if ( still2scan && *still2scan )
	    *still2scan++ = '\0';

	if ( *ptrkey )
	{
	    const bool doadd = !havetarget || onlyneedkey == ptrkey;
	    if ( doadd )
	    {
		keys.add( ptrkey ); vals.add( ptrval );
		if ( havetarget )
		    break;
	    }
	}
    }
}


void Desc::changeStoredID( const char* newid )
{
    if ( !isStored() ) return;

    ValParam* keypar = getValParam( StorageProvider::keyStr() );
    keypar->setValue( newid );
}


RefMan<Desc> Desc::getStoredInput() const
{
    RefMan<Desc> desc;
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] )
	    continue;

	if ( inputs_[idx]->isStored() )
	    return const_cast<Desc*>( inputs_[idx] );
	else
	    desc = inputs_[idx]->getStoredInput();
    }

    return desc;
}


DescID Desc::getMultiOutputInputID() const
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] )
	    continue;

	if ( inputs_[idx]->isStored() )
	{
	    if ( inputs_[idx]->selectedOutput()==-1 )
		return inputId( idx );
	}
	else
	{
	    DescID multoutinpdid = inputs_[idx]->getMultiOutputInputID();
	    if ( multoutinpdid != DescID::undef() )
		return multoutinpdid;
	}
    }

    return DescID::undef();
}


bool Desc::isStoredInMem() const
{
    if ( !isStored() )
	return false;

    const MultiID dbky = getStoredID();
    return dbky.isInMemoryDPID();
}


RefMan<Desc> Desc::cloneDescAndPropagateInput( const DescID& newinputid,
					       BufferString sufix )
{
    if ( seloutput_ == -1 )
	return descset_->getDesc( newinputid );

    RefMan<Desc> myclone = new Desc(*this);
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] )
	    continue;

	RefMan<Desc> newinpdesc =
		inputs_[idx]->cloneDescAndPropagateInput( newinputid, sufix );
	if ( !newinpdesc )
	    return nullptr;

	myclone->setInput( idx, newinpdesc.ptr() );
    }

    myclone->setHidden( true );
    BufferString newuserref( userref_, "_", sufix );
    myclone->setUserRef( newuserref.buf() );
    descset_->addDesc( myclone.ptr() );
    return myclone;
}

} // namespace Attrib
