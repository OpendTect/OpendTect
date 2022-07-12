/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert/Arnaud
 * DATE     : April 2018
-*/

#include "odjson.h"

#include "arraynd.h"
#include "dbkey.h"
#include "gason.h"
#include "od_iostream.h"
#include "posgeomid.h"
#include "separstr.h"
#include "stringbuilder.h"
#include "uistrings.h"

#include <string.h>

#ifdef __debug__
# ifdef __win__
#  include "file.h"
# endif
#endif


namespace OD
{

namespace JSON
{

class Value
{
public:

    union Contents
    {
	Contents() { val_ = 0; }
	NumberType val_;
	bool bool_;
	char* str_;
	ValueSet* vset_;
    };
    Contents		cont_;
    int			type_ = Number;

    virtual bool	isKeyed() const	{ return false; }

    ValueSet*		vSet()		{ return cont_.vset_; }
    const ValueSet*	vSet() const	{ return cont_.vset_; }
    bool&		boolVal()	{ return cont_.bool_; }
    bool		boolVal() const	{ return cont_.bool_; }
    NumberType&		val()		{ return cont_.val_; }
    NumberType		val() const	{ return cont_.val_; }
    char*		str()		{ return cont_.str_; }
    const char*		str() const	{ return cont_.str_; }


Value() : type_( int(Number) )		{}
virtual Value* getEmptyClone() const	{ return new Value; }

Value* clone( ValueSet* parent ) const
{
    Value* newval = getEmptyClone();
    if ( isValSet() )
    {
	ValueSet* newset = vSet()->clone();
	newset->setParent( parent );
	newval->setValue( newset );
    }
    else
    {
	switch ( DataType(type_) )
	{
	    case Boolean:	newval->setValue( boolVal() );	break;
	    case Number:	newval->setValue( val() );	break;
	    case String:	newval->setValue( str() );	break;
	}
    }
    return newval;
}

#define mDefSimpleConstr( typ, cast ) \
    Value( typ v ) { setValue( (cast)(v) ); }

mDefSimpleConstr( bool, bool )
mDefSimpleConstr( od_int16, NumberType )
mDefSimpleConstr( od_uint16,NumberType )
mDefSimpleConstr( od_int32, NumberType )
mDefSimpleConstr( od_uint32, NumberType )
mDefSimpleConstr( od_int64, NumberType )
mDefSimpleConstr( float, NumberType )
mDefSimpleConstr( double, NumberType )
mDefSimpleConstr( const char*, const char* )
mDefSimpleConstr( ValueSet*, ValueSet* )

virtual ~Value()
{
    cleanUp();
}

void cleanUp()
{
    if ( isValSet() )
	delete vSet();
    else if ( type_ == (int)String )
	delete [] str();
    cont_.val_ = 0;
}

void setValue( bool v )
{
    cleanUp();
    type_ = (int)Boolean;
    cont_.bool_ = v;
}

void setValue( NumberType v )
{
    cleanUp();
    type_ = (int)Number;
    cont_.val_ = v;
}

void setValue( const char* cstr )
{
    cleanUp();
    type_ = (int)String;

    if ( !cstr )
	cstr = "";
    const int len = StringView(cstr).size();
    char* contstr = new char[len + 1];
#ifdef __win__
    strcpy_s( contstr, len+1, cstr );
#else
    strcpy( contstr, cstr );
#endif
    cont_.str_ = contstr;
}

void setValue( ValueSet* vset )
{
    cleanUp();
    type_ = (int)String + 1;
    cont_.vset_ = vset;
}

bool isValSet() const
{
    return type_ > String;
}

};


//class KeyedValue
class KeyedValue : public Value
{
public:

    BufferString	key_;

    bool	isKeyed() const override	{ return true; }
    Value*	getEmptyClone() const override  { return new KeyedValue(key_); }

KeyedValue( const char* ky ) : key_(ky)	{}

#define mDefSimpleKeyedConstr(ctyp) \
    KeyedValue( const char* ky, ctyp i ) \
	: Value(i), key_(ky) {}

mDefSimpleKeyedConstr( bool )
mDefSimpleKeyedConstr( od_int16 )
mDefSimpleKeyedConstr( od_uint16 )
mDefSimpleKeyedConstr( od_int32 )
mDefSimpleKeyedConstr( od_uint32 )
mDefSimpleKeyedConstr( od_int64 )
mDefSimpleKeyedConstr( float )
mDefSimpleKeyedConstr( double )
mDefSimpleKeyedConstr( const char* )
mDefSimpleKeyedConstr( ValueSet* )

};

} // namespace JSON

} // namespace OD


