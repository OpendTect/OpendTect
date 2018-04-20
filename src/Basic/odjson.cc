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

#define mDefSimpleConstr(inptyp,enumtyp,contmemb,valtyp) \
    Value( inptyp i ) \
	: type_(enumtyp) { cont_.contmemb = (valtyp)i; }

mDefSimpleConstr( bool, Boolean, bool_, bool )
mDefSimpleConstr( od_int16, Number, val_, NumberType )
mDefSimpleConstr( od_uint16, Number, val_, NumberType )
mDefSimpleConstr( od_int32, Number, val_, NumberType )
mDefSimpleConstr( od_uint32, Number, val_, NumberType )
mDefSimpleConstr( od_int64, Number, val_, NumberType )
mDefSimpleConstr( float, Number, val_, NumberType )
mDefSimpleConstr( double, Number, val_, NumberType )

Value( ValueSet* vset )
    : type_((int)String+1)
{
    cont_.vset_ = vset;
}

Value( const char* cstr )
    : type_((int)String)
{
    if ( !cstr )
	cstr = "";
    const int len = FixedString(cstr).size();
    char* contstr = new char[ len + 1 ];
    strcpy( contstr, cstr );
    cont_.str_ = contstr;
}

virtual ~Value()
{
    if ( isValSet() )
	delete vSet();
    else if ( type_ == (int)String )
	delete [] str();
}

bool isValSet() const
{
    return type_ > String;
}

};

#define mDefSimpleKeyedConstr(ctyp) \
    KeyedValue( const char* ky, ctyp i ) \
	: Value(i), key_(ky) {}

class KeyedValue :public Value
{
public:

    BufferString	key_;

    virtual bool isKeyed() const { return true; }

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


OD::JSON::ValueSet::idx_type OD::JSON::ValueSet::indexOf( const char* nm ) const
{
    for ( auto val : values_ )
    {
	if ( val->key_ == nm )
	    return idx;
    }
    return -1;
}


OD::JSON::ValueSet::Type OD::JSON::ValueSet::valueType( idx_type idx )
{
    Type ret = Data;
    if ( !values_.validIdx(idx) )
	{ pErrMsg("Idx out of range"); return ret; }

    const Value& val = *values_[idx];
    if ( val.isValSet() )
	ret = val.valSet()->isArray() ? SubArray : SubNode;

    return ret;
}


OD::JSON::Tree* OD::JSON::ValueSet::tree()
{
    return parent_ ? parent_->tree() : (Tree*)this; }
}


const OD::JSON::Tree* OD::JSON::ValueSet::tree() const
{
    return parent_ ? parent_->tree() : (const Tree*)this; }
}


OD::JSON::ValueSet& OD::JSON::ValueSet::gtChild( idx_type idx ) const
{
    Value* val = values_[idx];
    if ( !val->isValSet() )
	{ pErrMsg("Value at idx is not ValSet - crash follows"); }
    return *const_cast<ValueSet*>( values_[idx]->vSet() );
}


OD::JSON::Array& OD::JSON::ValueSet::gtArray( idx_type idx ) const
{
    ValueSet& vset = gtChild( idx );
    if ( !vset.isArray() )
	{ pErrMsg("ValueSet at idx is not Array - crash follows"); }
    return static_cast<Array&>( vset );
}


OD::JSON::Node& OD::JSON::ValueSet::gtNode( idx_type idx ) const
{
    ValueSet& vset = gtChild( idx );
    if ( vset.isArray() )
	{ pErrMsg("ValueSet at idx is not Node - crash follows"); }
    return static_cast<Array&>( vst );
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
}


//--------- Array

OD::JSON::Array::Array( bool nodes, ValueSet* p )
    : ValueSet(p)
    , type_(nodes ? Nodes : Arrays)
    , valarr_(0)
{
}


OD::JSON::Array::Array( DataType dt, ValueSet* p )
    : ValueSet(p)
    , type_(Values)
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


OD::JSON::Container::SzType OD::JSON::Array::nrElements() const
{
    return type_ == Values ? valArr().size() : size();
}


void OD::JSON::Array::addChild( ValueSet* vset )
{
    if ( valtype_ == Data )
	{ pErrMsg("add child to value Array"); return; }
    else if ( valtype_ == SubArray != vset->isArray() )
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
void OD::JSON::Array::setValsImpl( const TypeSet<T>& vals )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_; valarr_ = new ValArr( Number );
    valarr_->vals().copy( vals );
}


#define mDefArraySetVals( inptyp ) \
void OD::JSON::Array::set( const TypeSet<inptyp>& vals ) { setVals( vals ); }

mDefArraySetVals( od_int16, vals )
mDefArraySetVals( od_uint16, vals )
mDefArraySetVals( od_int32, vals )
mDefArraySetVals( od_uint32, vals )
mDefArraySetVals( od_int64, vals )
mDefArraySetVals( float, vals )
mDefArraySetVals( double, vals )


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

OD::JSON::ValueSet* OD::JSON::Node::gtChild( const char* ky ) const
{
    const idx = indexOf( ky );
    if ( idx < 0 )
	return 0;

    Value* val  = values_[idx];
    if ( !val->isValSet() )
	{ pErrMsg("Request for child set which is a plain value"); return 0; }

    return const_cast<ValueSet*>( val->valSet() );
}


