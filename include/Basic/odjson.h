#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2018
________________________________________________________________________

*/

#include "namedobj.h"
#include "bufstringset.h"
#include "uistring.h"
#include "od_iosfwd.h"
#include <map>
#include <string>

class SeparString;
namespace Gason { union JsonValue; }


namespace OD
{

namespace JSON
{

class Value;
class Array;
class Node;

/*! data types you can find in a JSON file */

enum DataType
{
    Boolean, Int, Double, String
};


/*! holds 'flat' value sets of each of the DataType's */

mExpClass(Basic) ValArr
{
public:

    typedef BoolTypeSet::size_type	size_type;
    typedef size_type			idx_type;
    typedef BoolTypeSet			BSet;
    typedef TypeSet<od_int64>		ISet;
    typedef TypeSet<double>		DSet;
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
    ISet&		ints()			{ return *((ISet*)set_); }
    const ISet&		ints() const		{ return *((ISet*)set_); }
    DSet&		doubles()		{ return *((DSet*)set_); }
    const DSet&		doubles() const		{ return *((DSet*)set_); }
    SSet&		strings()		{ return *((SSet*)set_); }
    const SSet&		strings() const		{ return *((SSet*)set_); }

protected:

    DataType		type_;
    OD::Set*		set_;

};


/*!\brief holds Simple values, Node's, and/or Array's. */

mExpClass(Basic) ValueSet
{
public:

    typedef ValArr::size_type	size_type;
    typedef size_type		idx_type;
    enum ValueType		{ Data, SubArray, SubNode };

    virtual		~ValueSet()			{ setEmpty(); }

    virtual bool	isNode() const				= 0;
    inline size_type	size() const
			{ return (size_type)values_.size(); }
    virtual bool	isEmpty() const
			{ return values_.isEmpty(); }
    virtual void	setEmpty();

    index_type		indexOf(const char*) const;
    bool		isPresent( const char* ky ) const
						{ return indexOf(ky) >= 0; }

    virtual ValueType	valueType(idx_type) const		= 0;
    inline bool		isArray( idx_type i ) const
			{ return valueType(i) == Array; }
    inline bool		isNode( idx_type i ) const
			{ return valueType(i) == Node; }
    bool		isTop() const		{ return !parent_; }
    Tree*		tree();
    const Tree*		tree() const;

#   define		mMkSubFn(typ,getfn,implfn) \
    typ*		getfn( const char* ky )		{ return implfn(ky); } \
    const typ*		getfn( const char* ky ) const	{ return implfn(ky); }

    mMkSubFn(ValueSet,	getChild, gtChild )
    mMkSubFn(Node,	getNode, gtNode )
    mMkSubFn(Array,	getArray, gtArray )

    inline Value&	value( idx_type i )		{ return *values_[i]; }
    inline const Value&	value( idx_type i ) const	{ return *values_[i]; }
    Array&		array(idx_type);
    const Array&	array(idx_type) const;
    Node&		node(idx_type);
    const Node&		node(idx_type) const;

    BufferString	getStringValue(const char* ky) const;

    virtual void	add(const char* ky,ValueSet*);
    void		add(const char* ky,const char*);
    void		add(const char* ky,const OD::String&);
    void		add(const char* ky,od_int64);
    void		add(const char* ky,double);

protected:

			ValueSet( ValueSet* p )
			    : parent_(p)	{}
    virtual		~Node();

    ValueSet*		parent_;
    ObjectSet<Value>	values_;

    Value*		findValue(const char*) const;
    ValueSet*		gtChild(const char*) const;
    Array*		gtArray(const char*) const;
    Node*		gtNode(const char*) const;

};


/*!\brief container holding simple arrays of data or either other Arrays
  or other Nodes. Note that Arrays held by other Arrays may have no name. */

mExpClass(Basic) Array : public ValueSet
{
public:

			Array(bool nodes,const char*,ValueSet*);
			Array(DataType,const char*,ValueSet*);
			~Array();
    virtual bool	isArray() const		{ return true; }
    virtual void	setEmpty();

    virtual ValueType	valueType(idx_type) const { return valtype_; }
    size_type		nrElements() const;

    inline ValArr&	valArr()		{ return *valarr_; }
    inline const ValArr& valArr() const		{ return *valarr_; }

    virtual void	add(ValueSet*);

protected:

    ValueType		valtype_;
    ValArr*		valarr_;

};


/*!\brief container holding a mix of key-value pairs and other ValueSets. */

mExpClass(Basic) Node : public ValueSet
{ mODTextTranslationClass(OD::JSON::Node)
public:

			Node( const char* nm, ValueSet* p )
			    : NamedObj(nm), parent_(p)	{}
    virtual bool	isArray() const		{ return false; }
    virtual ValueType	valueType(idx_type) const;

    void		usePar(const IOPar&);
    void		parseJSon(char* buf,int bufsz,uiRetVal&);

    void		fillPar(IOPar&) const;
    void		dumpJSon(BufferString&) const;

protected:

    void		useJsonValue(Gason::JsonValue&,const char*);

};


/*!\brief is just a Node that can be read/written to/from a stream.

  The necessity of this class is debatable, but I guess it will make code more
  understandable.
*/

mExpClass(Basic) Tree : public Node
{ mODTextTranslationClass(OD::JSON::Tree)
public:

			Tree() : Node(0)		{}
			Tree( const IOPar& iop )
			    : Node(0)			{ usePar(iop); }

    uiRetVal		read(od_istream&);
    uiRetVal		write(od_ostream&);

};


/*!\brief The key to any value in a JSON Node/Tree

  Full keys can and will most often be multi-level. You can specify:
  * Create it as the BufferStringSet it is
  * From a string where the keys are separated by dots ('.')
  * A SeparString (hence also FileMultiString)

  You can spacify indexes in arrays using the intuitive [] subscript.

  GeoJSon example of fullKey:
  features[1].geometry.coordinates[0][3][1]	(double - Y value)
  features[1].geometry.coordinates[0][3]	(ValArr of type Double - Coord)
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


} // namespace JSON

} // namespace OD
