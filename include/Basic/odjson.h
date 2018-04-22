#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2018
________________________________________________________________________

*/

#include "bufstringset.h"
#include "uistringset.h"
#include "typeset.h"
#include "od_iosfwd.h"

class SeparString;
namespace Gason { struct JsonNode; }


namespace OD
{

namespace JSON
{

class Array;
class Node;
class Value;
class KeyedValue;

/*! data types you can find in a JSON file */

enum DataType
{
    Boolean, Number, String
};

typedef double NumberType;


/*! holds 'flat' value sets of each of the DataType's */

mExpClass(Basic) ValArr
{
public:

    typedef BoolTypeSet::size_type	size_type;
    typedef size_type			idx_type;
    typedef BoolTypeSet			BSet;
    typedef TypeSet<NumberType>		NSet;
    typedef BufferStringSet		SSet;

			ValArr(DataType);
			~ValArr()		{ delete set_; }
    DataType		dataType() const	{ return type_; }

    size_type		size() const
			{ return (size_type)set_->nrItems(); }
    bool		validIdx( idx_type idx ) const
			{ return set_->validIdx(idx); }
    bool		isEmpty() const		{ return set_->isEmpty(); }
    void		setEmpty()		{ set_->setEmpty(); }

    OD::Set&		odSet()			{ return *set_; }
    const OD::Set&	odSet() const		{ return *set_; }
    BSet&		bools()			{ return *((BSet*)set_); }
    const BSet&		bools() const		{ return *((BSet*)set_); }
    NSet&		vals()			{ return *((NSet*)set_); }
    const NSet&		vals() const		{ return *((NSet*)set_); }
    SSet&		strings()		{ return *((SSet*)set_); }
    const SSet&		strings() const		{ return *((SSet*)set_); }

    void		dumpJSon(BufferString&) const;

protected:

    DataType		type_;
    OD::Set*		set_;

};


/*!\brief holds Simple values, Node's, and/or Array's. Is base class for either Array or Node. */

mExpClass(Basic) ValueSet
{ mODTextTranslationClass(OD::JSON::ValueSet)
public:

    typedef ValArr::size_type	size_type;
    typedef size_type		idx_type;
    enum ValueType		{ Data, SubArray, SubNode };
    typedef Gason::JsonNode	GasonNode;

    virtual		~ValueSet()			{ setEmpty(); }
    virtual bool	isArray() const			 = 0;
    inline Array&	asArray();
    inline const Array&	asArray() const;
    inline Node&	asNode();
    inline const Node&	asNode() const;

    virtual size_type	size() const
			{ return (size_type)values_.size(); }
    virtual bool	isEmpty() const
			{ return values_.isEmpty(); }
    virtual void	setEmpty();

    virtual ValueType	valueType(idx_type) const;
    inline bool		isPlainData( idx_type i ) const
			{ return valueType(i) == Data; }
    inline bool		isArrayChild( idx_type i ) const
			{ return valueType(i) == SubArray; }
    inline bool		isNodeChild( idx_type i ) const
			{ return valueType(i) == SubNode; }

    bool		isTop() const		{ return !parent_; }
    ValueSet*		top();
    const ValueSet*	top() const;

#   define		mMkGetFns(typ,getfn,implfn) \
    inline typ&		getfn( idx_type i )		{ return *implfn(i); } \
    inline const typ&	getfn( idx_type i ) const	{ return *implfn(i); }
    mMkGetFns(ValueSet,	child, gtChildByIdx )
    mMkGetFns(Array,	array, gtArrayByIdx )
    mMkGetFns(Node,	node, gtNodeByIdx )
#   undef		mMkSubFn

    od_int64		getIntValue(idx_type) const;
    double		getDoubleValue(idx_type) const;
    BufferString	getStringValue(idx_type) const;

    static ValueSet*	parseJSon(char* buf,int bufsz,uiRetVal&);
    void		dumpJSon(BufferString&) const;

    uiRetVal		read(od_istream&);
    uiRetVal		write(od_ostream&);

protected:

			ValueSet( ValueSet* p )
			    : parent_(p)	{}

    ValueSet*		parent_;
    ObjectSet<Value>	values_;

    void		setParent( ValueSet* p )	{ parent_ = p; }

    ValueSet*		gtChildByIdx(idx_type) const;
    Array*		gtArrayByIdx(idx_type) const;
    Node*		gtNodeByIdx(idx_type) const;

    void		use(const GasonNode&);

    friend class	Array;
    friend class	Node;

};


/*!\brief ValueSet holding simple data arrays or other ValueSet's.

  If it holds plain data (valType()==Data), then you can only add
  plain values or set all at once. Otherwise, you can only add ValueSet's of the same type (either Array or Node).
 */

mExpClass(Basic) Array : public ValueSet
{
public:

			Array(bool nodes,ValueSet* p=0);
			Array(DataType,ValueSet* p=0);
			~Array();
    virtual bool	isArray() const		{ return true; }
    virtual void	setEmpty();

    virtual ValueType	valueType(idx_type) const { return valtype_; }
    ValueType		valType() const		{ return valtype_; }
    virtual size_type	size() const;

			// Only available if valType() == Data
    inline ValArr&	valArr()		{ return *valarr_; }
    inline const ValArr& valArr() const		{ return *valarr_; }

    Array*		add(Array*);
    Node*		add(Node*);

			// only usable if valType() == Data
    Array&		add(bool);
    Array&		add(od_int16);
    Array&		add(od_uint16);
    Array&		add(od_int32);
    Array&		add(od_uint32);
    Array&		add(od_int64);
    Array&		add(float);
    Array&		add(double);
    Array&		add(const char*);
    Array&		add( const OD::String& odstr )
			{ return add( odstr.str() ); }
    Array&		add(const uiString&);

			// also, only usable if valType() == Data
    void		set(const BoolTypeSet&);
    void		set(const TypeSet<od_int16>&);
    void		set(const TypeSet<od_uint16>&);
    void		set(const TypeSet<od_int32>&);
    void		set(const TypeSet<od_uint32>&);
    void		set(const TypeSet<od_int64>&);
    void		set(const TypeSet<float>&);
    void		set(const TypeSet<double>&);
    void		set(const BufferStringSet&);
    void		set(const uiStringSet&);

protected:

    ValueType		valtype_;
    ValArr*		valarr_;

    template <class T>
    void		setVals(const TypeSet<T>&);
    void		addVS(ValueSet*);

    friend class	ValueSet;

};


/*!\brief ValueSet holding key-value pairs and other ValueSets */

mExpClass(Basic) Node : public ValueSet
{
public:

			Node( ValueSet* p=0 )
			    : ValueSet(p)	{}
    virtual bool	isArray() const		{ return false; }

    idx_type		indexOf(const char*) const;
    bool		isPresent( const char* ky ) const
						{ return indexOf(ky) >= 0; }

#   define		mMkGetFn(typ,getfn,implfn) \
    inline typ*		getfn( const char* ky )		{ return implfn(ky); } \
    inline const typ*	getfn( const char* ky ) const	{ return implfn(ky); }
    mMkGetFn(ValueSet,	getChild, gtChildByKey )
    mMkGetFn(Array,	getArray, gtArrayByKey )
    mMkGetFn(Node,	getNode, gtNodeByKey )
#   undef		mMkGetFn

    od_int64		getIntValue(const char*) const;
    double		getDoubleValue(const char*) const;
    BufferString	getStringValue(const char*) const;

    Array*		set(const char* ky,Array*);
    Node*		set(const char* ky,Node*);

    void		set(const char* ky,bool);
    void		set(const char* ky,od_int16);
    void		set(const char* ky,od_uint16);
    void		set(const char* ky,od_int32);
    void		set(const char* ky,od_uint32);
    void		set(const char* ky,od_int64);
    void		set(const char* ky,float);
    void		set(const char* ky,double);
    void		set(const char* ky,const char*);
    void		set( const char* ky, const OD::String& str )
			{ set( ky, str.str() ); }

    void		remove(const char*);

protected:

    ValueSet*		gtChildByKey(const char*) const;
    Array*		gtArrayByKey(const char*) const;
    Node*		gtNodeByKey(const char*) const;

    void		set(KeyedValue*);
    void		setVS(const char*,ValueSet*);
    template <class T>
    void		setVal(const char*,T);

    friend class	ValueSet;

};


/*!\brief The key to any value in a JSON Node/Tree

  Full keys can and will most often be multi-level. You can specify:
  * Create it as the BufferStringSet it is
  * From a string where the keys are separated by dots ('.')
  * A SeparString (hence also FileMultiString)

  You can spacify indexes in arrays using the intuitive [] subscript.

  GeoJSon example of fullKey:
  features[1].geometry.coordinates[0][3][1]	(NumberType - Y value)
  features[1].geometry.coordinates[0][3]	(ValArr of type Number - Coord)
  features[1].geometry.coordinates[0]		(Array - PtSet/Polygon)
  features[1].geometry.coordinates		(Array - Set of PtSet/Polygon)
  features[1].geometry				(Node)
  features[1]					(Node)
  features					(Array)

 */

mExpClass(Basic) Key : public BufferStringSet
{
public:

			Key()					{}
			Key( const char* s )			{ set(s); }
			Key( const SeparString& ss )		{ set(ss); }
			Key( const BufferStringSet& bss,
			     int startlvl=0 ) { set(bss,startlvl); }

    BufferString	toString() const		{ return cat("."); }

    void		set(const char* dot_sep_if_multilevel);
    void		set(const SeparString&);
    void		set(const BufferStringSet&,int startlvl=0);

};


inline Array& ValueSet::asArray()
{ return *static_cast<Array*>( this ); }
inline const Array& ValueSet::asArray() const
{ return *static_cast<const Array*>( this ); }
inline Node& ValueSet::asNode()
{ return *static_cast<Node*>( this ); }
inline const Node& ValueSet::asNode() const
{ return *static_cast<const Node*>( this ); }


} // namespace JSON

} // namespace OD
