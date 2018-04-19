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

#define mDefSimpleConstr(ctyp,enumtyp,contmemb,utype) \
    Value( const char* ky, ctyp i ) \
	: key_(ky), type_(enumtyp) { cont_.contmemb = (utype)i; }

class Value
{
public:

    union Contents
    {
	Contents() { ival_ = 0; }
	FPType fpval_; IntType ival_; char* str_;
	ValueSet* vset_;
    };

const char* key() const { return name().str(); }
bool hasKey( const char* nm ) const { return name() == nm; }

mDefSimpleConstr( bool, Boolean, ival_, IntType )
mDefSimpleConstr( IntType, Int, ival_, IntType )
mDefSimpleConstr( FPType, Double, fpval_, FPType )

Value( const char* ky, ValueSet* vset )
    : key_(ky)
    , type_((int)String+1)
{
    cont_.vset_ = vset;
}

Value( const char* ky, const char* cstr )
    : key_(ky)
    , type_((int)String)
{
    if ( !cstr )
	cstr = "";
    const int len = FixedString(cstr).size();
    char* contstr = new char[ len + 1 ];
    strcpy( contstr, cstr );
    cont_.str_ = contstr;
}

~Value()
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

ValueSet* vSet()	{ return cont_.vset_; }
const ValueSet* vSet()	{ return cont_.vset_; }
IntType& iVal()		{ return cont_.ival_; }
IntType iVal() const	{ return cont_.ival_; }
FPType& fpVal()		{ return cont_.fpval_; }
FPType fpVal() const	{ return cont_.fpval_; }
char* str()		{ return cont_.str_; }
const char* str() const	{ return cont_.str_; }

    Contents		cont_;
    BufferString	key_;
    int			type_;

};

} // namespace JSON

} // namespace OD



OD::JSON::ValArr::ValArr( DataType typ )
    : type_(typ)
{
    switch ( type_ )
    {
	case Boolean:	set_ = new BSet;	break;
	case Int:	set_ = new ISet;	break;
	case Double:	set_ = new DSet;	break;
	default:	{ pErrMsg("Unknown type"); type_ = String; }
	case String:	set_ = new SSet;	break;
    }
}


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


OD::JSON::ValueSet* OD::JSON::ValueSet::gtChild( const char* ky ) const
{
    const idx = indexOf( ky );
    if ( idx < 0 )
	return 0;

    Value* val  = values_[idx];
    if ( !val->isValSet() )
	{ pErrMsg("Request for child set which is a plain value"); return 0; }

    return const_cast<ValueSet*>( val->valSet() );
}


OD::JSON::Array* OD::JSON::ValueSet::gtArray( const char* ky ) const
{
    ValueSet* vs = gtChild( ky );
    if ( !vs )
	return 0;
    else if ( !vs->isArray() )
	{ pErrMsg("Request for child Array which is a Node"); return 0; }

    return static_cast<Array*>( vs );
}


OD::JSON::Node* OD::JSON::ValueSet::gtNode( const char* ky ) const
{
    ValueSet* vs = gtChild( ky );
    if ( !vs )
	return 0;
    else if ( vs->isArray() )
	{ pErrMsg("Request for child Node which is an Array"); return 0; }

    return static_cast<Node*>( vs );
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
    return static_cast<Array&>( vst );
}


OD::JSON::Node& OD::JSON::ValueSet::gtNode( idx_type idx ) const
{
    ValueSet& vset = gtChild( idx );
    if ( vset.isArray() )
	{ pErrMsg("ValueSet at idx is not Node - crash follows"); }
    return static_cast<Array&>( vst );
}


BufferString OD::JSON::ValueSet::getStringValue( idx_type idx ) const
{
    BufferString ret;
    if ( !values_.validIdx(idx) )
	return ret;

    const Value* val = values_[idx];
    if ( val->isValSet() )
	{ pErrMsg("ValueSet at idx is not plain data"); return ret; }

    switch ( (DataType)val->type_ )
    {
	case Boolean:	ret.set( val->iVal() ? "true" : "false" );  break;
	case Int:	ret.set( val->iVal() );  break;
	case FP:	ret.set( val->fpVal() );  break;
	default: { pErrMsg("Huh"); }
	case String:	ret.set( val->str() );  break;
    }
}


void OD::JSON::ValueSet::addChild( const char* ky, ValueSet* vset )
{
    if ( !vset )
	{ pErrMsg("Null ValueSet added"); return; }

    values_ += new Value( ky, vset );
}


void OD::JSON::ValueSet::add( const char* ky, bool yn )
{
    values_ += new Value( ky, yn );
}


void OD::JSON::ValueSet::add( const char* ky, IntType ival )
{
    values_ += new Value( ky, ival );
}


void OD::JSON::ValueSet::add( const char* ky, FPType fpval )
{
    values_ += new Value( ky, fpval );
}


void OD::JSON::ValueSet::add( const char* ky, const char* str )
{
    values_ += new Value( ky, str );
}


void OD::JSON::ValueSet::add( const char* ky, const OD::String& str )
{
    values_ += new Value( ky, str.str() );
}


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


void OD::JSON::Array::addChild( const char* ky, ValueSet* vset )
{
    if ( valtype_ == Data )
	{ pErrMsg("add child to value Array"); return; }
    else if ( valtype_ == SubNode != vset->isNode() )
	{ pErrMsg("add wrong child type to Array"); return; }

    ValueSet::addChild( ky, vset );
}


void OD::JSON::Node::addChild( const char* ky, ValueSet* vset )
{
    if ( !ky || !*ky )
	{ pErrMsg("Empty key not allowed for Node's"); return; }
    ValueSet::addChild( ky, vset );
}


void OD::JSON::Node::fillPar( IOPar& iop ) const
{
    // Put '.' for subnode keys
    pErrMsg("Needs impl");
}


void OD::JSON::Node::usePar( const IOPar& iop )
{
    // Scan for '.' to make subnodes
    pErrMsg("Needs impl");
}


#define mJvalInt(jval) ((od_int64)jval.getPayload())
#define mJvalDouble(jval) (jval.toNumber())
#define mJvalString(jval) (jval.toString())
#define mJvalBool(jval) ((bool)jval.getPayload())


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
	    BufferStringSet strs; TypeSet<IntType> ints;
	    TypeSet<FPType> fps; BoolTypeSet bools;
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
