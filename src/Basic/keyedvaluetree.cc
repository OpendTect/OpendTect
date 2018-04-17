/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert/Arnaud
 * DATE     : April 2018
-*/

#include "keyedvaluetree.h"
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
    Value( ctyp i ) \
	: type_(enumtyp) { contents_.contmemb = (utype)i; }

class Value : public NamedObj
{
public:

    union Contents
    {
	Contents() { ival_ = 0; }
	double dval_; od_int64 ival_; char* str_;
	Array* arr_;
    };

mDefSimpleConstr( bool, Boolean, ival_, od_int64 )
mDefSimpleConstr( od_int16, Int, ival_, od_int64 )
mDefSimpleConstr( od_uint16, Int, ival_, od_int64 )
mDefSimpleConstr( od_int32, Int, ival_, od_int64 )
mDefSimpleConstr( od_uint32, Int, ival_, od_int64 )
mDefSimpleConstr( od_int64, Int, ival_, od_int64 )
mDefSimpleConstr( float, Double, dval_, double )
mDefSimpleConstr( double, Double, dval_, double )

Value( Array* arr )
    : type_((int)String+1)
{
    contents_.arr_ = arr_;
}

Value( const char* str )
    : type_((int)String)
{
    if ( !str )
	str = "";
    const int len = FixedString(str).size();
    char* contstr = new char[ len + 1 ];
    strcpy( contstr, str );
    contents_.str_ = contstr;
}

Value( const OD::String& str )
    : Value(str.str())
{
}

~Value()
{
    if ( isArr() )
	delete ((Array*)contents_.arr_);
    else if ( type_ == (int)String )
	delete [] (char*)contents_.str_;
}

bool isArr() const
{
    return type_ > String;
}

    int		type_;
    Contents	contents_;

};

} // namespace JSON

} // namespace OD


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


void OD::JSON::Container::setEmpty()
{
    for ( int idx=0; idx<containers_.size(); idx++ )
	containers_[idx]->setEmpty();
    deepErase( containers_ );
}


OD::JSON::Array::Array( bool nodes, const char* nm, Container* p )
    : Container(nm,p)
    , type_(nodes ? Nodes : Arrays)
    , valarr_(0)
{
}


OD::JSON::Array::Array( DataType dt, const char* nm, Container* p )
    : Container(nm,p)
    , type_(Values)
    , valarr_(new ValArr(dt))
{
}


OD::JSON::Array::~Array()
{
    delete valarr_;
    deepErase( containers_ );
}


void OD::JSON::Array::setEmpty()
{
    if ( valarr_ )
	valarr_->setEmpty();
    Container::setEmpty();
}


OD::JSON::Container::SzType OD::JSON::Array::nrChildren() const
{
    return type_ == Values ? 0 : containers_.size();
}


OD::JSON::Container::SzType OD::JSON::Array::nrValues() const
{
    return type_ == Values ? valArr().size() : 0;
}


OD::JSON::Container::SzType OD::JSON::Array::size() const
{
    return type_ == Values ? valArr().size() : containers_.size();
}


void OD::JSON::Array::add( Node* node )
{
    if ( type_ != Nodes )
	{ pErrMsg("Type not Nodes"); delete node; }
    else
	containers_ += node;
}


void OD::JSON::Array::add( Array* arr )
{
    if ( type_ != Arrays )
	{ pErrMsg("Type not Arrays"); delete arr; }
    else
	containers_ += arr;
}


OD::JSON::Node::~Node()
{
    deepErase( values_ );
}


void OD::JSON::Node::setEmpty()
{
    deepErase( values_ );
    Container::setEmpty();
}


OD::JSON::Key OD::JSON::Container::key() const
{
    Key ky;
    if ( parent_ )
	ky = parent_->key();
    ky.add( name() );
    return ky;
}


OD::JSON::DataType OD::JSON::Node::getDataType( const Key& ky ) const
{
    const DataType defdt = Boolean; // whatever
    if ( ky.size() < 1 )
	{ return defdt; }
    if ( ky.size() > 1 )
    {
	const std::string nmstr( ky.get(0).str() );
	auto it = children_.find( nmstr );
	if ( it == children_.end() )
	    return defdt;
	const Key chldky( ky, 1 );
	return it->second->getDataType( chldky );
    }

    const Value* valptr = findValue( ky.get(0) );
    return valptr ? valptr->type_ : defdt;
}


bool OD::JSON::Node::isArray( IdxType idx ) const
{
    for ( auto it : values_ )
    {
	if ( idx < 0 )
	    break;
	else if ( idx == 0 )
	    return it.second->isArray();
	idx--;
    }
    return false;
}


