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


/*!\brief holds Value's, Node's, and/or Array's. */

mExpClass(Basic) Container : public NamedObj
{ mODTextTranslationClass(OD::JSON::Container)
public:

    typedef ValArr::size_type	SzType;
    typedef SzType		IdxType;

    virtual		~Container()			{}

    Key			key() const;
    virtual bool	isEmpty() const			= 0;
    virtual bool	isNode() const			= 0;
    virtual SzType	nrValues() const		= 0;
    inline SzType	nrChildren() const
			{ return (SzType)children_.size(); }

    inline bool		isArray(IdxType) const;
    inline Array&	array(IdxType);
    inline const Array&	array(IdxType) const;
    inline Node&	node(IdxType);
    inline const Node&	node(IdxType) const;

protected:

			Container( const char* nm, Container* p )
			    : NamedObj(nm), parent_(p)	{}
    virtual		~Node();

    Container*		parent_;
    ObjectSet<Container> children_;

};


/*! holds 'flat' value sets of each of the DataType's */

mExpClass(Basic) ValArr
{
public:

    typedef BoolTypeSet::size_type	SzType;
    typedef SzType			IdxType;
    typedef BoolTypeSet			BSet;
    typedef TypeSet<od_int64>		ISet;
    typedef TypeSet<double>		DSet;
    typedef BufferStringSet		SSet;

			ValArr(DataType);
			~ValArr()		{ delete set_; }
    DataType		dataType() const	{ return type_; }

    SzType		size() const
			{ return (SzType)set_->nrItems(); }
    bool		validIdx( IdxType idx ) const
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


/*!\brief holds one of: simple values, other arrays, or other Node's (but no
          mix). Note that Arrays in Arrays have no name. */

mExpClass(Basic) Array : public Container
{
public:

    enum Type		{ Values, Arrays, Nodes };

			Array(bool nodes,const char*,Container*);
			Array(DataType,const char*,Container*);
			~Array();
    virtual bool	isNode() const		{ return false; }

    inline Type		type() const		{ return type_; }
    virtual SzType	nrValues() const;
    SzType		size() const;
    virtual bool	isEmpty() const		{ return size() < 1; }

    inline ValArr&	valArr()		{ return *valarr_; }
    inline const ValArr& valArr() const		{ return *valarr_; }

    void		add(Node*);
    void		add(Array*);

protected:

    Type		type_;
    ValArr*		valarr_;
    ObjectSet<Container> containers_;

};


/*!\brief is a tree of other nodes (children) and values. A value can be
  either a simple type or an Array.

  Every Node can have (many) Value's attached, each either being an Array, or
  a DataType value, of which you can obtain the DataType if you do not already
  know it (which is usually the case). The getValue and setValue functions are
  template functions, supporting:
  * OD int types (od_int16-64 and od_uint16-32)
  * float and double
  * BufferString; for setValue also const char* and OD::String

  Note that if the value itself has Array type, then that array can hold
  an unlimited number of Nodes (and sub-Arrays, for that matter).

  */

mExpClass(Basic) Node : public Container
{ mODTextTranslationClass(OD::JSON::Node)
public:

			Node( const char* nm, Container* p )
			    : NamedObj(nm), parent_(p)	{}
    virtual		~Node();
    virtual bool	isNode() const		{ return true; }

    virtual bool	isEmpty() const
			{ return children_.isEmpty() && values_.isEmpty(); }
    virtual SzType	nrValues() const
				{ return (SzType)values_.size(); }
    inline void		setEmpty();

    DataType		getValueType(IdxType) const;

    bool		isTop() const			{ return !parent_; }
    Node*		top()
			{ return parent_ ? parent_->top() : this; }
    const Node*		top() const
			{ return parent_ ? parent_->top() : this; }

    void		usePar(const IOPar&);
    void		parseJSon(char* buf,int bufsz,uiRetVal&);

    void		fillPar(IOPar&) const;
    void		dumpJSon(BufferString&) const;

protected:

    typedef ObjectSet<Value>	ValueSet;

    Node*		parent_;
    ValueSet		values_;

    int			childIdx(const char*) const;
    Node*		gtChld(IdxType) const;
    Node*		gtChld(const Key&) const;
    int			valueIdx(const char*) const;
    Value*		gtVal(IdxType) const;
    Value*		gtVal(const Key&) const;

    Value*		findValue(const char*) const;
    template<class T>
    bool		getChildValue(const Key&,T&) const;
    template<class IT>
    bool		getIValue(const Key&,IT&) const;
    template<class FT>
    bool		getFValue(const Key&,FT&) const;
    template<class T>
    bool		implSetValue(const Key&,const T&);

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


inline bool Container::isArray( IdxType i ) const
{ return containers_.validIdx(i) ? containers_[i]->isArray() : false; }
inline Array& Container::array( IdxType i )
{ return *((Array*)containers_[i]; }
inline const Array& Container::array( IdxType i ) const
{ return *((Array*)containers_[i]; }
inline Node& Container::node( IdxType i )
{ return *((Node*)containers_[i]; }
inline const Node& Container::node( IdxType i ) const
{ return *((Node*containers_vals_[i]; }

} // namespace JSON

} // namespace OD
