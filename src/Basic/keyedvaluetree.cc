/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert/Arnaud
 * DATE     : April 2018
-*/


#include "keyedvaluetree.h"
#include "od_iostream.h"
#include "separstr.h"
#include "typeset.h"
#include "uistrings.h"
#include <string.h>


namespace KeyedValue
{

#define mDefSimpleConstr(ctyp,enumtyp,isint,contmemb) \
    Value( ctyp i ) \
	: type_(enumtyp), isint_(isint) { contents_.contmemb = i; }

class Value
{
public:

    union Contents  { double dval_; od_int64 ival_; void* ptr_; };

mDefSimpleConstr( od_int16, Number, true, ival_ )
mDefSimpleConstr( od_int32, Number, true, ival_ )
mDefSimpleConstr( od_int64, Number, true, ival_ )
mDefSimpleConstr( float, Number, false, dval_ )
mDefSimpleConstr( double, Number, false, dval_ )


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


Value( const TypeSet<od_int16>& vals )
{ initI( vals ); }
Value( const TypeSet<od_int32>& vals )
{ initI( vals ); }
Value( const TypeSet<od_int64>& vals )
{ initI( vals ); }

template <class IT>
void initI( const TypeSet<IT>& vals )
{
    type_ = ArrayNumber;
    isint_ = true;
    arrsz_ = vals.size();
    if ( arrsz_ > 0 )
    {
	od_int64* arr = new od_int64[arrsz_];
	for ( int idx=0; idx<arrsz_; idx++ )
	    arr[idx] = vals[idx];
	contents_.ptr_ = arr;
    }
}

//TODO Array1D, and float and bool typeset versions


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
    for ( ValueMap::iterator it=values_.begin();
			     it!=values_.end(); it++ )
	delete it->second;

    for ( ChildrenMap::iterator it=children_.begin();
			        it!=children_.end(); it++ )
    {
	Node* child = it->second;
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
	ChildrenMap::iterator it = parent_->childIter( *this );
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
{ // older compilers need some template specializations to be in a namespace

template <class T>
bool Node::getSubNodeValue( const Key& ky, T& val ) const
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
	return getSubNodeValue( ky, str );

    const Value* valptr = findValue( ky.get(0) );
    if ( valptr->type_ == String )
	str.set( (const char*)valptr->contents_.ptr_ );
    else if ( valptr->type_ == Number )
	str.set( valptr->isint_ ? toString(valptr->contents_.ival_)
				: toString(valptr->contents_.dval_) );
    else if ( valptr->type_ == Boolean )
	str.set( valptr->contents_.ival_ ? "True" : "False" );
    else
	{ pErrMsg("Impl string get for this type"); return false; }

    return true;
}


template <class IT>
bool Node::getIValue( const Key& ky, IT& val ) const
{
    if ( ky.size() != 1 )
	return getSubNodeValue( ky, val );

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


template <>
bool Node::getValue( const Key& ky, od_int16& val ) const
{
    return getIValue( ky, val );
}


template <>
bool Node::getValue( const Key& ky, od_int32& val ) const
{
    return getIValue( ky, val );
}


template <>
bool Node::getValue( const Key& ky, od_int64& val ) const
{
    return getIValue( ky, val );
}

//TODO float types, arrays


template <class T>
bool Node::implAddValue( const Key& ky, const T& val )
{
    const int keysz = ky.size();
    if ( keysz < 1 )
	{ return false; }

    const BufferString& nm = ky.get( 0 );
    const std::string nmstr( nm.str() );
    if ( keysz > 1 )
    {
	auto it = children_.find( nmstr );
	if ( it == children_.end() )
	    return false;

	const Key chldky( ky, 1 );
	return it->second->addValue( chldky, val );
    }

    const BufferString& valnm = ky.get( 0 );
    const std::string valnmstr( valnm.str() );
    values_[valnmstr] = new Value( val );
    return true;
}


bool KeyedValue::Node::addValue( const Key& ky, const char* str )
{
    return implAddValue( ky, str );
}


bool KeyedValue::Node::addValue( const Key& ky, const OD::String& str )
{
    return implAddValue( ky, str.buf() );
}


#define mImplAddValue(typ) \
template <> \
bool Node::addValue( const Key& ky, const typ& val ) \
{ \
    return implAddValue( ky, val ); \
}

mImplAddValue( od_int16 )
mImplAddValue( od_int32 )
mImplAddValue( od_int64 )
mImplAddValue( float )
mImplAddValue( double )
mImplAddValue( BufferStringSet )
mImplAddValue( TypeSet<od_int16> )
mImplAddValue( TypeSet<od_int32> )
mImplAddValue( TypeSet<od_int64> )
//TODO Array1D, and float and bool typeset versions

} // namespace KeyedValue


KeyedValue::Node* KeyedValue::Node::gtNode( IdxType idx ) const
{
    if ( idx >= 0 )
    {
	for ( ChildrenMap::const_iterator it = children_.begin();
	      it!=children_.end(); it++ )
	{
	    if ( idx == 0 )
		return const_cast<Node*>( it->second );
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


uiRetVal KeyedValue::Tree::readJSon( od_istream& strm )
{
    // use Gason
    return uiRetVal( mTODONotImplPhrase() );
}


uiRetVal KeyedValue::Tree::writeJSon( od_ostream& strm )
{
    // Implement something that looks reasonably pretty-printed
    return uiRetVal( mTODONotImplPhrase() );
}
