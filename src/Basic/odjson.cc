/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odjson.h"

#include "dbkey.h"
#include "gason.h"
#include "od_iostream.h"
#include "posgeomid.h"
#include "posidxpair.h"
#include "stringbuilder.h"
#include "uistrings.h"

#include <QJsonDocument>

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

	NumberType  val_;
	INumberType ival_;
	bool	    bool_;
	char*	    str_;
	ValueSet*   vset_;
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
    INumberType&	ival()		{ return cont_.ival_; }
    INumberType		ival() const	{ return cont_.ival_; }
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
	    case INumber:	newval->setValue( ival() );	break;
	    case String:	newval->setValue( str() );	break;
	    case Mixed:		break;
	}
    }
    return newval;
}

#define mDefSimpleConstr( typ, cast ) \
    Value( typ v ) { setValue( (cast)(v) ); }

mDefSimpleConstr( bool, bool )
mDefSimpleConstr( od_int16, INumberType )
mDefSimpleConstr( od_uint16, INumberType )
mDefSimpleConstr( od_int32, INumberType )
mDefSimpleConstr( od_uint32, INumberType )
mDefSimpleConstr( od_int64, INumberType )
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

void setValue( INumberType v )
{
    cleanUp();
    type_ = (int)INumber;
    cont_.ival_ = v;
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

static BufferString getPathStr( const FilePath& fp )
{
    BufferString ret = fp.fullPath();
    if ( __iswin__ && !fp.isURI() )
	ret.replace( "\\", "/" );

    return ret;
}

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
	case INumber:	set_ = new INSet;	break;
	case String:	set_ = new SSet;	break;
	case Mixed:	break;
	default:	{ pErrMsg("Unknown type"); type_ = String; }
    }
}


OD::JSON::ValArr::ValArr( const ValArr& oth )
    : ValArr(oth.type_)
{
    switch ( type_ )
    {
	case Boolean:	bools() = oth.bools();		break;
	case Number:	vals() = oth.vals();		break;
	case INumber:	ivals() = oth.ivals();		break;
	case String:	strings() = oth.strings();	break;
	case Mixed:	break;
    }
}


OD::JSON::ValArr::~ValArr()
{
    delete set_;
}


BufferString OD::JSON::ValArr::dumpJSon() const
{
    BufferString ret;
    dumpJSon( ret );
    return ret;
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
	    case INumber:
	    {
		const INumberType val = ivals()[idx];
		sb.add( val );
	    } break;
	    case String:
	    {
		const BufferString toadd( "\"", strings().get(idx), "\"" );
		sb.add( toadd );
	    } break;
	    case Mixed:
		break;
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

    strings().get( idx ) = getPathStr( fp );
}


FilePath OD::JSON::ValArr::getFilePath( idx_type idx ) const
{
    if ( !strings().validIdx(idx) )
	return FilePath();

    FilePath ret;
    if ( type_ == String )
	ret.set( strings().get(idx) );

    return ret;
}


void OD::JSON::ValArr::ensureNumber()
{
    if ( dataType() == Number || isEmpty() )
	return; //Nothing to do

    if ( dataType() != INumber )
	return; //Not supported

    const int sz = size();
    const INSet* oldset = ivals().clone();
    delete set_;
    set_ = new NSet( sz );
    for ( int idx=0; idx<sz; idx++ )
	vals()[idx] = (*oldset)[idx];

    delete oldset;
    type_ = Number;
}


//--------- ValueSet


OD::JSON::ValueSet::ValueSet( const ValueSet& oth )
    : parent_(oth.parent_)
{
    for ( const auto* val : oth.values_ )
	values_ += val->clone( this );
}


OD::JSON::ValueSet::ValueSet( ValueSet* p )
    : parent_(p)
{
}