namespace OD
{
namespace JSON
{ // older compilers need template specializations to be in the namespace

template <class T>
bool Node::getChildValue( const Key& ky, T& val ) const
{
    if ( ky.size() < 1 )
	{ return false; }

    const std::string nmstr( ky.get(0).str() );
    auto it = children_.find( nmstr );
    if ( it == children_.end() )
	return false;

    const Key chldky( ky, 1 );
    return it->second->getValue( chldky, val );
}


template <>
bool Node::getValue( const Key& ky, BufferString& str ) const
{
    if ( ky.size() != 1 )
	return getChildValue( ky, str );

    const Value* valptr = findValue( ky.get(0) );
    if ( valptr->type_ == String )
	str.set( (const char*)valptr->contents_.str_ );
    else if ( valptr->type_ == Number )
	str.set( valptr->isint_ ? toString(valptr->contents_.ival_)
				: toString(valptr->contents_.dval_) );
    else if ( valptr->type_ == Boolean )
	str.set( valptr->contents_.ival_ ? "True" : "False" );
    else
	{ pErrMsg("Don't try to get arrays in one string"); return false; }

    return true;
}


template <class IT>
bool Node::getIValue( const Key& ky, IT& val ) const
{
    if ( ky.size() != 1 )
	return getChildValue( ky, val );

    const Value* valptr = findValue( ky.get(0) );
    if ( !valptr )
	return false;

    if ( isArray(valptr->type_) )
	{ pErrMsg("Type is array"); return false; }
    if ( valptr->type_ == Boolean )
	{ pErrMsg("Type is boolean"); return false; }

    if ( valptr->type_ == Number )
    {
	if ( valptr->isint_ )
	    val = (IT)valptr->contents_.ival_;
	else
	    val = mRounded( IT, valptr->contents_.dval_ );
    }
    else if ( valptr->type_ == String )
    {
	const char* valstr = (const char*)valptr->contents_.str_;
	if ( !isNumberString(valstr) )
	    { pErrMsg("Not a number string"); return false; }
	const double dval = toDouble( valstr );
	val = mRounded( IT, dval );
    }
    else
	{ pErrMsg("Huh"); return false; }

    return true;
}


template <class FT>
bool Node::getFValue( const Key& ky, FT& val ) const
{
    if ( ky.size() != 1 )
	return getChildValue( ky, val );

    const Value* valptr = findValue( ky.get(0) );
    if ( !valptr )
	return false;

    if ( isArray(valptr->type_) )
	{ pErrMsg("Type is array"); return false; }
    if ( valptr->type_ == Boolean )
	{ pErrMsg("Type is boolean"); return false; }

    if ( valptr->type_ == Number )
    {
	if ( valptr->isint_ )
	    val = (FT)valptr->contents_.ival_;
	else
	    val = (FT)valptr->contents_.dval_;
    }
    else if ( valptr->type_ == String )
    {
	const char* valstr = (const char*)valptr->contents_.str_;
	if ( !isNumberString(valstr) )
	    { pErrMsg("Not a number string"); return false; }
	val = (FT)toDouble( valstr );
    }
    else
	{ pErrMsg("Huh"); return false; }

    return true;
}


#define mImplGetValue(typ,fn) \
template <> \
bool Node::getValue( const Key& ky, typ& val ) const \
{ \
    return fn( ky, val ); \
}

mImplGetValue(od_int16,getIValue)
mImplGetValue(od_uint16,getIValue)
mImplGetValue(od_int32,getIValue)
mImplGetValue(od_uint32,getIValue)
mImplGetValue(od_int64,getIValue)
mImplGetValue(float,getFValue)
mImplGetValue(double,getFValue)

template <>
bool Node::getValue( const Key& ky, bool& val ) const
{
    od_int16 i = 0;
    if ( getValue(ky,i) )
	val = (bool)i;
    else
	return false;
    return true;
}


//TODO getValue for TypeSet and Array1D


template <class T>
bool Node::implSetValue( const Key& ky, const T& tval )
{
    const int keysz = ky.size();
    if ( keysz < 1 )
	{ return false; }

    const BufferString& nm = ky.get( 0 );
    const std::string nmstr( nm.str() );
    if ( keysz > 1 )
    {
	auto chit = children_.find( nmstr );
	Node* child = chit == children_.end() ? 0 : chit->second;
	if ( !child )
	{
	    child = new Node( this );
	    addChld( nm, child );
	}

	const Key chldky( ky, 1 );
	return child->implSetValue( chldky, tval );
    }

    const BufferString& valnm = ky.get( 0 );
    const std::string valnmstr( valnm.str() );

    auto vit = values_.find( valnmstr );
    if ( vit != values_.end() )
	delete vit->second;

    values_[valnmstr] = new Value( tval );
    return true;
}


bool OD::JSON::Node::setValue( const Key& ky, const char* str )
{ return implSetValue( ky, str ); }
bool OD::JSON::Node::setValue( const Key& ky, char* str )
{ return implSetValue( ky, (const char*)str ); }
bool OD::JSON::Node::setValue( const Key& ky, const OD::String& str )
{ return implSetValue( ky, str.str() ); }


#define mImplAddValue(typ) \
template <> \
bool Node::setValue( const Key& ky, const typ& val ) \
{ \
    return implSetValue( ky, val ); \
}

mImplAddValue( bool )
mImplAddValue( od_int16 )
mImplAddValue( od_int32 )
mImplAddValue( od_int64 )
mImplAddValue( float )
mImplAddValue( double )
mImplAddValue( BufferStringSet )
mImplAddValue( BoolTypeSet )

#define mImplContainerAddValue(clss) \
mImplAddValue(clss<od_int16>) \
mImplAddValue(clss<od_uint16>) \
mImplAddValue(clss<od_int32>) \
mImplAddValue(clss<od_uint32>) \
mImplAddValue(clss<od_int64>) \
mImplAddValue(clss<float>) \
mImplAddValue(clss<double>)

mImplContainerAddValue(TypeSet)
mImplContainerAddValue(Array1D)

} // namespace JSON
} // namespace OD