//--------- ValArr

OD::JSON::ValArr::ValArr( DataType typ )
    : type_(typ)
{
    switch ( type_ )
    {
	case Boolean:	set_ = new BSet;	break;
	case Number:	set_ = new NSet;	break;
	default:	{ pErrMsg("Unknown type"); type_ = String; }
	case String:	set_ = new SSet;	break;
    }
}


OD::JSON::ValArr::ValArr( const ValArr& oth )
    : ValArr(oth.type_)
{
    switch ( type_ )
    {
	case Boolean:	bools() = oth.bools();		break;
	case Number:	vals() = oth.vals();		break;
	case String:	strings() = oth.strings();	break;
    }
}


BufferString OD::JSON::ValArr::dumpJSon() const
{
    StringBuilder sb;
    dumpJSon( sb );
    return sb.result();
}


void OD::JSON::ValArr::dumpJSon( BufferString& bs ) const
{
    StringBuilder sb;
    dumpJSon( sb );
    bs = sb.result();
}


void OD::JSON::ValArr::dumpJSon( StringBuilder& sb ) const
{
    const int sz = size_type(set_->nrItems());
    sb.add( '[' );
    for ( int idx=0; idx<sz; idx++ )
    {
	switch ( type_ )
	{
	    case Boolean:
	    {
		const bool val = bools()[idx];
		sb.add( val ? "true" : "false" );
	    } break;
	    case Number:
	    {
		const NumberType val = vals()[idx];
		sb.add( val );
	    } break;
	    case String:
	    {
		const BufferString toadd( "\"", strings().get(idx), "\"" );
		sb.add( toadd );
	    } break;
	}
	if ( idx != sz-1 )
	    sb.add( "," );
    }

    sb.add( ']' );
}


void OD::JSON::ValArr::setFilePath( const FilePath& fp, idx_type idx )
{
    if ( !strings().validIdx(idx) )
    {
	pErrMsg("Cannot set FilePath.Not a valid index");
	return;
    }

    BufferString& bs = strings().get( idx );
    bs.set( fp.fullPath() );
    if ( __iswin__ )
	bs.replace( "\\", "/" );

}


FilePath OD::JSON::ValArr::getFilePath( idx_type idx ) const
{
    BufferString ret;
    if ( !strings().validIdx( idx ) )
	return FilePath( ret );

    if ( type_ == String )
    {
	ret.set( strings().get(idx) );
#ifdef __win__
	ret.replace( "/", "\\" );
#endif
    }

    return FilePath( ret );
}


//--------- ValueSet


OD::JSON::ValueSet::ValueSet( const ValueSet& oth )
    : parent_(oth.parent_)
{
    for ( const auto* val : oth.values_ )
	values_ += val->clone( this );
}


void OD::JSON::ValueSet::setEmpty()
{
    deepErase( values_ );
}


OD::JSON::ValueSet::ValueType OD::JSON::ValueSet::valueType(
				    idx_type idx ) const
{
    ValueType ret = Data;
    if ( !values_.validIdx(idx) )
	{ pErrMsg("Idx out of range"); return ret; }

    const Value& val = *values_[idx];
    if ( val.isValSet() )
	ret = val.vSet()->isArray() ? SubArray : SubObject;

    return ret;
}


const BufferString& OD::JSON::ValueSet::key( idx_type idx ) const
{
    const Value* val = values_[idx];
    if ( !val->isKeyed() )
	return BufferString::empty();

    const KeyedValue& kydval = *static_cast<const KeyedValue*>( val );
    return kydval.key_;
}


OD::JSON::ValueSet* OD::JSON::ValueSet::top()
{
    return parent_ ? parent_->top() : this;
}


const OD::JSON::ValueSet* OD::JSON::ValueSet::top() const
{
    return parent_ ? parent_->top() : this;
}


OD::JSON::ValueSet* OD::JSON::ValueSet::gtChildByIdx( idx_type idx ) const
{
    if ( !values_.validIdx(idx) )
	return nullptr;

    const Value* val = values_[idx];
    if ( !val->isValSet() )
	{ pErrMsg("Value at idx is not ValSet"); }

    return const_cast<ValueSet*>( val->vSet() );
}


OD::JSON::Array* OD::JSON::ValueSet::gtArrayByIdx( idx_type idx ) const
{
    ValueSet* vset = gtChildByIdx( idx );
    if ( !vset || !vset->isArray() )
	return nullptr;

    return static_cast<Array*>( vset );
}