OD::JSON::ValueSet::~ValueSet()
{
    setEmpty();
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


OD::JSON::DataType OD::JSON::ValueSet::dType( idx_type idx ) const
{
    DataType ret = Boolean;
    if ( !values_.validIdx(idx) )
	{ pErrMsg("Idx out of range"); return ret; }

    if ( valueType(idx) != Data )
	return ret;

    const Value& val = *values_.get( idx );
    return (DataType)val.type_;
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
    {
	pErrMsg("Value at idx is not ValSet");
    }

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
    {
	pErrMsg(gtvalnotplaindatastr);
	return ret;
    }

    switch ( DataType(val->type_) )
    {
	case Boolean:	ret.set( val->boolVal() ? "true" : "false" );  break;
	case Number:	ret.set( val->val() );  break;
	case INumber:	ret.set( val->ival() );  break;
	case String:	ret.set( val->str() ); break;
	default:	{ pErrMsg("Huh"); }
    }

    return ret;
}


FilePath OD::JSON::ValueSet::getFilePath( idx_type idx ) const
{
    if ( !values_.validIdx(idx) )
	return FilePath();

    const Value* val = values_[idx];
    if ( val->isValSet() )
    {
	pErrMsg( gtvalnotplaindatastr );
	return FilePath();
    }

    FilePath ret;
    if ( DataType(val->type_) == String )
	ret.set( val->str() );

    return ret;
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
	case Number:
	{
	    const double dval = val->val();
	    ret = mIsUdf(dval) ? mUdf(od_int64) : mNINT64(dval); break;
	}
	case INumber:	ret = val->ival();  break;
	case String:	ret = toInt64( val->str() );  break;
	default:	{ pErrMsg("Huh"); }
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
	case Boolean:	ret = val->boolVal() ? 0. : 1.;  break;
	case Number:	ret = val->val();  break;
	case INumber:
	{
	    const od_int64 ival = val->ival();
	    ret = mIsUdf(ival) ? mUdf(double) : (double) ival; break;
	}
	case String:	ret = toDouble( val->str() );  break;
	default:	{ pErrMsg("Huh"); }
    }

    return ret;
}


namespace Gason
{

#ifdef __msvc__
#pragma warning(push)
#pragma warning( disable : 4702)  // ln 376 was unreachable on VS13
#endif

static JsonTag getNextTag( const JsonValue& gasonval, bool allowmixed )
{
    JsonTag ret = JSON_NULL;
    if ( !allowmixed )
    {
	for ( auto gasonnode : gasonval )
	    return gasonnode->value.getTag();

	return ret;
    }

    for ( const auto* gasonnode : gasonval )
    {
	const JsonTag tag = gasonnode->value.getTag();
	if ( tag == JSON_NULL )
	    continue;

	if ( tag != ret && ret != JSON_NULL )
	    return JSON_MIXED;

	ret = tag;
    }

    return ret;
}

#ifdef __msvc__
#pragma warning(pop)
#endif

} // namespace Gason


namespace OD
{

static JSON::ValueSet* getSubVS( JSON::ValueSet* parent,
				 Gason::JsonTag tag, Gason::JsonTag nexttag )
{
    if ( tag == Gason::JSON_OBJECT )
	return new JSON::Object( parent );
    else if ( tag != Gason::JSON_ARRAY )
	return nullptr;

    const bool nextisarr = nexttag == Gason::JSON_ARRAY;
    const bool nextisobj = nexttag == Gason::JSON_OBJECT;
    if ( nextisarr || nextisobj )
	return new JSON::Array( nextisobj, parent );

    JSON::DataType dt = JSON::Boolean;
    if ( nexttag == Gason::JSON_NUMBER )
	dt = JSON::Number;
    else if ( nexttag == Gason::JSON_INUMBER )
	dt = JSON::INumber;
    else if ( nexttag == Gason::JSON_STRING )
	dt = JSON::String;
    else if ( nexttag == Gason::JSON_MIXED )
	dt = JSON::Mixed;
    else if ( nexttag == Gason::JSON_NULL )
	dt = JSON::Number;

    return new JSON::Array( dt, parent );
}

} // namespace OD


void OD::JSON::ValueSet::use( const GasonNode& gasonnode, bool allowmixed )
{
    const Gason::JsonValue& gasonval = gasonnode.value;
    const Gason::JsonTag tag = gasonval.getTag();
    bool isobj = !isArray();
    const char* ky = isobj ? gasonnode.key : nullptr;

    switch ( tag )
    {
	case Gason::JSON_NUMBER:
	{
	    const double val = gasonval.toNumber();
	    if ( isobj )
		values_ += new KeyedValue( ky, val );
	    else
	    {
		Array& arr = asArray();
		if ( allowmixed )
		{
		    if ( arr.dataType() != Number )
			arr.ensureMixed();
		}
		else
		    arr.ensureNumber();

		arr.add( val );
	    }
	} break;

	case Gason::JSON_INUMBER:
	{
	    const od_int64 val = gasonval.toInt64();
	    if ( isobj )
		values_ += new KeyedValue( ky, val );
	    else
	    {
		Array& arr = asArray();
		if ( allowmixed && arr.dataType() != INumber )
		    arr.ensureMixed();

		arr.add( val );
	    }
	} break;

	case Gason::JSON_STRING:
	{
	    const char* val = gasonval.toString();
	    if ( isobj )
		values_ += new KeyedValue( gasonnode.key, val );
	    else
	    {
		Array& arr = asArray();
		if ( allowmixed && arr.dataType() != String )
		    arr.ensureMixed();

		arr.add( val );
	    }
	} break;

	case Gason::JSON_TRUE:
	case Gason::JSON_FALSE:
	{
	    const bool val = tag == Gason::JSON_TRUE;
	    if ( isobj )
		values_ += new KeyedValue( gasonnode.key, val );
	    else
	    {
		Array& arr = asArray();
		if ( allowmixed && arr.dataType() != Boolean )
		    arr.ensureMixed();

		asArray().add( val );
	    }
	} break;

	case Gason::JSON_ARRAY:
	{
	    const Gason::JsonTag nexttag =
				Gason::getNextTag( gasonval, allowmixed );
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
		    arr->use( *subgasonnode, allowmixed );
	} break;

	case Gason::JSON_OBJECT:
	{
	    auto* obj = new Object( this );
	    if ( isobj )
		values_ += new KeyedValue( gasonnode.key, obj );
	    else
		values_ += new Value( obj );

	    for ( auto subgasonnode : gasonval )
		obj->use( *subgasonnode, allowmixed );
	} break;

	case Gason::JSON_MIXED:
	case Gason::JSON_NULL:
	{
	} break;
    }
}


