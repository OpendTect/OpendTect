/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert/Arnaud
 * DATE     : April 2018
-*/

#include "odjson.h"
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

    virtual bool	isKeyed() const	{ return false; }

    ValueSet*		vSet()		{ return cont_.vset_; }
    const ValueSet*	vSet() const	{ return cont_.vset_; }
    bool&		boolVal()	{ return cont_.bool_; }
    bool		boolVal() const	{ return cont_.bool_; }
    NumberType&		val()		{ return cont_.val_; }
    NumberType		val() const	{ return cont_.val_; }
    char*		str()		{ return cont_.str_; }
    const char*		str() const	{ return cont_.str_; }

    Contents		cont_;
    int			type_;

Value() : type_((int)Number) {}

#define mDefSimpleConstr( typ, cast ) \
    Value( typ v ) { setValue( (cast)v ); }

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
    const int len = FixedString(cstr).size();
    char* contstr = new char[ len + 1 ];
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


//--------- ValueSet

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
	ret = val.vSet()->isArray() ? SubArray : SubNode;

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
	{ pErrMsg("Value at idx is not ValSet"); }
    return const_cast<ValueSet*>( val->vSet() );
}


OD::JSON::Array* OD::JSON::ValueSet::gtArrayByIdx( idx_type idx ) const
{
    ValueSet* vset = gtChildByIdx( idx );
    if ( !vset || !vset->isArray() )
	return 0;
    return static_cast<Array*>( vset );
}


OD::JSON::Node* OD::JSON::ValueSet::gtNodeByIdx( idx_type idx ) const
{
    ValueSet* vset = gtChildByIdx( idx );
    if ( !vset || vset->isArray() )
	return 0;
    return static_cast<Node*>( vset );
}


static const char* gtvalnotplaindatastr = "ValueSet at idx is not plain data";