OD::JSON::Array* OD::JSON::Node::gtArray( const char* ky ) const
{
    ValueSet* vs = gtChild( ky );
    if ( !vs )
	return 0;
    else if ( !vs->isArray() )
	{ pErrMsg("Request for child Array which is a Node"); return 0; }

    return static_cast<Array*>( vs );
}


OD::JSON::Node* OD::JSON::Node::gtNode( const char* ky ) const
{
    ValueSet* vs = gtChild( ky );
    if ( !vs )
	return 0;
    else if ( vs->isArray() )
	{ pErrMsg("Request for child Node which is an Array"); return 0; }

    return static_cast<Node*>( vs );
}


void OD::JSON::Node::setChild( const char* ky, ValueSet* vset )
{
    if ( !ky || !*ky )
	{ pErrMsg("Empty key not allowed for children of Node's"); return; }
    set( ky, new KeyedValue( ky, vset ) );
}


void OD::JSON::Node::set( KeyedValue* val )
{
    const int idx = indexOf( val->key_ );
    if ( idx < 0 )
	values_ += val;
    else
	delete values_.replace( idx, val );
}



template <class T>
void OD::JSON::Node::setVal( const char* ky, T t )
{
    set( new KeyedValue(ky,t) );
}


#define mDefNodeSetVal(typ) \
void OD::JSON::Node::set( const char* ky, T val ) { setVal(ky,val); }

mDefNodeSetVal( bool )
mDefNodeSetVal( od_int16 )
mDefNodeSetVal( od_uint16 )
mDefNodeSetVal( od_int32 )
mDefNodeSetVal( od_uint32 )
mDefNodeSetVal( od_int64 )
mDefNodeSetVal( float )
mDefNodeSetVal( double )
mDefNodeSetVal( const char* )


void OD::JSON::Node::fillPar( IOPar& iop ) const
{
    // Put '.' for subnode keys
    pErrMsg("TODO: Needs impl");
}


void OD::JSON::Node::usePar( const IOPar& iop )
{
    // Scan for '.' to make subnodes
    pErrMsg("TODO: Needs impl");
}


#define mJvalDouble(jval) (jval.toNumber())
#define mJvalString(jval) (jval.toString())


void OD::JSON::Node::useJsonValue( Gason::JsonValue& jsonval,
				     const char* jsonky )
{
    const Gason::JsonTag tag = jsonval.getTag();
    const Key ky( jsonky );

    switch ( tag )
    {
	case Gason::JSON_NUMBER:
	{
	    if ( jsonval.isDouble() )
		setValue( ky, mJvalDouble(jsonval) );
	    else
		setValue( ky, mJvalInt(jsonval) );
	} break;

	case Gason::JSON_STRING:
	{
	    setValue( ky, mJvalString(jsonval) );
	} break;

	case Gason::JSON_ARRAY:
	{
	    Gason::JsonTag valtag = Gason::JSON_NULL;
	    for ( auto it : jsonval )
	    {
		const Gason::JsonValue& arrjval = it->value;

		if ( valtag == Gason::JSON_NULL )
		    valtag = arrjval.getTag();

		switch ( valtag )
		{
		    case Gason::JSON_NUMBER:
		    {
			if ( arrjval.isDouble() )
			    fps += mJvalDouble( arrjval );
			else
			    ints += mJvalInt( arrjval );
		    } break;
		    case Gason::JSON_STRING:
		    {
			strs.add( mJvalString(arrjval) );
		    } break;
		    case Gason::JSON_TRUE:
		    case Gason::JSON_FALSE:
		    {
			bools.add( mJvalBool(arrjval) );
		    } break;
		    default:
			break;
		}
	    }
	    if ( !strs.isEmpty() )
		setValue( ky, strs );
	    if ( !bools.isEmpty() )
		setValue( ky, bools );
	    if ( !ints.isEmpty() )
		setValue( ky, ints );
	    if ( !fps.isEmpty() )
		setValue( ky, fps );
	} break;

	case Gason::JSON_OBJECT:
	{
	    Gason::JsonNode* jsonnode = jsonval.toNode();
	    if ( !jsonnode )
		{ pErrMsg("Huh"); }
	    else
	    {
		Node* child = new Node( this );
		addChld( jsonnode->key, child );
		child->useJsonValue( jsonnode->value, jsonnode->key );
	    }
	} break;

	case Gason::JSON_TRUE:
	case Gason::JSON_FALSE:
	{
	    setValue( ky, (bool)jsonval.getPayload() );
	} break;

	case Gason::JSON_NULL:
	{
	    // simply ignore ... right?
	} break;
    }
}


void OD::JSON::Node::parseJSon( char* buf, int bufsz, uiRetVal& uirv )
{
    uirv.setOK();
    if ( !buf || bufsz < 1 )
	{ uirv.set( uiStrings::phrInternalErr("No data to parse (JSON)") );
	    return; }

    Gason::JsonAllocator allocator;
    Gason::JsonValue root; char* endptr;
    int status = Gason::jsonParse( buf, &endptr, &root, allocator );
    if ( status != Gason::JSON_OK )
    {
	//TODO figure out the errmsg for this status
	uirv.set( tr("Could not parse JSON data") );
	return;
    }

    for ( auto it : root )
	useJsonValue( it->value, it->key );
}


void OD::JSON::Node::dumpJSon( BufferString& str ) const
{
    str.set( "TODO" );
}


uiRetVal OD::JSON::Tree::read( od_istream& strm )
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


uiRetVal OD::JSON::Tree::write( od_ostream& strm )
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