OD::JSON::ValueSet* OD::JSON::ValueSet::gtByParse( char* buf, int bufsz,
			    bool allowmixedarr,
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

    if ( intovset && buf_has_array != intovset->isArray() )
    {
	if ( buf_has_array )
	    uirv = tr("Cannot parse a JSON Array into a JSON Object");
	else
	    uirv = tr("Cannot parse a JSON Object into a JSON Array");

	return intovset;
    }

    const Gason::JsonTag nexttag = getNextTag( rootgasonval, allowmixedarr );
    if ( intovset && intovset->isArray() )
    {
	const ValueType valtyp = intovset->asArray().valType();
	const bool tagisobj = nexttag == Gason::JSON_OBJECT;
	const bool tagisarray = nexttag == Gason::JSON_ARRAY;
	if ( tagisobj || tagisarray )
	{
	     if ( tagisobj && valtyp != SubObject )
		 uirv = tr("Cannot parse a JSON array of object as another "
			    "type of array");
	     else if ( tagisarray && valtyp != SubArray )
		 uirv = tr("Cannot parse a JSON array of arrays as another "
			    "type of array");
	}
	else if ( valtyp != Data )
	    uirv = tr("Cannot parse a JSON array of values as another "
		       "type of array");

	if ( uirv.isError() )
	    return intovset;
    }

    ValueSet* usevset = intovset;
    bool intoisarray = buf_has_array;
    if ( intovset )
	intoisarray = intovset->isArray();
    else
    {
	usevset = getSubVS( nullptr, rootgasonval.getTag(), nexttag );
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

    for ( const auto* gasonnode : rootgasonval )
	usevset->use( *gasonnode, allowmixedarr );

    return usevset;
}


OD::JSON::ValueSet* OD::JSON::ValueSet::getFromJSon( char* buf, int bufsz,
						     uiRetVal& uirv,
						     bool allowmixedarr )
{
    return gtByParse( buf, bufsz, allowmixedarr, uirv, nullptr );
}


uiRetVal OD::JSON::ValueSet::parseJSon( char* buf, int bufsz,
					bool allowmixedarr )
{
    uiRetVal uirv;
    gtByParse( buf, bufsz, allowmixedarr, uirv, this );
    return uirv;
}


BufferString OD::JSON::ValueSet::dumpJSon( bool pretty ) const
{
    BufferString ret;
    dumpJSon( ret, pretty );
    return ret;
}


void OD::JSON::ValueSet::dumpJSon( BufferString& bs, bool pretty ) const
{
    StringBuilder sb;
    dumpJSon( sb );
    bs = sb.result();
    if ( pretty )
    {
	QJsonDocument qjsondoc = QJsonDocument::fromJson( bs.buf() );
	bs = qjsondoc.toJson( QJsonDocument::Indented ).constData();
    }
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
	    else if ( vset.asArray().isMixed() )
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
		case INumber:
		    sb.add( val.ival() );
		    break;
		case String:
		    sb.add( "\"" ).add( val.str() ).add( "\"" );
		    break;
		case Mixed:
		    break;
	    }
	}
	if ( &val != values_.last() )
	    sb.add( "," );
    }

    sb.add( isarr ? "]" : "}" );
}


uiRetVal OD::JSON::ValueSet::read( const char* fnm, bool allowmixedarr )
{
    od_istream istrm( fnm );
    if ( istrm.isBad() )
	return uiStrings::phrCannotRead( toUiString(fnm) );

    return read( istrm, allowmixedarr );
}


OD::JSON::ValueSet* OD::JSON::ValueSet::read( const char* fnm, uiRetVal& uirv,
					      bool allowmixedarr )
{
    od_istream istrm( fnm );
    if ( istrm.isBad() )
    {
	uirv.set( uiStrings::phrCannotRead( toUiString(fnm) ) );
	return nullptr;
    }

    return read( istrm, uirv, allowmixedarr );
}