OD::JSON::Object* OD::JSON::ValueSet::gtObjectByIdx( idx_type idx ) const
{
    ValueSet* vset = gtChildByIdx( idx );
    if ( !vset || vset->isArray() )
	return nullptr;

    return static_cast<Object*>( vset );
}


static const char* gtvalnotplaindatastr = "ValueSet at idx is not plain data";

bool OD::JSON::ValueSet::getBoolValue( idx_type idx ) const
{
    bool ret = false;
    if ( !values_.validIdx(idx) )
	return ret;

    const Value* val = values_[idx];
    if ( val->isValSet() )
    { pErrMsg(gtvalnotplaindatastr); return ret; }

    switch ( DataType(val->type_) )
    {
    case Boolean:	ret = val->boolVal();  break;
    default:		ret = false;
    }
    return ret;
}


BufferString OD::JSON::ValueSet::getStringValue( idx_type idx ) const
{
    BufferString ret;
    if ( !values_.validIdx(idx) )
	return ret;

    const Value* val = values_[idx];
    if ( val->isValSet() )
	{ pErrMsg(gtvalnotplaindatastr); return ret; }

    switch ( DataType(val->type_) )
    {
	case Boolean:	ret.set( val->boolVal() ? "true" : "false" );  break;
	case Number:	ret.set( val->val() );  break;
	default:	{ pErrMsg("Huh"); }
	case String:
    {
        ret.set( val->str() );
#ifdef __debug__
# ifdef __win__
	const FilePath fp( ret );
	if ( !fp.isURI() && fp.exists() && fp.isAbsolute() )
	{
	    pErrMsg( "Should not use getStringValue for a filepath" );
	    DBG::forceCrash( false );
	}
# endif
#endif
        break;
    }
    }
    return ret;
}


FilePath OD::JSON::ValueSet::getFilePath( idx_type idx ) const
{
    BufferString ret;
    if ( !values_.validIdx( idx ) )
        return FilePath(ret);

    const Value* val = values_[idx];
    if ( val->isValSet() )
    {
        pErrMsg( gtvalnotplaindatastr ); return FilePath(ret);
    }
    if ( DataType(val->type_) == String )
    {
        ret.set( val->str() );
#ifdef __win__
        ret.replace( "/", "\\" );
#endif
    }

    return FilePath( ret );
}


od_int64 OD::JSON::ValueSet::getIntValue( idx_type idx ) const
{
    od_int64 ret = mUdf(od_int64);
    if ( !values_.validIdx(idx) )
	return ret;

    const Value* val = values_[idx];
    if ( val->isValSet() )
	{ pErrMsg(gtvalnotplaindatastr); return ret; }

    switch ( DataType(val->type_) )
    {
	case Boolean:	ret = val->boolVal() ? 0 : 1;  break;
	case Number:	ret = mNINT64( val->val() );  break;
	default:	{ pErrMsg("Huh"); }
	case String:	ret = toInt64( val->str() );  break;
    }

    return ret;
}


double OD::JSON::ValueSet::getDoubleValue( idx_type idx ) const
{
    double ret = mUdf(double);
    if ( !values_.validIdx(idx) )
	return ret;

    const Value* val = values_[idx];
    if ( val->isValSet() )
	{ pErrMsg(gtvalnotplaindatastr); return ret; }

    switch ( DataType(val->type_) )
    {
	case Boolean:	ret = val->boolVal() ? 0 : 1;  break;
	case Number:	ret = val->val();  break;
	default:	{ pErrMsg("Huh"); }
	case String:	ret = toDouble( val->str() );  break;
    }

    return ret;
}


#ifdef __msvc__
#pragma warning(push)
#pragma warning( disable : 4702)  // ln 376 was unreachable on VS13
#endif

static Gason::JsonTag getNextTag( const Gason::JsonValue gasonval )
{
    for ( auto gasonnode : gasonval )
	return gasonnode->value.getTag();

    return Gason::JSON_NULL;
}

#ifdef __msvc__
#pragma warning(pop)
#endif


