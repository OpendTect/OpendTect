/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2003
-*/



#include "attribdesc.h"

#include "attribparam.h"
#include "attribdescset.h"
#include "attribstorprovider.h"
#include "ioobj.h"
#include "keystrs.h"
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


bool InputSpec::operator==( const InputSpec& oth ) const
{
    if ( desc_!=oth.desc_
      || enabled_!=oth.enabled_
      || issteering_!=oth.issteering_ )
	return false;

    for ( int idx=0; idx<forbiddenDts_.size(); idx++ )
    {
	if ( !oth.forbiddenDts_.isPresent(forbiddenDts_[idx]) )
	    return false;
    }

    for ( int idx=0; idx<oth.forbiddenDts_.size(); idx++ )
    {
	if ( !forbiddenDts_.isPresent(oth.forbiddenDts_[idx]) )
	    return false;
    }

    return true;
}



Desc::Desc( const char* attribname, DescUpdater updater,
	    DescUpdater defupdater )
    : descset_(0)
    , attribname_(attribname)
    , statusupdater_(updater)
    , defaultsupdater_(defupdater)
    , seloutput_(0)
    , is2d_(false)
    , isps_(false)
    , issingtrc_(false)
    , issteering_(false)
    , ishidden_(false)
    , usestrcpos_(false)
    , needprovinit_(false)
    , userRefChanged(this)
{
    inputs_.setNullAllowed(true);
    attribname_.replace( ' ', '_' );
}


Desc::Desc( const Desc& oth )
    : descset_(oth.descset_)
    , attribname_(oth.attribname_)
    , userref_(oth.userref_)
    , statusupdater_(oth.statusupdater_)
    , defaultsupdater_(oth.defaultsupdater_)
    , seloutput_(oth.seloutput_)
    , isps_(oth.isps_)
    , issingtrc_(oth.isps_)
    , issteering_(oth.isps_)
    , ishidden_(oth.ishidden_)
    , usestrcpos_(oth.usestrcpos_)
    , needprovinit_(oth.needprovinit_)
    , userRefChanged(this)
{
    inputs_.setNullAllowed(true);

    for ( int idx=0; idx<oth.params_.size(); idx++ )
	addParam( oth.params_[idx]->clone() );

    for ( int idx=0; idx<oth.inputs_.size(); idx++ )
    {
	addInput( oth.inputSpec(idx) );
	if ( oth.inputs_[idx] )
	    setInput( idx, oth.inputs_[idx] );
    }

    for ( int idx=0; idx<oth.nrOutputs(); idx++ )
	addOutputDataType( oth.outputtypes_[idx] );
}


Desc::~Desc()
{
    detachAllNotifiers();
    deepErase( params_ );
    deepUnRef( inputs_ );
}


void Desc::setDescSet( DescSet* nds )
{
    descset_ = nds;
    if ( nds )
	is2d_ = nds->is2D();
}


DescID Desc::id() const
{
    return descset_ ? descset_->getID( *this ) : DescID();
}


bool Desc::getDefStr( BufferString& res ) const
{
    res.set( attribName() );

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( !params_[idx]->isEnabled() )
	    continue;

	BufferString curdef;
	params_[idx]->fillDefStr( curdef );
	if ( !curdef.isEmpty() )
	    res.add( ' ' ).add( curdef );
    }

    if ( seloutput_!=-1 || stringEndsWith( "|ALL", userref_.buf() ) )
	res.add( " output=" ).add( seloutput_ );

    return true;
}


bool Desc::parseDefStr( const char* defstr )
{
    BufferString defstrnm;
    if ( !getAttribName(defstr,defstrnm) || defstrnm!=attribname_ )
	return false;

    BufferString outputstr;
    bool res = getParamString( defstr, "output", outputstr );
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

    if ( !errmsg_.isEmpty() )
	return false;

    for ( int idx=0; idx<params_.size(); idx++ )
    {
         if ( !params_[idx]->isOK() )
	     return false;
    }

    return true;
}


void Desc::setUserRef( const char* str )
{
    if ( userref_ != str )
    {
	userref_ = str;
	userRefChanged.trigger();
    }
}