uiRetVal OD::JSON::ValueSet::write( const char* fnm, bool pretty )
{
    od_ostream ostrm( fnm );
    return write( ostrm, pretty );
}


uiRetVal OD::JSON::ValueSet::read( od_istream& strm, bool allowmixedarr )
{
    BufferString buf;
    if ( strm.getAll(buf) )
	return parseJSon( buf.getCStr(), buf.size(), allowmixedarr );

    uiRetVal uirv( uiStrings::phrCannotRead(toUiString(strm.fileName())) );
    strm.addErrMsgTo( uirv );
    return uirv;
}


OD::JSON::ValueSet* OD::JSON::ValueSet::read( od_istream& strm, uiRetVal& uirv,
					      bool allowmixedarr )
{
    BufferString buf;
    if ( strm.getAll(buf) )
	return getFromJSon( buf.getCStr(), buf.size(), uirv, allowmixedarr );

    uirv.set( uiStrings::phrCannotRead( toUiString(strm.fileName()) ) );
    strm.addErrMsgTo( uirv );
    return nullptr;
}


uiRetVal OD::JSON::ValueSet::write( od_ostream& strm, bool pretty )
{
    BufferString jsonstr;
    dumpJSon( jsonstr, pretty );
    uiRetVal uirv;
    if ( jsonstr.isEmpty() )
	return uirv;

    if ( !strm.add(jsonstr.str()).isOK() )
    {
	uirv.set( uiStrings::phrCannotWrite( toUiString(strm.fileName()) ) );
	strm.addErrMsgTo( uirv );
    }

    return uirv;
}


uiRetVal OD::JSON::ValueSet::writePretty( od_ostream& strm )
{
    return write( strm, true );
}


//--------- Array

OD::JSON::Array::Array( bool objs, ValueSet* p )
    : ValueSet(p)
    , valtype_(objs ? SubObject : SubArray)
{
}


OD::JSON::Array::Array( DataType dt, ValueSet* p )
    : ValueSet(p)
    , valtype_(Data)
{
    if ( dt != DataType::Mixed )
	valarr_ = new ValArr(dt);
}


OD::JSON::Array::Array( const Array& oth )
    : ValueSet(oth)
    , valtype_(oth.valtype_)
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


bool OD::JSON::Array::isMixed() const
{
    return isData() && !valarr_;
}


bool OD::JSON::Array::isEmpty() const
{
    return isData() && !isMixed() ? valArr().isEmpty() : ValueSet::isEmpty();
}


bool OD::JSON::Array::validIdx( idx_type idx ) const
{
    return isData() && !isMixed() ? valArr().validIdx( idx )
				  : ValueSet::validIdx( idx );
}


OD::JSON::DataType OD::JSON::Array::dType( idx_type idx ) const
{
    if ( dataType() == OD::JSON::Mixed )
	return ValueSet::dType( idx );

    return dataType();
}


OD::JSON::ValueSet::size_type OD::JSON::Array::size() const
{
    return isData() && !isMixed() ? valArr().size() : ValueSet::size();
}


OD::JSON::DataType OD::JSON::Array::dataType() const
{
    return isMixed() ? DataType::Mixed : valarr_->dataType();
}


bool OD::JSON::Array::get( ::IdxPair& pos ) const
{
    if ( size() < 2 || (!isData() && !isMixed()) )
	return false;

    pos.first = mCast(int,getIntValue(0));
    pos.second = mCast(int,getIntValue(1));
    return true;
}


bool OD::JSON::Array::get( Coord& crd ) const
{
    if ( size() < 2 || (!isData() && !isMixed()) )
	return false;

    crd.x_ = getDoubleValue(0);
    crd.y_ = getDoubleValue(1);
    return true;
}


bool OD::JSON::Array::get( Coord3& crd ) const
{
    if ( size() < 3 || (!isData() && !isMixed()) )
	return false;

    crd.x_ = getDoubleValue(0);
    crd.y_ = getDoubleValue(1);
    crd.z_ = getDoubleValue(2);
    return true;
}


bool OD::JSON::Array::get( TypeSet<Coord>& coords ) const
{
    if ( valType() != SubArray )
	return false;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !isArrayChild(idx) )
	    return false;

	Coord crd = Coord::udf();
	if ( !array(idx).get(crd) )
	    return false;

	coords += crd;
    }

    return true;
}


bool OD::JSON::Array::get( TypeSet<Coord3>& coords ) const
{
    if ( valType() != SubArray )
	return false;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !isArrayChild(idx) )
	    return false;

	Coord3 crd = Coord3::udf();
	if ( !array(idx).get(crd) )
	    return false;

	coords += crd;
    }

    return true;
}