static OD::JSON::ValueSet* getSubVS( OD::JSON::ValueSet* parent,
		Gason::JsonTag tag, Gason::JsonTag nexttag )
{
    if ( tag == Gason::JSON_OBJECT )
	return new OD::JSON::Object( parent );
    else if ( tag != Gason::JSON_ARRAY )
	return nullptr;

    const bool nextisarr = nexttag == Gason::JSON_ARRAY;
    const bool nextisobj = nexttag == Gason::JSON_OBJECT;
    if ( nextisarr || nextisobj )
	return new OD::JSON::Array( nextisobj, parent );

    OD::JSON::DataType dt = OD::JSON::Boolean;
    if ( nexttag == Gason::JSON_NUMBER || nexttag == Gason::JSON_NULL )
	dt = OD::JSON::Number;
    else if ( nexttag == Gason::JSON_STRING )
	dt = OD::JSON::String;

    return new OD::JSON::Array( dt, parent );
}


void OD::JSON::ValueSet::use( const GasonNode& gasonnode )
{
    const Gason::JsonValue& gasonval = gasonnode.value;
    const Gason::JsonTag tag = gasonval.getTag();
    bool isobj = !isArray();
    const char* ky = isobj ? gasonnode.key : 0;

    switch ( tag )
    {
	case Gason::JSON_NUMBER:
	{
	    const double val = gasonval.toNumber();
	    if ( isobj )
		values_ += new KeyedValue( ky, val );
	    else
		asArray().valArr().vals() += val;
	} break;

	case Gason::JSON_STRING:
	{
	    const char* val = gasonval.toString();
	    if ( isobj )
		values_ += new KeyedValue( gasonnode.key, val );
	    else
		asArray().valArr().strings().add( val );
	} break;

	case Gason::JSON_TRUE:
	case Gason::JSON_FALSE:
	{
	    const bool val = tag == Gason::JSON_TRUE;
	    if ( isobj )
		values_ += new KeyedValue( gasonnode.key, val );
	    else
		asArray().valArr().bools() += val;
	} break;

	case Gason::JSON_ARRAY:
	{
	    const Gason::JsonTag nexttag = getNextTag( gasonval );
	    Array* arr = (Array*)getSubVS( this, tag, nexttag );
	    if ( arr )
	    {
		if ( isobj )
		    values_ += new KeyedValue( ky, arr );
		else
		    values_ += new Value( arr );
	    }

	    for ( auto subgasonnode : gasonval )
		if ( arr )
		    arr->use( *subgasonnode );
	} break;

	case Gason::JSON_OBJECT:
	{
	    Object* obj = new Object( this );
	    if ( isobj )
		values_ += new KeyedValue( gasonnode.key, obj );
	    else
		values_ += new Value( obj );

	    for ( auto subgasonnode : gasonval )
		obj->use( *subgasonnode );
	} break;

	case Gason::JSON_NULL:
	{
	} break;
    }
}


OD::JSON::ValueSet* OD::JSON::ValueSet::gtByParse( char* buf, int bufsz,
			    uiRetVal& uirv, ValueSet* intovset )
{
    uirv.setEmpty();
    if ( !buf || bufsz < 1 )
	{ uirv.set( mINTERNAL("No data to parse (JSON)") ); return intovset; }

    Gason::JsonAllocator allocator;
    Gason::JsonValue rootgasonval; char* endptr;
    int status = Gason::jsonParse( buf, &endptr, &rootgasonval, allocator );
    if ( status != Gason::JSON_OK )
    {
	BufferString gasonerr;
	if ( status == Gason::JSON_BREAKING_BAD )
	    gasonerr.set( "incomplete input" );
	else
	    gasonerr.set( Gason::jsonStrError( status ) );

	int res = endptr-buf+1;
	uirv.set( tr("JSON parse error: '%1' at position %2")
			.arg(gasonerr).arg(res) );
	return intovset;
    }

    const Gason::JsonTag roottag = rootgasonval.getTag();
    const bool buf_has_array = roottag == Gason::JSON_ARRAY;
    if ( !buf_has_array && roottag != Gason::JSON_OBJECT )
    {
	uirv.set( tr("Make sure JSON content starts with '{' or '['") );
	return intovset;
    }

    ValueSet* usevset = intovset;
    bool intoisarray = buf_has_array;
    if ( intovset )
	intoisarray = intovset->isArray();
    else
    {
	usevset = getSubVS( 0, rootgasonval.getTag(), getNextTag(rootgasonval));
	if ( !usevset )
	{
	    uirv.set( tr("No meaningful JSON content found") );
	    return intovset;
	}
    }

    if ( intoisarray != buf_has_array )
    {
	if ( intoisarray )
	    intovset->asArray().add( &usevset->asObject() );
	else
	    intovset->asObject().set( "JSON", &usevset->asArray() );
    }

    for ( auto gasonnode : rootgasonval )
	usevset->use( *gasonnode );

    return usevset;
}