void Desc::getInputs( TypeSet<DescID>& ids ) const
{
    for ( int idx=0; idx<nrInputs(); idx++ )
	ids += inputs_[idx] ? inputs_[idx]->id() : DescID();
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


DataType Desc::dataType( int target ) const
{
    if ( seloutput_==-1 || outputtypes_.isEmpty() )
	return Seis::UnknownData;

    int outidx = target==-1 ? seloutput_ : target;
    if ( outidx >= outputtypes_.size() )
	outidx = 0;

    const int inpidx = outputtypeinpidxs_[outidx];
    if ( inpidx < 0 || !inputs_.validIdx(inpidx) || !inputs_[inpidx] )
	return outputtypes_[outidx];

    return inputs_[inpidx]->dataType();
}


bool Desc::setInput( int inp, const Desc* nd )
{
    return setInput_( inp, const_cast<Desc*>( nd ) );
}


bool Desc::setInput_( int input, Desc* nd )
{
    if ( nd && (inputspecs_[input].forbiddenDts_.isPresent(nd->dataType()) ||
		( nd->dataType() != Seis::Dip &&
		  inputspecs_[input].issteering_!=nd->isSteering()) ) )
	return false;

    if ( inputs_[input] ) inputs_[input]->unRef();
    inputs_.replace( input, nd );
    if ( inputs_[input] ) inputs_[input]->ref();

    return true;
}


const Desc* Desc::getInput( int inpidx ) const
{
    return inputs_.validIdx(inpidx) ? inputs_[inpidx] : 0;
}


Desc* Desc::getInput( int inpidx )
{
    return inputs_.validIdx(inpidx) ? inputs_[inpidx] : 0;
}


#define mErrRet(msg) { errmsg_ = msg; return GenError; }


Desc::SatisfyLevel Desc::satisfyLevel() const
{
    if ( seloutput_==-1 && !stringEndsWith( "|ALL", userref_.buf() ) )
	mErrRet( tr("Selected output is not correct") )

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( !params_[idx]->isOK() )
	{
	    const BufferString ky = params_[idx]->getKey();
	    if ( ky == "id" )
	    {
		errmsg_ = tr("A stored input is incorrect");
		return StorNotFound;
	    }
	    else
	    {
		mErrRet( tr("Parameter '%1' for '%2' is not correct")
			 .arg( ky ).arg( userref_ ) )
	    }
	}
    }

    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputspecs_[idx].enabled_ )
	    continue;

	if ( !inputs_[idx] )
	    mErrRet( tr("Input for '%1' (%2) is not provided")
		     .arg( userref_ ).arg( inputspecs_[idx].getDesc() ) )
	else
	{
	    TypeSet<Attrib::DescID> deps( inputs_[idx]->id() );
	    inputs_[idx]->getDependencies( deps );

	    if ( deps.isPresent( id() ) )
		mErrRet( tr("'%1' is dependent on itself").arg(userref_) );
	}
    }

    return AllOk;
}


const uiString Desc::errMsg() const
{
    if ( errmsg_.isEmpty() )
    {
	if ( !isStored() )
	    errmsg_ = tr("%1 attribute '%2' has incorrect parameters.")
				.arg( attribname_ ).arg( userref_ );
	else
	{
	    pErrMsg( "Stored attrib shld already have the error message set" );
	    errmsg_ = tr("Error accessing '%1'").arg( userref_ );
	}
    }

    return errmsg_;
}


bool Desc::isIdenticalTo( const Desc& oth, bool cmpoutput ) const
{
    if ( this == &oth )
	return true;

    if ( params_.size() != oth.params_.size()
      || inputs_.size() != oth.inputs_.size() )
	return false;

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( *params_[idx] != *oth.params_[idx] )
	    return false;
    }

    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	const Desc* myinp = inputs_[idx];
	const Desc* othinp = oth.inputs_[idx];
	if ( myinp == othinp )
	    continue;
	else if ( !myinp || !othinp || !myinp->isIdenticalTo(*othinp,true) )
	    return false;
    }

    return !cmpoutput || seloutput_ == oth.seloutput_;
}


DescID Desc::inputId( int idx ) const
{
    return inputs_.validIdx(idx) && inputs_[idx]
	 ? inputs_[idx]->id() : DescID();
}


const ValParam* Desc::getValParam( const char* key ) const
{
    mDynamicCastGet( const ValParam*, valpar, findParam(key) )
    return valpar;
}


ValParam* Desc::getValParam( const char* key )
{
    mDynamicCastGet( ValParam*, valpar, findParam(key) )
    return valpar;
}


void Desc::setParamEnabled( const char* key, bool yn )
{
    Param* param = findParam( key );
    if ( param )
	param->setEnabled(yn);
}


bool Desc::isParamEnabled( const char* key ) const
{
    const Param* param = findParam( key );
    return param && param->isEnabled();
}



void Desc::setParamRequired( const char* key, bool yn )
{
    Param* param = findParam( key );
    if ( param )
	param->setRequired(yn);
}


bool Desc::isParamRequired( const char* key ) const
{
    const Param* param = getParam( key );
    return param && param->isRequired();
}