bool OD::JSON::Array::get( BufferStringSet& bss ) const
{
    if ( !isData() && !isMixed() )
	return false;

    bss.setEmpty();
    for ( int idx=0; idx<size(); idx++ )
	bss.add( getStringValue(idx) );

    return true;
}


bool OD::JSON::Array::get( uiStringSet& uistrs ) const
{
    BufferStringSet strs;
    if ( !get(strs) )
	return false;

    uistrs.setEmpty();
    for ( const auto* str : strs )
	uistrs.add( toUiString(str->buf()) );

    return true;
}


bool OD::JSON::Array::get( TypeSet<MultiID>& mids ) const
{
    BufferStringSet strs;
    if ( !get(strs) )
	return false;

    mids.setEmpty();
    bool allok = true;
    for ( const auto* str : strs )
    {
	MultiID mid;
	if ( !mid.fromString(str->buf()) )
	    allok = false;

	mids.add( mid );
    }

    return allok;
}


bool OD::JSON::Array::get( DBKeySet& keys ) const
{
    BufferStringSet strs;
    if ( !get(strs) )
	return false;

    keys.setEmpty();
    bool allok = true;
    for ( const auto* str : strs )
    {
	DBKey dbky;
	if ( !dbky.fromString(str->buf()) )
	    allok = false;

	keys.add( dbky );
    }

    return allok;
}


bool OD::JSON::Array::get( TypeSet<FilePath>& fps ) const
{
    BufferStringSet strs;
    if ( !get(strs) )
	return false;

    fps.setEmpty();
    for ( const auto* str : strs )
	fps += FilePath( str->buf() );

    return true;
}


bool OD::JSON::Array::get( BoolTypeSet& arr ) const
{
    if ( !isData() && !isMixed() )
	return false;

    const int sz = size();
    if ( !arr.setSize(sz) )
	return false;

    for ( int idx=0; idx<sz; idx++ )
	arr[idx] = getBoolValue( idx );

    return true;
}


OD::JSON::Array& OD::JSON::Array::add( Coord crd )
{
    if ( valType() != SubArray )
	return *this;

    auto* jsarr = new Array( Number );
    jsarr->add( crd.x_ ).add( crd.y_ );
    add( jsarr );
    return *this;
}


OD::JSON::Array& OD::JSON::Array::add( Coord3 crd )
{
    if ( valType() != SubArray )
	return *this;

    auto* jsarr = new Array( Number );
    jsarr->add( crd.x_ ).add( crd.y_ ).add( crd.z_ );
    add( jsarr );
    return *this;
}


OD::JSON::Array& OD::JSON::Array::add( const TypeSet<Coord>& coords )
{
    if ( valType() != SubArray )
	return *this;

    auto* jsarr = new Array( false );
    for ( const auto& crd : coords )
	jsarr->add( crd );

    add( jsarr );
    return *this;
}


OD::JSON::Array& OD::JSON::Array::add( const TypeSet<Coord3>& coords )
{
    if ( valType() != SubArray )
	return *this;

    auto* jsarr = new Array( false );
    for ( const auto& crd : coords )
	jsarr->add( crd );

    add( jsarr );
    return *this;
}


void OD::JSON::Array::addVS( ValueSet* vset )
{
    vset->setParent( this );
    values_ += new Value( vset );
}


void OD::JSON::Array::ensureNumber()
{
    if ( isData() && !isMixed() )
	valArr().ensureNumber();
}


void OD::JSON::Array::ensureMixed()
{
    if ( isMixed() || !isData() )
	return;

    if ( isEmpty() )
    {
	deleteAndNullPtr( valarr_ );
	return;
    }

    const DataType datatyp = dataType();
    if ( datatyp == Boolean )
    {
	for ( const auto& val : valArr().bools() )
	    values_.add( new Value(val) );
    }
    else if ( datatyp == Number )
    {
	for ( const auto& val : valArr().vals() )
	    values_.add( new Value(val) );
    }
    else if ( datatyp == INumber )
    {
	for ( const auto& val : valArr().ivals() )
	    values_.add( new Value(val) );
    }
    else if ( datatyp == String )
    {
	for ( const auto& val : valArr().strings() )
	    values_.add( new Value(val) );
    }

    deleteAndNullPtr( valarr_ );
}


OD::JSON::Array* OD::JSON::Array::add( Array* arr )
{
    if ( valType()==SubArray )
	addVS( arr );
    else
	{ pErrMsg("add Array to Array of data or sub-objects"); }

    return arr;
}


OD::JSON::Object* OD::JSON::Array::add( Object* obj )
{
    if ( valType()==SubObject )
	addVS( obj );
    else
	{ pErrMsg("add Object to Array of data or sub-arrays"); }

    return obj;
}