BufferString OD::JSON::ValueSet::getStringValue( idx_type idx ) const
{
    BufferString ret;
    if ( !values_.validIdx(idx) )
	return ret;
    const Value* val = values_[idx];
    if ( val->isValSet() )
	{ pErrMsg(gtvalnotplaindatastr); return ret; }

    switch ( (DataType)val->type_ )
    {
	case Boolean:	ret.set( val->boolVal() ? "true" : "false" );  break;
	case Number:	ret.set( val->val() );  break;
	default:	{ pErrMsg("Huh"); }
	case String:	ret.set( val->str() );  break;
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
	{ pErrMsg(gtvalnotplaindatastr); return ret; }

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
	{ pErrMsg(gtvalnotplaindatastr); return ret; }

    switch ( (DataType)val->type_ )
    {
	case Boolean:	ret = val->boolVal() ? 0 : 1;  break;
	case Number:	ret = val->val();  break;
	default:	{ pErrMsg("Huh"); }
	case String:	ret = toDouble( val->str() );  break;
    }
    return ret;
}


static Gason::JsonTag getNextTag( const Gason::JsonValue& gasonval )
{
    for ( auto gasonnode : gasonval )
	return gasonnode->value.getTag();
    return Gason::JSON_NULL;
}


static OD::JSON::ValueSet* getSubVS( OD::JSON::ValueSet* parent,
		Gason::JsonTag tag, Gason::JsonTag nexttag )
{
    if ( tag == Gason::JSON_OBJECT )
	return new OD::JSON::Node( parent );
    else if ( tag != Gason::JSON_ARRAY )
	return 0;

    const bool nextisarr = nexttag == Gason::JSON_ARRAY;
    const bool nextisnode = nexttag == Gason::JSON_OBJECT;
    if ( nextisarr || nextisnode )
	return new OD::JSON::Array( nextisnode, parent );

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
    bool isnode = !isArray();
    const char* ky = isnode ? gasonnode.key : 0;

    switch ( tag )
    {
	case Gason::JSON_NUMBER:
	{
	    const double val = gasonval.toNumber();
	    if ( isnode )
		values_ += new KeyedValue( ky, val );
	    else
		asArray().valArr().vals() += val;
	} break;

	case Gason::JSON_STRING:
	{
	    const char* val = gasonval.toString();
	    if ( isnode )
		values_ += new KeyedValue( gasonnode.key, val );
	    else
		asArray().valArr().strings().add( val );
	} break;

	case Gason::JSON_TRUE:
	case Gason::JSON_FALSE:
	{
	    const bool val = tag == Gason::JSON_TRUE;
	    if ( isnode )
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
		if ( isnode )
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
	    Node* node = new Node( this );
	    if ( isnode )
		values_ += new KeyedValue( gasonnode.key, node );
	    else
		values_ += new Value( node );

	    for ( auto subgasonnode : gasonval )
		node->use( *subgasonnode );
	} break;

	case Gason::JSON_NULL:
	{
	} break;
    }
}


OD::JSON::ValueSet* OD::JSON::ValueSet::parseJSon( char* buf, int bufsz,
						    uiRetVal& uirv )
{
    uirv.setOK();
    if ( !buf || bufsz < 1 )
	{ uirv.set( uiStrings::phrInternalErr("No data to parse (JSON)") );
	    return 0; }

    Gason::JsonAllocator allocator;
    Gason::JsonValue rootgasonval; char* endptr;
    int status = Gason::jsonParse( buf, &endptr, &rootgasonval, allocator );
    if ( status != Gason::JSON_OK )
    {
	BufferString gasonerr;
	if ( status == Gason::JSON_BREAKING_BAD )
	    gasonerr.set( "incomplete input" );
	else
	    gasonerr.set( Gason::jsonStrError(status) );
	uirv.set( tr("JSON parse error: '%1' at char %2")
		    .arg( gasonerr )
		    .arg( endptr-buf+1 ) );
	return 0;
    }

    const Gason::JsonTag roottag = rootgasonval.getTag();
    if ( roottag != Gason::JSON_ARRAY && roottag != Gason::JSON_OBJECT )
    {
	uirv.set( tr("Make sure JSON content starts with '{' or '['") );
	return 0;
    }

    ValueSet* vset = getSubVS( 0, rootgasonval.getTag(),
				  getNextTag(rootgasonval) );
    if ( !vset )
	uirv.set( tr("No meaningful JSON content found") );
    else
    {
	for ( auto gasonnode : rootgasonval )
	    vset->use( *gasonnode );
    }

    return vset;
}


void OD::JSON::ValueSet::dumpJSon( BufferString& str ) const
{
    str.set( "TODO" );
}


uiRetVal OD::JSON::ValueSet::read( od_istream& strm )
{
    uiRetVal uirv;
    BufferString buf;
    if ( strm.getAll(buf) )
	parseJSon( buf.getCStr(), buf.size(), uirv );
    else
    {
	uirv.set( uiStrings::phrCannotRead( toUiString(strm.fileName()) ) );
	strm.addErrMsgTo( uirv );
    }
    return uirv;
}


uiRetVal OD::JSON::ValueSet::write( od_ostream& strm )
{
    BufferString buf;
    dumpJSon( buf );
    uiRetVal uirv;
    if ( !strm.add(buf).isOK() )
    {
	uirv.set( uiStrings::phrCannotWrite( toUiString(strm.fileName()) ) );
	strm.addErrMsgTo( uirv );
    }
    return uirv;
}


//--------- Array

OD::JSON::Array::Array( bool nodes, ValueSet* p )
    : ValueSet(p)
    , valtype_(nodes ? SubNode : SubArray)
    , valarr_(0)
{
}


OD::JSON::Array::Array( DataType dt, ValueSet* p )
    : ValueSet(p)
    , valtype_(Data)
    , valarr_(new ValArr(dt))
{
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


OD::JSON::ValueSet::size_type OD::JSON::Array::nrElements() const
{
    return valtype_ == Data ? valArr().size() : size();
}


void OD::JSON::Array::addChild( ValueSet* vset )
{
    if ( valtype_ == Data )
	{ pErrMsg("add child to value Array"); return; }
    else if ( (valtype_==SubArray) != vset->isArray() )
	{ pErrMsg("add wrong child type to Array"); return; }

    values_ += new Value( vset );
}


static const char* addarrnonvalstr = "add value to non-value Array";

#define mDefArrayAddVal(inptyp,fn,valtyp) \
void OD::JSON::Array::add( inptyp val ) \
{ \
    if ( valtype_ != Data ) \
	{ pErrMsg(addarrnonvalstr); } \
    else \
	valarr_->fn().add( (valtyp)val ); \
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

void OD::JSON::Array::add( const uiString& val )
{
    BufferString bs;
    val.fillUTF8String( bs );
    add( bs );
}


template<class T>
void OD::JSON::Array::setVals( const TypeSet<T>& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( Number );
    copy( valarr_->vals(), vals );
}


#define mDefArraySetVals( inptyp ) \
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
    for ( auto uistrptr : vals )
    {
	BufferString bs;
	uistrptr->fillUTF8String( bs );
	bss.add( bs );
    }
    set( bss );
}


//--------- Node

OD::JSON::ValueSet::idx_type OD::JSON::Node::indexOf( const char* nm ) const
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

OD::JSON::ValueSet* OD::JSON::Node::gtChildByKey( const char* ky ) const
{
    const idx_type idx = indexOf( ky );
    return idx < 0 ? 0 : gtChildByIdx( idx );
}


OD::JSON::Array* OD::JSON::Node::gtArrayByKey( const char* ky ) const
{
    ValueSet* vs = gtChildByKey( ky );
    if ( !vs )
	return 0;
    else if ( !vs->isArray() )
	{ pErrMsg("Request for child Array which is a Node"); return 0; }

    return static_cast<Array*>( vs );
}


OD::JSON::Node* OD::JSON::Node::gtNodeByKey( const char* ky ) const
{
    ValueSet* vs = gtChildByKey( ky );
    if ( !vs )
	return 0;
    else if ( vs->isArray() )
	{ pErrMsg("Request for child Node which is an Array"); return 0; }

    return static_cast<Node*>( vs );
}


od_int64 OD::JSON::Node::getIntValue( const char* ky ) const
{
    return ValueSet::getIntValue( indexOf(ky) );
}


double OD::JSON::Node::getDoubleValue( const char* ky ) const
{
    return ValueSet::getDoubleValue( indexOf(ky) );
}


BufferString OD::JSON::Node::getStringValue( const char* ky ) const
{
    return ValueSet::getStringValue( indexOf(ky) );
}


void OD::JSON::Node::setChild( const char* ky, ValueSet* vset )
{
    if ( !ky || !*ky )
	{ pErrMsg("Empty key not allowed for children of Node's"); return; }
    set( ky, new KeyedValue( ky, vset ) );
}


void OD::JSON::Node::set( KeyedValue* val )
{
    const idx_type idx = indexOf( val->key_ );
    if ( idx >= 0 )
	delete values_.removeSingle( idx );
    values_ += val;
}



template <class T>
void OD::JSON::Node::setVal( const char* ky, T t )
{
    set( new KeyedValue(ky,t) );
}


#define mDefNodeSetVal(typ) \
void OD::JSON::Node::set( const char* ky, typ val ) { setVal(ky,val); }

mDefNodeSetVal( bool )
mDefNodeSetVal( od_int16 )
mDefNodeSetVal( od_uint16 )
mDefNodeSetVal( od_int32 )
mDefNodeSetVal( od_uint32 )
mDefNodeSetVal( od_int64 )
mDefNodeSetVal( float )
mDefNodeSetVal( double )
mDefNodeSetVal( const char* )


void OD::JSON::Key::set( const char* inp )
{
    setEmpty();
    if ( !inp || !*inp )
	return;

    if ( FixedString(inp).contains('.') )
	set( SeparString(inp,'.') );
    else
	add( inp );
}


void OD::JSON::Key::set( const SeparString& ss )
{
    setEmpty();
    const int sz = ss.size();
    for ( int idx=0; idx<sz; idx++ )
	add( ss[idx] );
}


void OD::JSON::Key::set( const BufferStringSet& bss, int startat )
{
    setEmpty();
    for ( int idx=startat; idx<bss.size(); idx++ )
	add( bss.get(idx) );
}
