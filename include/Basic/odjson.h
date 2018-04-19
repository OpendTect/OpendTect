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
    Boolean, Int, FP, String
};

typedef od_int64 IntType;
typedef double FPType;


/*! holds 'flat' value sets of each of the DataType's */

mExpClass(Basic) ValArr
{
public:

    typedef BoolTypeSet::size_type	size_type;
    typedef size_type			idx_type;
    typedef BoolTypeSet			BSet;
    typedef TypeSet<IntType>		ISet;
    typedef TypeSet<FPType>		FPSet;
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
    FPSet&		fps()			{ return *((FPSet*)set_); }
    const FPSet&	fps() const		{ return *((FPSet*)set_); }
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
    virtual bool	isArray() const			 = 0;

    inline size_type	size() const
			{ return (size_type)values_.size(); }
    virtual bool	isEmpty() const
			{ return values_.isEmpty(); }
    virtual void	setEmpty();

    index_type		indexOf(const char*) const;
    bool		isPresent( const char* ky ) const
						{ return indexOf(ky) >= 0; }

    virtual ValueType	valueType(idx_type) const;
    inline bool		isPlainData( idx_type i ) const
			{ return valueType(i) == Data; }
    inline bool		isArrayChild( idx_type i ) const
			{ return valueType(i) == Array; }
    inline bool		isNodeChild( idx_type i ) const
			{ return valueType(i) == Node; }

    bool		isTop() const		{ return !parent_; }
    Tree*		tree();
    const Tree*		tree() const;

#   define		mMkGetFns(typ,getfn,implfn) \
    inline typ&		getfn( idx_type i )		{ return implfn(i); } \
    inline const typ&	getfn( idx_type i ) const	{ return implfn(i); }
    mMkGetFns(ValueSet,	child, gtChild )
    mMkGetFns(Array,	array, gtArray )
    mMkGetFns(Node,	node, gtNode )
#   undef		mMkSubFn

    BufferString	getStringValue(idx_type) const;

protected:

			ValueSet( ValueSet* p )
			    : parent_(p)	{}
    virtual		~Node();

    ValueSet*		parent_;
    ObjectSet<Value>	values_;

    ValueSet*		gtChild(idx_type) const;
    Array*		gtArray(idx_type) const;
    Node*		gtNode(idx_type) const;

};


/*!\brief container holding simple arrays of data or either other Arrays
  or other Nodes. Note that Arrays held by other Arrays may have no name. */

mExpClass(Basic) Array : public ValueSet
{
public:

			Array(bool nodes,ValueSet*);
			Array(DataType,ValueSet*);
			~Array();
    virtual bool	isArray() const		{ return true; }
    virtual void	setEmpty();

    virtual ValueType	valueType(idx_type) const { return valtype_; }
    size_type		nrElements() const;

    inline ValArr&	valArr()		{ return *valarr_; }
    inline const ValArr& valArr() const		{ return *valarr_; }

    void		addChild(ValueSet*);

    void		add(bool);
    void		add(IntType);
    void		add(FPType);
    void		add(const char*);
    void		add( const OD::String& odstr ) { add( odstr.str(); }
    void		set(const ValArr::BSet&);
    void		set(const ValArr::ISet&);
    void		set(const ValArr::FPSet&);
    void		set(const ValArr::SSet&);

protected:

    ValueType		valtype_;
    ValArr*		valarr_;

};


/*!\brief ValueSet holding a mix of key-value pairs and other ValueSets. */

mExpClass(Basic) Node : public ValueSet
{ mODTextTranslationClass(OD::JSON::Node)
public:

			Node( ValueSet* p )
			    : ValueSet(p)	{}
    virtual bool	isArray() const		{ return false; }

#   define		mMkGetFn(typ,getfn,implfn) \
    inline typ*		getfn( const char* ky )		{ return implfn(ky); } \
    inline const typ*	getfn( const char* ky ) const	{ return implfn(ky); }
    mMkGetFn(ValueSet,	getChild, gtChild )
    mMkGetFn(Array,	getArray, gtArray )
    mMkGetFn(Node,	getNode, gtNode )
#   undef		mMkGetFn

    void		setChild(const char* ky,ValueSet*);
    void		set(const char* ky,bool);
    void		set(const char* ky,IntType);
    void		set(const char* ky,FPType);
    void		set(const char* ky,const char*);
    void		set(const char* ky,const OD::String&);

    void		usePar(const IOPar&);
    void		parseJSon(char* buf,int bufsz,uiRetVal&);

    void		fillPar(IOPar&) const;
    void		dumpJSon(BufferString&) const;

protected:

    ValueSet*		gtChild(const char*) const;
    Array*		gtArray(const char*) const;
    Node*		gtNode(const char*) const;

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
  features[1].geometry.coordinates[0][3][1]	(FPType - Y value)
  features[1].geometry.coordinates[0][3]	(ValArr of type FP - Coord)
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