void Desc::updateParams()
{
    if ( statusupdater_ )
	statusupdater_(*this);

    for ( int idx=0; idx<nrInputs(); idx++ )
    {
	Desc* dsc = getInput(idx);
	if ( dsc && dsc->isHidden() )
	    dsc->updateParams();
    }
}


void Desc::updateDefaultParams()
{
    if ( defaultsupdater_ )
	defaultsupdater_(*this);
}


void Desc::addInput( const InputSpec& is )
{
    inputspecs_ += is;
    inputs_ += 0;
}


bool Desc::removeInput( int idx )
{
    inputspecs_.removeSingle(idx);
    inputs_.removeSingle(idx);
    return true;
}


void Desc::removeOutputs()
{
    outputtypes_.erase();
    outputtypeinpidxs_.erase();
}


void Desc::setNrOutputs( DataType dt, int nroutp )
{
    removeOutputs();
    for ( int idx=0; idx<nroutp; idx++ )
	addOutputDataType( dt );
}


void Desc::addOutputDataType( DataType dt )
{
    outputtypes_ += dt;
    outputtypeinpidxs_ += -1;
}


void Desc::addOutputDataTypeSameAs( int inpidx )
{
    outputtypes_ += Seis::UnknownData;
    outputtypeinpidxs_ += inpidx;
}


void Desc::changeOutputDataType( int inpidx, DataType ndt )
{
    if ( outputtypes_.validIdx(inpidx) )
	outputtypes_[inpidx] = ndt;
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
    if ( isspace(*stopptr) )
	*stopptr = '\0';
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


Param* Desc::findParam( const char* key ) const
{
    for ( int idx=0; idx<params_.size(); idx++ )
    {
	Param* par = const_cast<Param*>( params_[idx] );
	if ( FixedString(par->getKey()) == key )
	    return par;
    }
    return 0;
}


bool Desc::isStored() const
{
    return attribName() == StorageProvider::attribName();
}


DBKey Desc::getStoredID( bool recursive ) const
{
    DBKey dbky;

    if ( isStored() )
    {
	const ValParam* keypar = getValParam( StorageProvider::keyStr() );
	if ( keypar )
	    dbky.fromString( keypar->getStringValue() );
    }
    else if ( recursive )
    {
	for ( int idx=0; idx<nrInputs(); idx++ )
	{
	    const Desc* desc = getInput( idx );
	    dbky = desc ? desc->getStoredID(true) : DBKey();
	    if ( dbky.isValid() )
		break;
	}
    }

    return dbky;
}


BufferString Desc::getStoredType( bool recursive ) const
{
    BufferString typestr;
    const DBKey key( getStoredID(recursive) );
    PtrMan<IOObj> ioobj = key.getIOObj();
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

    const FixedString onlyneedkey( targetky );
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


void Desc::changeStoredID( const DBKey& newid )
{
    if ( !isStored() )
	return;

    ValParam* keypar = getValParam( StorageProvider::keyStr() );
    keypar->setValue( newid.toString() );
}


Desc* Desc::getStoredInput() const
{
    Desc* desc = 0;
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] ) continue;

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
	if ( !inputs_[idx] ) continue;

	if ( inputs_[idx]->isStored() )
	{
	    if ( inputs_[idx]->selectedOutput()==-1 )
		return inputId( idx );
	}
	else
	{
	    DescID multoutinpdid = inputs_[idx]->getMultiOutputInputID();
	    if ( multoutinpdid.isValid() )
		return multoutinpdid;
	}
    }

    return DescID();
}


bool Desc::isStoredInMem() const
{
    if ( !isStored() ) return false;

    const BufferString bs( getValParam(
			Attrib::StorageProvider::keyStr())->getStringValue(0) );
    return !bs.isEmpty() && bs[0] == '#';
}


Desc* Desc::cloneDescAndPropagateInput( const DescID& newinputid,
					BufferString sufix ) const
{
    if ( seloutput_ == -1 )
	return descset_->getDesc( newinputid );

    Desc* myclone = new Desc( *this );

    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] ) continue;

	Desc* newinpdesc =
		inputs_[idx]->cloneDescAndPropagateInput( newinputid, sufix );
	if ( !newinpdesc )
	    return 0;

	myclone->setInput( idx, newinpdesc );
    }

    myclone->ref();
    myclone->setIsHidden( true );
    BufferString newuserref( userref_, "_", sufix );
    myclone->setUserRef( newuserref.buf() );
    descset_->addDesc( myclone );
    return myclone;
}


void getIntFromDescStr( Desc& desc, int& var, const char* str )
{
    var = desc.getValParam(str)->getIntValue( 0 );
    if ( mIsUdf(var) )
        var = desc.getValParam(str)->getDefaultIntValue( 0 );
}


}; // namespace Attrib