OD::JSON::ValueSet* OD::JSON::ValueSet::getFromJSon( char* buf, int bufsz,
						    uiRetVal& uirv )
{
    return gtByParse( buf, bufsz, uirv, 0 );
}


uiRetVal OD::JSON::ValueSet::parseJSon( char* buf, int bufsz )
{
    uiRetVal uirv;
    gtByParse( buf, bufsz, uirv, this );
    return uirv;
}


BufferString OD::JSON::ValueSet::dumpJSon() const
{
    StringBuilder sb;
    dumpJSon( sb );
    return sb.result();
}


void OD::JSON::ValueSet::dumpJSon( BufferString& bs ) const
{
    StringBuilder sb;
    dumpJSon( sb );
    bs = sb.result();
}


void OD::JSON::ValueSet::dumpJSon( StringBuilder& sb ) const
{
    const bool isarr = isArray();
    sb.add( isarr ? '[' : '{' );

    for ( int idx=0; idx<values_.size(); idx++ )
    {
	const Value& val = *values_[idx];
	if ( val.isKeyed() )
	{
	    const KeyedValue& keyedval = static_cast<const KeyedValue&>( val );
	    sb.add( "\"" ).add( keyedval.key_ ).add( "\":" );
	}

	if ( val.isValSet() )
	{
	    const ValueSet& vset = *val.vSet();
	    if ( !vset.isArray() || vset.asArray().valType() != Data )
		vset.dumpJSon( sb );
	    else
		vset.asArray().valArr().dumpJSon( sb );
	}
	else
	{
	    switch ( DataType(val.type_) )
	    {
		case Boolean:
		    sb.add( val.boolVal() ? "true" : "false" );
		break;
		case Number:
		    sb.add( val.val() );
		break;
		case String:
		    sb.add( "\"" ).add( val.str() ).add( "\"" );
		break;
	    }
	}
	if ( &val != values_.last() )
	    sb.add( "," );
    }

    sb.add( isarr ? "]" : "}" );
}


uiRetVal OD::JSON::ValueSet::read( od_istream& strm )
{
    BufferString buf;
    if ( strm.getAll(buf) )
	return parseJSon( buf.getCStr(), buf.size() );

    uiRetVal uirv( uiStrings::phrCannotRead(toUiString(strm.fileName())) );
    strm.addErrMsgTo( uirv );
    return uirv;
}


OD::JSON::ValueSet* OD::JSON::ValueSet::read( od_istream& strm, uiRetVal& uirv )
{
    BufferString buf;
    if ( strm.getAll(buf) )
	return getFromJSon( buf.getCStr(), buf.size(), uirv );

    uirv.set( uiStrings::phrCannotRead( toUiString(strm.fileName()) ) );
    strm.addErrMsgTo( uirv );
    return nullptr;
}


uiRetVal OD::JSON::ValueSet::write( od_ostream& strm )
{
    StringBuilder sb;
    dumpJSon( sb );
    uiRetVal uirv;
    if ( !strm.add(sb.result()).isOK() )
    {
	uirv.set( uiStrings::phrCannotWrite( toUiString(strm.fileName()) ) );
	strm.addErrMsgTo( uirv );
    }

    return uirv;
}


//--------- Array

OD::JSON::Array::Array( bool objs, ValueSet* p )
    : ValueSet(p)
    , valtype_(objs ? SubObject : SubArray)
    , valarr_(0)
{
}


OD::JSON::Array::Array( DataType dt, ValueSet* p )
    : ValueSet(p)
    , valtype_(Data)
    , valarr_(new ValArr(dt))
{
}


OD::JSON::Array::Array( const Array& oth )
    : ValueSet(oth)
    , valtype_(oth.valtype_)
    , valarr_(0)
{
    if ( oth.valarr_ )
	valarr_ = new ValArr( *oth.valarr_ );
}


OD::JSON::Array::~Array()
{
    delete valarr_;
}


void OD::JSON::Array::setEmpty()
{
    if ( valarr_ )
	valarr_->setEmpty();

    ValueSet::setEmpty();
}


bool OD::JSON::Array::isData() const
{
    return valtype_ == Data;
}


bool OD::JSON::Array::isEmpty() const
{
    return isData() ? valArr().isEmpty() : ValueSet::isEmpty();
}


OD::JSON::ValueSet::size_type OD::JSON::Array::size() const
{
    return isData() ? valArr().size() : ValueSet::size();
}