bool OD::JSON::Array::getBoolValue( idx_type idx ) const
{
    if ( !isData() || isMixed() )
	return ValueSet::getBoolValue( idx );

    return valArr().validIdx( idx ) ? static_cast<bool>(valArr().bools()[idx])
				    : false;
}


od_int64 OD::JSON::Array::getIntValue( idx_type idx ) const
{
    if ( !isData() || isMixed())
	return ValueSet::getIntValue( idx );

    if ( !valArr().validIdx(idx) )
	return mUdf(od_int64);

    od_int64 ret = mUdf(od_int64);
    switch ( DataType(valArr().dataType()) )
    {
	case Boolean:	ret = valArr().bools()[idx] ? 0 : 1;  break;
	case Number:
	{
	    const double dval = valArr().vals()[idx];
	    ret = mIsUdf(dval) ? mUdf(od_int64) : mNINT64(dval); break;
	}
	case INumber:	ret = valArr().ivals()[idx];  break;
	case String:	ret = toInt64( valArr().strings()[idx]->buf() );  break;
	default:	{ pErrMsg("Huh"); }
    }
    return ret;
}


double OD::JSON::Array::getDoubleValue( idx_type idx ) const
{
    if ( !isData() || isMixed() )
	return ValueSet::getDoubleValue( idx );

    if ( !valArr().validIdx(idx) )
	return mUdf(double);

    double ret = mUdf(double);
    switch ( DataType(valArr().dataType()) )
    {
	case Boolean:	ret = valArr().bools()[idx] ? 0. : 1.;	break;
	case Number:	ret = valArr().vals()[idx];  break;
	case INumber:
	{
	    const od_int64 ival = valArr().ivals()[idx];
	    ret = mIsUdf(ival) ? mUdf(double) : (double) ival; break;
	}
	case String:	ret = toDouble( valArr().strings()[idx]->buf());  break;
	default:	{ pErrMsg("Huh"); }
    }

    return ret;
}


BufferString OD::JSON::Array::getStringValue( idx_type idx ) const
{
    if ( !isData() || isMixed() )
	return ValueSet::getStringValue( idx );

    BufferString ret;
    if ( !valArr().validIdx(idx) )
	return ret;

    DataType datatype = valArr().dataType();
    if ( datatype == Boolean )
	ret.set( toString(valArr().bools().get(idx)) );
    else if ( datatype == Number )
	ret.set( toString(valArr().vals().get(idx)) );
    else if ( datatype == INumber )
	ret.set( toString(valArr().ivals().get(idx)) );
    else
	ret.set( valArr().strings().get(idx) );

    return ret;
}


FilePath OD::JSON::Array::getFilePath( idx_type idx ) const
{
    return isData() && !isMixed() ? valArr().getFilePath( idx )
				  : ValueSet::getFilePath( idx );
}


