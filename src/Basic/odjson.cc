/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert/Arnaud
 * DATE     : April 2018
-*/

#include "odjson.h"
#include "multiid.h"
#include "od_iostream.h"
#include "separstr.h"
#include "typeset.h"
#include "arraynd.h"
#include "uistrings.h"
#include "gason.h"
#include <string.h>


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
    int			type_;

    virtual bool	isKeyed() const { return false; }

    ValueSet*		vSet()		{ return cont_.vset_; }
    const ValueSet*	vSet() const	{ return cont_.vset_; }
    bool&		boolVal()	{ return cont_.bool_; }
    bool		boolVal() const { return cont_.bool_; }
    NumberType&		val()		{ return cont_.val_; }
    NumberType		val() const	{ return cont_.val_; }
    char*		str()		{ return cont_.str_; }
    const char*		str() const	{ return cont_.str_; }


Value() : type_( (int)Number )		{}
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
	switch ( (DataType)type_ )
	{
	    case Boolean:	newval->setValue( boolVal() );	break;
	    case Number:	newval->setValue( val() );	break;
	    case String:	newval->setValue( str() );	break;
	}
    }
    return newval;
}

#define mDefSimpleConstr( typ, cast ) \
Value( typ v ) { setValue( (cast)v ); }

mDefSimpleConstr( bool, bool )
mDefSimpleConstr( od_int16, NumberType )
mDefSimpleConstr( od_uint16, NumberType )
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
	delete[] str();
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
    const int len = FixedString( cstr ).size();
    char* contstr = new char[len + 1];
    strcpy( contstr, cstr );
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

class KeyedValue :public Value
{
public:

    BufferString	key_;

    virtual bool isKeyed() const { return true; }
    virtual Value* getEmptyClone() const { return new KeyedValue(key_); }

KeyedValue( const char* ky ) : key_(ky) {}

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
    : type_( typ )
{
    switch ( type_ )
    {
    case Boolean:	set_ = new BSet;	break;
    case Number:	set_ = new NSet;	break;
    default:		{ pErrMsg( "Unknown type" ); type_ = String; }
    case String:	set_ = new SSet;	break;
    }
}


OD::JSON::ValArr::ValArr( const ValArr& oth )
    : ValArr( oth.type_ )
{
    switch ( type_ )
    {
    case Boolean:	bools() = oth.bools();		break;
    case Number:	vals() = oth.vals();		break;
    case String:	strings() = oth.strings();	break;
    }
}


void OD::JSON::ValArr::dumpJSon( BufferString& bs ) const
{
    const int sz = (size_type)set_->nrItems();
    bs.add( "[" );
    for ( int idx=0; idx<sz; idx++ )
    {
	switch ( type_ )
	{
	    case Boolean:
	    {
		const bool val = bools()[idx];
		bs.add( val ? "true" : "false" );
	    } break;
	    case Number:
	    {
		const NumberType val = vals()[idx];
		bs.add( val );
	    } break;
	    case String:
	    {
		const BufferString toadd( "\"", strings().get( idx ), "\"" );
		bs.add( toadd );
	    } break;
	}
	if ( idx != sz-1 )
	    bs.add( "," );
    }
    bs.add( "]" );
}


//--------- ValueSet


OD::JSON::ValueSet::ValueSet( const ValueSet& oth )
    : parent_( oth.parent_ )
{
    for ( int idx=0; idx<oth.values_.size(); idx++ )
	values_ += oth.values_[idx]->clone( this );
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
    {
	pErrMsg("Idx out of range");
	return ret;
    }

    const Value& val = *values_[idx];
    if ( val.isValSet() )
	ret = val.vSet()->isArray() ? SubArray : SubObject;

    return ret;
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
	return 0;
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
	return 0;
    return static_cast<Array*>( vset );
}


OD::JSON::Object* OD::JSON::ValueSet::gtObjectByIdx( idx_type idx ) const
{
    ValueSet* vset = gtChildByIdx( idx );
    if ( !vset || vset->isArray() )
	return 0;
    return static_cast<Object*>( vset );
}



BufferString OD::JSON::ValueSet::getStringValue( idx_type idx ) const
{
    BufferString ret;
    if ( !values_.validIdx( idx ) )
	return ret;
    const Value* val = values_[idx];
    if ( val->isValSet() )
	{ pErrMsg(gtvalnotplaindatastr()); return ret; }

    switch ( (DataType)val->type_ )
    {
	case Boolean:	ret.set( val->boolVal() ? "true" : "false" );  break;
	case Number:	ret.set( val->val() );	break;
	default:		{ pErrMsg("Huh"); }
	case String:	ret.set( val->str() );	break;
    }
    return ret;
}


od_int64 OD::JSON::ValueSet::getIntValue( idx_type idx ) const
{
    od_int64 ret = mUdf(od_int64);
    if ( !values_.validIdx(idx) )
	return ret;
    const Value* val = values_[idx];
    if ( val->isValSet() )
    {
	pErrMsg(gtvalnotplaindatastr());
	return ret;
    }

    switch ( (DataType)val->type_ )
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
    {
	pErrMsg(gtvalnotplaindatastr());
	return ret;
    }

    switch ( (DataType)val->type_ )
    {
	case Boolean:	ret = val->boolVal() ? 0 : 1;  break;
	case Number:	ret = val->val();  break;
	default:	{ pErrMsg( "Huh" ); }
	case String:	ret = toDouble( val->str() );  break;
    }
    return ret;
}


#ifdef __msvc__
#pragma warning(push)
#pragma warning( disable : 4702 )  // ln 376 was unreachable on VS13
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
	return 0;