void OD::JSON::Array::addVS( ValueSet* vset )
{
    if ( isData() )
	{ pErrMsg("add child to value Array"); }
    else if ( (valtype_==SubArray) != vset->isArray() )
	{ pErrMsg("add wrong child type to Array"); }
    else
    {
	vset->setParent( this );
	values_ += new Value( vset );
    }
}


OD::JSON::Array* OD::JSON::Array::add( Array* arr )
{
    addVS( arr );
    return arr;
}


OD::JSON::Object* OD::JSON::Array::add( Object* obj )
{
    addVS( obj );
    return obj;
}


bool OD::JSON::Array::getBoolValue( idx_type idx ) const
{
    if ( !isData() )
	return ValueSet::getBoolValue( idx );

    return valArr().validIdx( idx ) ? static_cast<bool>(valArr().bools()[idx])
				    : false;
}


od_int64 OD::JSON::Array::getIntValue( idx_type idx ) const
{
    if ( !isData() )
	return ValueSet::getIntValue( idx );

    return valArr().validIdx( idx ) ? valArr().vals()[idx] : mUdf(od_int64);
}


double OD::JSON::Array::getDoubleValue( idx_type idx ) const
{
    if ( !isData() )
	return ValueSet::getDoubleValue( idx );

    return valArr().validIdx( idx ) ? valArr().vals()[idx] : mUdf(double);
}


BufferString OD::JSON::Array::getStringValue( idx_type idx ) const
{
    if ( !isData() )
	return ValueSet::getStringValue( idx );

    BufferString ret;
    if ( !valArr().validIdx(idx) )
	return ret;

    ret.set( valArr().strings().get(idx) );
    return ret;
}


FilePath OD::JSON::Array::getFilePath( idx_type idx ) const
{
    return isData() ? valArr().getFilePath( idx )
		    : ValueSet::getFilePath( idx );
}


static const char* addarrnonvalstr = "add value to non-value Array";

#define mDefArrayAddVal(inptyp,fn,valtyp) \
OD::JSON::Array& OD::JSON::Array::add( inptyp val ) \
{ \
    if ( valtype_ != Data ) \
	{ pErrMsg(addarrnonvalstr); } \
    else \
	valarr_->fn().add( (valtyp)val ); \
    return *this; \
}

mDefArrayAddVal( bool, bools, bool )
mDefArrayAddVal( od_int16, vals, NumberType )
mDefArrayAddVal( od_uint16, vals, NumberType )
mDefArrayAddVal( od_int32, vals, NumberType )
mDefArrayAddVal( od_uint32, vals, NumberType )
mDefArrayAddVal( od_int64, vals, NumberType )
mDefArrayAddVal( float, vals, NumberType )
mDefArrayAddVal( double, vals, NumberType )
mDefArrayAddVal( const char*, strings, const char* )

OD::JSON::Array& OD::JSON::Array::add( const DBKey& dbky )
{
    return add( dbky.toString(true) );
}


OD::JSON::Array& OD::JSON::Array::add( const MultiID& mid )
{
    return add( mid.toString() );
}

OD::JSON::Array& OD::JSON::Array::add( const uiString& val )
{
    BufferString bs;
    val.fillUTF8String( bs );
    return add( bs.str() );
}

OD::JSON::Array& OD::JSON::Array::add( const OD::String& odstr )
{
    return add( odstr.str() );
}


OD::JSON::Array& OD::JSON::Array::add( const FilePath& fp )
{
    BufferString bs( fp.fullPath() );
#ifdef __win__
    bs.replace( "\\", "/" );
#endif
    return add( bs.buf() );
}


template<class T>
OD::JSON::Array& OD::JSON::Array::setVals( const TypeSet<T>& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( Number );
    copy( valarr_->vals(), vals );
    return *this;
}


template<class T>
OD::JSON::Array& OD::JSON::Array::setVals( const T* vals, size_type sz )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( Number );
    valarr_->vals().setSize( sz );
    OD::memCopy( valarr_->vals().arr(), vals, sz*sizeof(T) );
    return *this;
}


#define mDefArraySetVals( typ ) \
    OD::JSON::Array& OD::JSON::Array::set( typ val ) \
	{ return setVals(&val,1); } \
    OD::JSON::Array& OD::JSON::Array::set( const TypeSet<typ>& vals ) \
	{ return setVals(vals); } \
    OD::JSON::Array& OD::JSON::Array::set( const typ* vals, size_type sz ) \
	{ return setVals(vals,sz); }

