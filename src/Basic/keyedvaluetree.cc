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


namespace KeyedValue
{

#define mDefSimpleConstr(ctyp,enumtyp,isint,contmemb,utype) \
    Value( ctyp i ) \
	: type_(enumtyp), isint_(isint) { contents_.contmemb = (utype)i; }

class Value
{
public:

    union Contents  { double dval_; od_int64 ival_; void* ptr_; };

mDefSimpleConstr( bool, Boolean, true, ival_, od_int64 )
mDefSimpleConstr( od_int16, Number, true, ival_, od_int64 )
mDefSimpleConstr( od_uint16, Number, true, ival_, od_int64 )
mDefSimpleConstr( od_int32, Number, true, ival_, od_int64 )
mDefSimpleConstr( od_uint32, Number, true, ival_, od_int64 )
mDefSimpleConstr( od_int64, Number, true, ival_, od_int64 )
mDefSimpleConstr( float, Number, false, dval_, double )
mDefSimpleConstr( double, Number, false, dval_, double )


Value( const char* str )
    : type_(String)
{
    if ( !str )
	str = "";
    const int len = FixedString(str).size();
    char* contstr = new char[ len + 1 ];
    strcpy( contstr, str );
    contents_.ptr_ = contstr;
}


Value( const OD::String& str )
    : Value(str.buf())
{
}

Value( const BufferStringSet& bss )
    : type_(ArrayString)
    , arrsz_(bss.size())
{
    if ( arrsz_ > 0 )
    {
	char** strs = new char* [arrsz_];
	for ( int idx=0; idx<arrsz_; idx++ )
	{
	    const BufferString& bs = bss.get( idx );
	    const int sz = bs.size();
	    strs[idx] = new char [sz];
	    strcpy( strs[idx], bs.buf() );
	}
	contents_.ptr_ = strs;
    }
}

template <class CT>
void initFromContainer( const CT& vals, bool isint )
{
    type_ = ArrayNumber;
    isint_ = isint;
    arrsz_ = (int)vals.size();
    if ( arrsz_ > 0 )
    {
	if ( isint_ )
	{
	    od_int64* arr = new od_int64[arrsz_];
	    for ( int idx=0; idx<arrsz_; idx++ )
		arr[idx] = (od_int64)vals[idx];
	    contents_.ptr_ = arr;
	}
	else
	{
	    double* arr = new double[arrsz_];
	    for ( int idx=0; idx<arrsz_; idx++ )
		arr[idx] = (double)vals[idx];
	    contents_.ptr_ = arr;
	}
    }
}


#define mDefContainerConstr(ct,isint) \
    Value( const ct& vals ) { initFromContainer( vals, isint ); }

mDefContainerConstr(BoolTypeSet,true)

#define mDefContainerConstr4All(clss) \
mDefContainerConstr(clss<od_int16>,true) \
mDefContainerConstr(clss<od_uint16>,true) \
mDefContainerConstr(clss<od_int32>,true) \
mDefContainerConstr(clss<od_uint32>,true) \
mDefContainerConstr(clss<od_int64>,true) \
mDefContainerConstr(clss<float>,false) \
mDefContainerConstr(clss<double>,false)

mDefContainerConstr4All(TypeSet)
mDefContainerConstr4All(Array1D)


~Value()
{
    if ( type_ == String )
	delete [] (char*)contents_.ptr_;
    else if ( type_ == ArrayBoolean )
	delete [] (bool*)contents_.ptr_;
    else if ( type_ == ArrayNumber )
    {
	if ( isint_ )
	    delete [] (od_int64*)contents_.ptr_;
	else
	    delete [] (double*)contents_.ptr_;
    }
    else if ( type_ == ArrayString )
    {
	char** strs = (char**)contents_.ptr_;
	for ( int idx=0; idx<arrsz_; idx++ )
	    delete [] strs[idx];
	delete [] strs;
    }
}

    DataType	type_;
    Contents	contents_;
    bool	isint_		= true;
    int		arrsz_		= 0;

};

} // namespace KeyedValue


void KeyedValue::Key::set( const char* inp )
{
    setEmpty();
    if ( !inp || !*inp )
	return;

    if ( FixedString(inp).contains('.') )
	set( SeparString(inp,'.') );
    else
	add( inp );
}


void KeyedValue::Key::set( const SeparString& ss )
{
    setEmpty();
    const int sz = ss.size();
    for ( int idx=0; idx<sz; idx++ )
	add( ss[idx] );
}


void KeyedValue::Key::set( const BufferStringSet& bss, int startat )
{
    setEmpty();
    for ( int idx=startat; idx<bss.size(); idx++ )
	add( bss.get(idx) );
}


KeyedValue::Node::~Node()
{
    setEmpty();
}