    const bool nextisarr = nexttag == Gason::JSON_ARRAY;
    const bool nextisobj = nexttag == Gason::JSON_OBJECT;
    if ( nextisarr || nextisobj )
	return new OD::JSON::Array( nextisobj, parent );

    OD::JSON::DataType dt = OD::JSON::Boolean;
    if ( nexttag == Gason::JSON_NUMBER )
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
    {
	uirv.set( toUiString("No data to parse (JSON)") );
	return intovset;
    }

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
	uirv.set( tr("JSON parse error: '%1' at char %2")
					.arg(gasonerr).arg(endptr-buf+1) );
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
	usevset = getSubVS( 0, rootgasonval.getTag(),
						    getNextTag(rootgasonval) );
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


void OD::JSON::ValueSet::dumpJSon( BufferString& str ) const
{
    const bool isarr = isArray();
    str.add( isarr ? "[" : "{" );
    for ( int idx=0; idx<values_.size(); idx++ )
    {
	const Value& val = *values_[idx];
	BufferString toadd;
	if ( val.isKeyed() )
	{
	    const KeyedValue& keyedval = static_cast<const KeyedValue&>( val );
	    toadd.set( "\"" ).add( keyedval.key_ ).add( "\":" );
	}

	if ( val.isValSet() )
	{
	    const ValueSet& vset = *val.vSet();
	    if ( !vset.isArray() || vset.asArray().valType() != Data )
		vset.dumpJSon( toadd );
	    else
		vset.asArray().valArr().dumpJSon( toadd );
	}
	else
	{
	    switch ( (DataType)val.type_ )
	    {
		case Boolean:
		    toadd.add( val.boolVal() ? "true" : "false" );
		    break;
		case Number:
		    toadd.add( val.val() );
		    break;
		case String:
		    toadd.add( "\"" ).add( val.str() ).add( "\"" );
		    break;
	    }
	}
	if ( &val != values_.last() )
	    toadd.add( "," );
	str.add( toadd );
    }
    str.add( isarr ? "]" : "}" );
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

    uirv.set( uiStrings::phrCannotRead(toUiString(strm.fileName())) );
    strm.addErrMsgTo( uirv );
    return 0;
}


uiRetVal OD::JSON::ValueSet::write( od_ostream& strm )
{
    BufferString buf;
    dumpJSon( buf );
    uiRetVal uirv;
    if ( !strm.add( buf ).isOK() )
    {
	uirv.set( uiStrings::phrCannotWrite(toUiString(strm.fileName())) );
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


OD::JSON::ValueSet::size_type OD::JSON::Array::size() const
{
    return valtype_ == Data ? valArr().size() : ValueSet::size();
}


void OD::JSON::Array::addVS( ValueSet* vset )
{
    if ( valtype_ == Data )
	pErrMsg("add child to value Array");
    else if ( (valtype_ == SubArray) != vset->isArray() )
	pErrMsg("add wrong child type to Array");
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




#define mDefArrayAddVal(inptyp,fn,valtyp) \
OD::JSON::Array& OD::JSON::Array::add( inptyp val ) \
{ \
    if ( valtype_ != Data ) \
	{ pErrMsg(addarrnonvalstr()); } \
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

OD::JSON::Array& OD::JSON::Array::add( const uiString& val )
{
    BufferString bs;
    val.fillUTF8String( bs );
    return add( bs.str() );
}


template<class T>
void OD::JSON::Array::setVals( const TypeSet<T>& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( Number );
    for ( int idx=0; idx<vals.size(); idx++ )
	valarr_->vals().add( mCast(NumberType,vals[idx]) );
}


#define mDefArraySetVals(inptyp) \
void OD::JSON::Array::set( const TypeSet<inptyp>& vals ) { setVals(vals); }

mDefArraySetVals( od_int16 )
mDefArraySetVals( od_uint16 )
mDefArraySetVals( od_int32 )
mDefArraySetVals( od_uint32 )
mDefArraySetVals( od_int64 )
mDefArraySetVals( float )
mDefArraySetVals( double )


void OD::JSON::Array::set( const BoolTypeSet& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( Boolean );
    valarr_->bools() = vals;
}


void OD::JSON::Array::set( const BufferStringSet& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( String );
    valarr_->strings() = vals;
}


void OD::JSON::Array::set( const uiStringSet& vals )
{
    BufferStringSet bss;
    for ( int idx=0; idx<vals.size(); idx++ )
    {
	BufferString bs;
	vals[idx].fillUTF8String( bs );
	bss.add( bs );
    }
    set( bss );
}


//--------- Object


OD::JSON::Object::Object( const Object& oth )
    : ValueSet(oth)
{
}

OD::JSON::ValueSet::idx_type OD::JSON::Object::indexOf( const char* nm ) const
{
    idx_type idx = 0;
    for ( int sidx=0; sidx<size(); sidx++ )
    {
	OD::JSON::Value* val = const_cast<Value*>( values_[sidx] );
	if ( !val )
	    continue;
	const KeyedValue& kydval = *static_cast<KeyedValue*>( val );
	if ( kydval.key_ == nm )
	    return idx;
	idx++;
    }
    return -1;
}

OD::JSON::ValueSet* OD::JSON::Object::gtChildByKey( const char* ky ) const
{
    const idx_type idx = indexOf( ky );
    return idx < 0 ? 0 : gtChildByIdx( idx );
}


OD::JSON::Array* OD::JSON::Object::gtArrayByKey( const char* ky ) const
{
    ValueSet* vs = gtChildByKey( ky );
    if ( !vs )
	return 0;
    else if ( !vs->isArray() )
    {
	pErrMsg("Request for child Array which is an Object");
	return 0;
    }

    return static_cast<Array*>(vs);
}


OD::JSON::Object* OD::JSON::Object::gtObjectByKey( const char* ky ) const
{
    ValueSet* vs = gtChildByKey( ky );
    if ( !vs )
	return 0;
    else if ( vs->isArray() )
    {
	pErrMsg("Request for child Object which is an Array");
	return 0;
    }

    return static_cast<Object*>( vs );
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
    return ValueSet::getStringValue( indexOf(ky) );
}


void OD::JSON::Object::set( KeyedValue* val )
{
    const idx_type idx = indexOf( val->key_ );
    if ( idx >= 0 )
	delete values_.removeSingle( idx );
    values_ += val;
}



void OD::JSON::Object::setVS( const char* ky, ValueSet* vset )
{
    if ( !vset )
	return;

    if ( !ky || !*ky )
    { pErrMsg(errnoemptykey()); }
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
    {
	pErrMsg(errnoemptykey);
	return;
    }
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

void OD::JSON::Object::set( const char* ky, const MultiID& id )
{
    setVal( ky, id.buf() );
}


void OD::JSON::Object::remove( const char* ky )
{
    const idx_type idx = indexOf( ky );
    if ( idx >= 0 )
	delete values_.removeSingle( idx );
}