mDefArraySetVals( od_int16 )
mDefArraySetVals( od_uint16 )
mDefArraySetVals( od_int32 )
mDefArraySetVals( od_uint32 )
mDefArraySetVals( od_int64 )
mDefArraySetVals( float )
mDefArraySetVals( double )


OD::JSON::Array& OD::JSON::Array::set( const char* val )
{
    return set( BufferStringSet(val) );
}

OD::JSON::Array& OD::JSON::Array::set( const DBKey& dbky )
{
    return set( DBKeySet(dbky) );
}

OD::JSON::Array& OD::JSON::Array::set( const uiString& val )
{
    return set( uiStringSet(val) );
}

OD::JSON::Array& OD::JSON::Array::set( const OD::String& val )
{
    return set( val.str() );
}

OD::JSON::Array& OD::JSON::Array::set( const FilePath& fp )
{
    BufferString val( fp.fullPath() );
#ifdef __win__
    val.replace( "\\", "/" );
#endif
    return set( BufferStringSet(val) );
}

OD::JSON::Array& OD::JSON::Array::set( bool val )
{
    return set( BoolTypeSet(val) );
}


OD::JSON::Array& OD::JSON::Array::set( const BoolTypeSet& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( Boolean );
    valarr_->bools() = vals;
    return *this;
}


OD::JSON::Array& OD::JSON::Array::set( const bool* vals, size_type sz )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( Boolean );
    for ( auto idx=0; idx<sz; idx++ )
	valarr_->bools().add( vals[idx] );

    return *this;
}


OD::JSON::Array& OD::JSON::Array::set( const BufferStringSet& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( String );
    valarr_->strings() = vals;
    return *this;
}


OD::JSON::Array& OD::JSON::Array::set( const DBKeySet& vals )
{
    BufferStringSet bss;
    vals.addTo( bss );
    return set( bss );
}


OD::JSON::Array& OD::JSON::Array::set( const uiStringSet& vals )
{
    BufferStringSet bss;
    for ( auto uistrptr : vals )
    {
	BufferString bs;
	uistrptr->fillUTF8String( bs );
	bss.add( bs );
    }

    return set( bss );
}


//--------- Object


OD::JSON::Object::Object( const Object& oth )
    : ValueSet(oth)
{
}

OD::JSON::ValueSet::idx_type OD::JSON::Object::indexOf( const char* nm ) const
{
    idx_type idx = 0;
    for ( auto val : values_ )
    {
	const KeyedValue& kydval = *static_cast<KeyedValue*>( val );
	if ( kydval.key_ == nm )
	    return idx;
	idx++;
    }

    return -1;
}


void OD::JSON::Object::getSubObjKeys( BufferStringSet& bss ) const
{
    for ( auto val : values_ )
    {
	if ( !val->isKeyed() )
	    continue;

	const KeyedValue& kydval = *static_cast<KeyedValue*>( val );
	bss.add( kydval.key_ );
    }
}


OD::JSON::ValueSet* OD::JSON::Object::gtChildByKey( const char* ky ) const
{
    const idx_type idx = indexOf( ky );
    return idx < 0 ? nullptr : gtChildByIdx( idx );
}


OD::JSON::Array* OD::JSON::Object::gtArrayByKey( const char* ky ) const
{
    ValueSet* vs = gtChildByKey( ky );
    if ( !vs )
	return nullptr;
    else if ( !vs->isArray() )
    {
	pErrMsg("Request for child Array which is an Object");
	return nullptr;
    }

    return static_cast<Array*>( vs );
}


OD::JSON::Object* OD::JSON::Object::gtObjectByKey( const char* ky ) const
{
    ValueSet* vs = gtChildByKey( ky );
    if ( !vs )
	return nullptr;
    else if ( vs->isArray() )
    {
	pErrMsg("Request for child Object which is an Array");
	return nullptr;
    }

    return static_cast<Object*>( vs );
}


OD::JSON::ValueSet* OD::JSON::Object::gtChildByKeys(
					    const BufferStringSet& kys ) const
{
    ValueSet* vs = (ValueSet*) this;
    for ( int idk=0; idk<kys.size(); idk++ )
    {
	if ( !vs || vs->isArray() )
	    return nullptr;
	const BufferString& key = kys.get( idk );
	vs = ( vs->asObject() ).gtChildByKey( key );
	if ( !vs )
	    return nullptr;
    }

    return vs;
}