OD::JSON::Node::IdxType OD::JSON::Node::valueIdx( const char* nm ) const
{
    for ( int idx=values_.size()-1; idx>=0; idx-- )
    {
	Value* valptr = values_[idx];
	if ( values_[idx]->name() == nm )
	    return idx;
    }
    return -1;
}


OD::JSON::Value* OD::JSON::Node::gtVal( IdxType idx ) const
{
    return values_.validIdx(idx) ? values_[idx] : 0;
}


OD::JSON::Value* OD::JSON::Node::gtVal( const Key& ky ) const
{
    const int keysz = ky.size();
    if ( keysz < 1 )
	return 0;

    int idxof = idxOf( ky.get(0) );
    if ( idxof < 0 )
	return 0;

    const Node* ret = children_[idxof];
    if ( keysz > 1 )
	ret = ret->gtChld( Key(ky,1) );

    return const_cast<Node*>( ret );
}


OD::JSON::Node::IdxType OD::JSON::Node::childIdx( const char* nm )
{
    for ( int idx=children_.size()-1; idx>=0; idx-- )
    {
	Node* child = children_[idx];
	if ( child->name() == nm )
	    return idx;
    }
    return -1;
}


OD::JSON::Node* OD::JSON::Node::gtChld( IdxType idx ) const
{
    return children_.validIdx(idx) ? children_[idx] : 0;
}


OD::JSON::Node* OD::JSON::Node::gtChld( const Key& ky ) const
{
    const int keysz = ky.size();
    if ( keysz < 1 )
	return const_cast<Node*>( this );

    int idxof = idxOf( ky.get(0) );
    if ( idxof < 0 )
	return 0;

    const Node* ret = children_[idxof];
    if ( keysz > 1 )
	ret = ret->gtChld( Key(ky,1) );

    return const_cast<Node*>( ret );
}


void OD::JSON::Node::setChild( const Key& ky, Node* node )
{
    const int keysz = ky.size();
    if ( keysz < 1 )
    {
	if ( !parent_ )
	    delete node;
	else
	    parent_->stChild( name(), node );
    }

    const BufferString& nm = ky.get( 0 );
    int idxof = idxOf( nm );
    if ( idxof >= 0 )
    {
	if ( keysz > 1 )
	    children_[idxof]->setChild( Key(ky,1), node );
	else
	{
	    Node* oldchild = 0;
	    if ( !node )
		oldchild = children_.removeSingle( idxof );
	    else
		oldchild = children_.replace( idxof, node );
	    delete oldchild;
	}
	return;
    }

    Node* newchild = new Node( nm, this );
    children_ += newchild;
    if ( keysz > 1 )
	newchild->setChild( Key(ky,1), node );
}


void OD::JSON::Node::addChld( const char* nm, Node* node )
{
    if ( !nm || !*nm )
	nm = "ERR:EMPTY_KEY";
    node->parent_ = this;
    children_[nm] = node;
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
	    BufferStringSet strs; TypeSet<od_int64> ints;
	    TypeSet<double> doubles; BoolTypeSet bools;
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
			    doubles += mJvalDouble( arrjval );
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
	    if ( !doubles.isEmpty() )
		setValue( ky, doubles );
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