void KeyedValue::Node::setEmpty()
{
    for ( auto it : values_ )
	delete it.second;

    for ( auto it : children_ )
    {
	Node* child = it.second;
	child->setEmpty();
	delete child;
    }

    children_.clear();
    values_.clear();
}


KeyedValue::Node::ChildrenMap::const_iterator KeyedValue::Node::childIter(
		const Node& child ) const
{
    ChildrenMap::const_iterator it = children_.begin();
    for ( ; it!=children_.end(); it++ )
    {
	if ( it->second == &child )
	    break;
    }
    return it;
}


KeyedValue::Node::ChildrenMap::iterator KeyedValue::Node::childIter(
		const Node& child )
{
    ChildrenMap::iterator it = children_.begin();
    for ( ; it!=children_.end(); it++ )
    {
	if ( it->second == &child )
	    break;
    }
    return it;
}


BufferString KeyedValue::Node::name() const
{
    BufferString nm;
    if ( parent_ )
    {
	ChildrenMap::const_iterator it = parent_->childIter( *this );
	nm.set( it->first.c_str() );
    }
    return nm;
}


KeyedValue::Key KeyedValue::Node::key() const
{
    Key ky;
    if ( parent_ )
	parent_->fillKey( *this, ky );
    return ky;
}


void KeyedValue::Node::fillKey( const Node& child, Key& ky ) const
{
    ChildrenMap::const_iterator it = childIter( child );
    if ( it != children_.end() )
	ky.insertAt( new BufferString(it->first.c_str()), 0 );

    if ( parent_ )
	parent_->fillKey( *this, ky );
}


KeyedValue::Value* KeyedValue::Node::findValue( const char* nm ) const
{
    const std::string valnmstr( nm );
    auto vit = values_.find( valnmstr );
    return vit == values_.end() ? 0 : const_cast<Value*>(vit->second);
}


KeyedValue::DataType KeyedValue::Node::getDataType( const Key& ky ) const
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


namespace KeyedValue
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
	str.set( (const char*)valptr->contents_.ptr_ );
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
	const char* valstr = (const char*)valptr->contents_.ptr_;
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
	const char* valstr = (const char*)valptr->contents_.ptr_;
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
	    addNode( child, nm );
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


bool KeyedValue::Node::setValue( const Key& ky, const char* str )
{ return implSetValue( ky, str ); }
bool KeyedValue::Node::setValue( const Key& ky, char* str )
{ return implSetValue( ky, (const char*)str ); }
bool KeyedValue::Node::setValue( const Key& ky, const OD::String& str )
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

} // namespace KeyedValue


KeyedValue::Node* KeyedValue::Node::gtNode( IdxType idx ) const
{
    if ( idx >= 0 )
    {
	for ( auto it : children_ )
	{
	    if ( idx == 0 )
		return const_cast<Node*>( it.second );
	    idx--;
	}
    }

    return 0;
}


KeyedValue::Node* KeyedValue::Node::gtNode( const Key& ky ) const
{
    const int keysz = ky.size();
    if ( keysz < 1 )
	return const_cast<Node*>( this );

    const BufferString& nm = ky.get( 0 );
    const std::string nmstr( nm.str() );
    auto it = children_.find( nmstr );
    if ( it == children_.end() )
	return 0;

    const Node* ret = it->second;
    if ( keysz > 1 )
	ret = ret->gtNode( Key(ky,1) );

    return const_cast<Node*>( ret );
}


void KeyedValue::Node::addNode( Node* node, const char* nm )
{
    if ( !nm || !*nm )
	nm = "ERR:EMPTY_KEY";
    node->parent_ = this;
    children_[nm] = node;
}


void KeyedValue::Node::fillPar( IOPar& iop ) const
{
    // Put '.' for subnode keys
    pErrMsg("Needs impl");
}


void KeyedValue::Node::usePar( const IOPar& iop )
{
    // Scan for '.' to make subnodes
    pErrMsg("Needs impl");
}


#define mJvalInt(jval) ((od_int64)jval.getPayload())
#define mJvalDouble(jval) (jval.toNumber())
#define mJvalString(jval) (jval.toString())
#define mJvalBool(jval) ((bool)jval.getPayload())


void KeyedValue::Node::useJsonValue( Gason::JsonValue& jsonval,
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
		addNode( child, jsonnode->key );
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


void KeyedValue::Node::parseJSon( char* buf, int bufsz, uiRetVal& uirv )
{
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


void KeyedValue::Node::dumpJSon( BufferString& str ) const
{
    str.set( "TODO" );
}


uiRetVal KeyedValue::Tree::readJSon( od_istream& strm )
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


uiRetVal KeyedValue::Tree::writeJSon( od_ostream& strm )
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