OD::JSON::Array& OD::JSON::Array::set( const BufferStringSet& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( String );
    ValueSet::setEmpty();
    valarr_->strings() = vals;
    return *this;
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


OD::JSON::Array& OD::JSON::Array::set( const TypeSet<MultiID>& mids )
{
    BufferStringSet bss;
    for ( const auto& mid : mids )
	bss.add( mid.toString() );

    return set( bss );
}


OD::JSON::Array& OD::JSON::Array::set( const DBKeySet& dbkeys )
{
    BufferStringSet bss;
    dbkeys.addTo( bss );
    return set( bss );
}


OD::JSON::Array& OD::JSON::Array::set( const BoolTypeSet& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( Boolean );
    ValueSet::setEmpty();
    valarr_->bools() = vals;
    return *this;
}


OD::JSON::Array& OD::JSON::Array::set( const TypeSet<FilePath>& fps )
{
    BufferStringSet fpstrs;
    for ( const auto& fp : fps )
    {
	const BufferString fnm = getPathStr( fp );
	fpstrs.add( fnm.buf() );
    }

    return set( fpstrs );
}


OD::JSON::Array& OD::JSON::Array::set( const bool* vals, size_type sz )
{
    return setVals( vals, sz );
}


OD::JSON::Array& OD::JSON::Array::add( bool val )
{
    return addVal( val );
}


template <class T, typename Enable>
OD::JSON::Array& OD::JSON::Array::addVal( T val )
{
    if ( isMixed() )
    {
        values_ += new Value( val );
    }
    else if ( isData() )
    {
        const DataType arrtyp = dataType();
        if ( arrtyp == Number )
            valarr_->vals().add( mIsUdf(val) ? mUdf(NumberType) : val );
        else if ( arrtyp == INumber )
            valarr_->ivals().add( mIsUdf(val) ? mUdf(INumberType) : val );
        else if ( arrtyp == Boolean )
            valarr_->bools().add( mIsUdf(val) ? mUdf(BoolType) : (bool) val );
        else if ( arrtyp == String )
        {
            pErrMsg("Probably incorrect add");
            valarr_->strings().add( ::toString(val) );
        }
    }
    else
    {
	pErrMsg("Incorrect add");
    }

    return *this;
}


#define mDefArraySetAddVals( typ ) \
    OD::JSON::Array& OD::JSON::Array::set( const TypeSet<typ>& vals ) \
	{ return setVals(vals); } \
    OD::JSON::Array& OD::JSON::Array::set( const typ* vals, size_type sz ) \
	{ return setVals(vals,sz); } \
    OD::JSON::Array& OD::JSON::Array::add( typ val ) \
	{ return addVal( val ); } \
    OD::JSON::Array& OD::JSON::Array::add( const TypeSet<typ>& vals ) \
        { return addVals(vals); } \
    OD::JSON::Array& OD::JSON::Array::add( const typ* vals, size_type sz ) \
        { return addVals(vals,sz); }


mDefArraySetAddVals( od_int16 )
mDefArraySetAddVals( od_uint16 )
mDefArraySetAddVals( od_int32 )
mDefArraySetAddVals( od_uint32 )
mDefArraySetAddVals( od_int64 )
mDefArraySetAddVals( float )
mDefArraySetAddVals( double )


OD::JSON::Array& OD::JSON::Array::set( const FilePath& fp, size_type idx )
{
    if ( !validIdx(idx) )
	return *this;

    if ( isData() && !isMixed() )
	valArr().setFilePath( fp, idx );
    else
    {
	const BufferString fnm = getPathStr( fp );
	auto* newval = new Value( fnm.str() );
	delete values_.replace( idx, newval );
    }

    return *this;
}


OD::JSON::Array& OD::JSON::Array::add( const char* str )
{
    if ( isMixed() )
    {
	values_ += new Value( str );
    }
    else if ( isData() && dataType() == String )
    {
	valarr_->strings().add( str );
    }
    else
    {
	pErrMsg("Incorrect add");
    }

    return *this;
}


OD::JSON::Array& OD::JSON::Array::add( const OD::String& odstr )
{
    return add( odstr.buf() );
}


OD::JSON::Array& OD::JSON::Array::add( const uiString& val )
{
    BufferString bs;
    val.fillUTF8String( bs );
    return add( bs.buf() );
}


OD::JSON::Array& OD::JSON::Array::add( const FilePath& fp )
{
    const BufferString bs = getPathStr( fp );
    return add( bs.buf() );
}


OD::JSON::Array& OD::JSON::Array::add( const MultiID& mid )
{
    const BufferString bs = mid.toString();
    return add( bs.buf() );
}


OD::JSON::Array& OD::JSON::Array::add( const DBKey& dbky )
{
    return add( dbky.toString(true) );
}


void OD::JSON::Array::dumpJSon( StringBuilder& sb ) const
{
    if ( isData() && !isMixed() )
	valArr().dumpJSon( sb );
    else
	ValueSet::dumpJSon( sb );
}


BufferString OD::JSON::Array::dumpJSon( bool pretty ) const
{
    return ValueSet::dumpJSon( pretty );
}


//--------- Object

OD::JSON::Object::Object( ValueSet* p )
    : ValueSet(p)
{}


OD::JSON::Object::Object( const Object& oth )
    : ValueSet(oth)
{
}


OD::JSON::Object::~Object()
{}


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
#ifdef __win__
    const FilePath fp( ret );
    if ( !fp.isURI() && fp.exists() && fp.isAbsolute() )
    {
# ifdef __debug__
	pErrMsg( "Should not use getStringValue for a filepath" );
	DBG::forceCrash( false );
# else
	return getFilePath(ky).fullPath();
# endif
    }
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


MultiID OD::JSON::Object::getMultiID( const char* ky ) const
{
    if ( !isPresent(ky) )
	return MultiID::udf();

    MultiID key;
    key.fromString( getStringValue(ky).buf() );
    return key;
}


bool OD::JSON::Object::get( const char* ky, ::IdxPair& pos ) const
{
    const Array* jsarr = getArray( ky );
    return jsarr ? jsarr->get( pos ) : false;
}


bool OD::JSON::Object::get( const char* ky, Coord& crd ) const
{
    const Array* jsarr = getArray( ky );
    return jsarr ? jsarr->get( crd ) : false;
}


bool OD::JSON::Object::get( const char* ky, Coord3& crd ) const
{
    const Array* jsarr = getArray( ky );
    return jsarr ? jsarr->get( crd ) : false;
}


bool OD::JSON::Object::get( const char* ky, BufferStringSet& strs ) const
{
    const Array* stringsarr = getArray( ky );
    return stringsarr ? stringsarr->get( strs ) : false;
}


bool OD::JSON::Object::get( const char* ky, uiStringSet& uistrs ) const
{
    const Array* stringsarr = getArray( ky );
    return stringsarr ? stringsarr->get( uistrs ) : false;
}


bool OD::JSON::Object::get( const char* ky, TypeSet<MultiID>& mids ) const
{
    const Array* stringsarr = getArray( ky );
    return stringsarr ? stringsarr->get( mids ) : false;
}


bool OD::JSON::Object::get( const char* ky, DBKeySet& dbkeys ) const
{
    const Array* stringsarr = getArray( ky );
    return stringsarr ? stringsarr->get( dbkeys ) : false;
}


bool OD::JSON::Object::get( const char* ky, BoolTypeSet& arr ) const
{
    const Array* boolarr = getArray( ky );
    return boolarr ? boolarr->get( arr ) : false;
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


template <class T, typename Enable>
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

void OD::JSON::Object::set( const char* ky, const char* str )
{
    if ( !ky || !*ky )
        { pErrMsg("Empty key not allowed for Object's"); return; }

    set( new KeyedValue(ky,str) );
}


void OD::JSON::Object::set( const char* ky, const OD::String& str )
{
    set( ky, str.buf() );
}


void OD::JSON::Object::set( const char* ky, const uiString& str )
{
    BufferString bs;
    str.fillUTF8String( bs );
    set( ky, bs.buf() );
}


void OD::JSON::Object::set( const char* ky, const FilePath& fp )
{
    const BufferString fnm = getPathStr( fp );
    set( ky, fnm.buf() );
}


void OD::JSON::Object::set( const char* ky, const MultiID& id )
{
    const BufferString idstr = id.toString();
    set( ky, idstr.buf() );
}


void OD::JSON::Object::set( const char* ky, const DBKey& id )
{
    const BufferString idstr = id.toString( false );
    set( ky, idstr.buf() );
}


void OD::JSON::Object::set( const char* ky, const ::IdxPair& pos )
{
    auto* jsarr = new Array( INumber );
    jsarr->add( pos.first ).add( pos.second );
    set( ky, jsarr );
}


void OD::JSON::Object::set( const char* ky, const Coord& crd )
{
    auto* jsarr = new Array( Number );
    jsarr->add( crd.x_ ).add( crd.y_ );
    set( ky, jsarr );
}


void OD::JSON::Object::set( const char* ky, const Coord3& crd )
{
    auto* jsarr = new Array( Number );
    jsarr->add( crd.x_ ).add( crd.y_ ).add( crd.z_ );
    set( ky, jsarr );
}


void OD::JSON::Object::set( const char* ky, const TypeSet<Coord>& coords )
{
    auto* jsarr = new Array( false );
    for ( const auto& crd : coords )
    {
	Array* coordarr = jsarr->add( new Array(Number) );
	coordarr->add( crd.x_ ).add( crd.y_ );
    }

    set( ky, jsarr );
}


void OD::JSON::Object::set( const char* ky, const TypeSet<Coord3>& coords )
{
    auto* jsarr = new Array( false );
    for ( const auto& crd : coords )
    {
	Array* coordarr = jsarr->add( new Array(Number) );
	coordarr->add( crd.x_ ).add( crd.y_ ).add( crd.z_ );
    }

    set( ky, jsarr );
}


void OD::JSON::Object::set( const char* ky, const BoolTypeSet& arr )
{
    auto* jsarr = new Array( Boolean );
    jsarr->set( arr );
    set( ky, jsarr );
}


void OD::JSON::Object::set( const char* ky, const BufferStringSet& bss )
{
    auto* jsarr = new Array( OD::JSON::String );
    jsarr->set( bss );
    set( ky, jsarr );
}


void OD::JSON::Object::set( const char* ky, const uiStringSet& uistrs )
{
    auto* jsarr = new Array( OD::JSON::String );
    jsarr->set( uistrs );
    set( ky, jsarr );
}


void OD::JSON::Object::set( const char* ky, const TypeSet<MultiID>& mids )
{
    auto* jsarr = new Array( OD::JSON::String );
    jsarr->set( mids );
    set( ky, jsarr );
}


void OD::JSON::Object::set( const char* ky, const DBKeySet& dbkys )
{
    auto* jsarr = new Array( OD::JSON::String );
    jsarr->set( dbkys );
    set( ky, jsarr );
}


void OD::JSON::Object::remove( const char* ky )
{
    const idx_type idx = indexOf( ky );
    if ( idx >= 0 )
	delete values_.removeSingle( idx );
}