OD::JSON::Array* OD::JSON::Object::gtArrayByKeys(
					    const BufferStringSet& kys ) const
{
    ValueSet* vs = gtChildByKeys( kys );
    if ( !vs || !vs->isArray() )
	return nullptr;
    else
	return static_cast<Array*>( vs );
}


OD::JSON::Object* OD::JSON::Object::gtObjectByKeys(
					    const BufferStringSet& kys ) const
{
    ValueSet* vs = gtChildByKeys( kys );
    if ( !vs || vs->isArray() )
	return nullptr;
    else
	return static_cast<Object*>( vs );
}


bool OD::JSON::Object::getBoolValue( const char* ky ) const
{
    return ValueSet::getBoolValue( indexOf(ky) );
}


od_int64 OD::JSON::Object::getIntValue( const char* ky ) const
{
    return ValueSet::getIntValue( indexOf(ky) );
}


double OD::JSON::Object::getDoubleValue( const char* ky ) const
{
    return ValueSet::getDoubleValue( indexOf(ky) );
}


BufferString OD::JSON::Object::getStringValue( const char* ky ) const
{
    const BufferString ret = ValueSet::getStringValue( indexOf(ky) );
#ifdef __debug__
# ifdef __win__
    const FilePath fp( ret );
    if ( !fp.isURI() && fp.exists() && fp.isAbsolute() )
    {
	pErrMsg( "Should not use getStringValue for a filepath" );
	DBG::forceCrash( false );
    }
# endif
#endif
    return ret;
}


FilePath OD::JSON::Object::getFilePath( const char* ky ) const
{
    return ValueSet::getFilePath( indexOf(ky) );
}


bool OD::JSON::Object::getGeomID( const char* ky, Pos::GeomID& gid ) const
{
    if ( !isPresent(ky) )
	return false;

    gid.set( getIntValue(ky) );
    return true;
}


bool OD::JSON::Object::getStrings( const char* ky, BufferStringSet& bss ) const
{
    const Array* stringsarr = getArray( ky );
    if ( !stringsarr || stringsarr->valType() != Data )
	return false;

    bss = stringsarr->valArr().strings();
    return !bss.isEmpty();
}


void OD::JSON::Object::set( KeyedValue* val )
{
    const idx_type idx = indexOf( val->key_ );
    if ( idx >= 0 )
	delete values_.removeSingle( idx );

    values_ += val;
}


static const char* errnoemptykey = "Empty key not allowed for Object's";

void OD::JSON::Object::setVS( const char* ky, ValueSet* vset )
{
    if ( !vset )
	{}
    else if ( !ky || !*ky )
	{ pErrMsg(errnoemptykey); }
    else
    {
	vset->setParent( this );
	set( new KeyedValue(ky,vset) );
    }
}


OD::JSON::Array* OD::JSON::Object::set( const char* ky, Array* arr )
{
    setVS( ky, arr );
    return arr;
}


OD::JSON::Object* OD::JSON::Object::set( const char* ky, Object* obj )
{
    setVS( ky, obj );
    return obj;
}


template <class T>
void OD::JSON::Object::setVal( const char* ky, T t )
{
    if ( !ky || !*ky )
	{ pErrMsg(errnoemptykey); return; }

    set( new KeyedValue(ky,t) );
}


#define mDefObjectSetVal(typ) \
void OD::JSON::Object::set( const char* ky, typ val ) { setVal(ky,val); }

mDefObjectSetVal( bool )
mDefObjectSetVal( od_int16 )
mDefObjectSetVal( od_uint16 )
mDefObjectSetVal( od_int32 )
mDefObjectSetVal( od_uint32 )
mDefObjectSetVal( od_int64 )
mDefObjectSetVal( float )
mDefObjectSetVal( double )
mDefObjectSetVal( const char* )

void OD::JSON::Object::set( const char* ky, const FilePath& fp )
{
    BufferString fnm( fp.fullPath() );
#ifdef __win__
    fnm.replace( "\\", "/" );
#endif
    setVal( ky, fnm.buf() );
}

void OD::JSON::Object::set( const char* ky, const DBKey& id )
{
    setVal( ky, id.toString(false) );
}


void OD::JSON::Object::set( const char* ky, const MultiID& id )
{
    setVal( ky, id.toString() );
}


void OD::JSON::Object::set( const char* ky, const uiString& str )
{
    BufferString bs;
    str.fillUTF8String( bs );
    setVal( ky, bs );
}


void OD::JSON::Object::remove( const char* ky )
{
    const idx_type idx = indexOf( ky );
    if ( idx >= 0 )
	delete values_.removeSingle( idx );
}
