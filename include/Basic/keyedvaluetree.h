#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2018
________________________________________________________________________

*/

#include "bufstringset.h"
#include "uistring.h"
#include "od_iosfwd.h"
#include <map>
#include <string>

class SeparString;
namespace Gason { union JsonValue; }


namespace KeyedValue
{

class Value;


/*! DataTypes you can find in a JSON file */

enum DataType
{
    Boolean, Number, String,
    ArrayBoolean, ArrayNumber, ArrayString
};

inline bool isArray( DataType dt ) { return dt > String; }


/*!\brief The key to any value in a KeyedValue Node/Tree

  Full keys can and will most often be multi-level. You can specify:
  * Create it as the BufferStringSet it is
  * From a string where the keys are separated by dots ('.') (the 'fullKey')
  * A SeparString (hence also FileMultiString)

 */

mExpClass(Basic) Key : public BufferStringSet
{
public:

			Key()					{}
    explicit		Key( od_int64 i )			{ set(i); }
			Key( const char* s )			{ set(s); }
			Key( const SeparString& ss )		{ set(ss); }
			Key( const BufferStringSet& bss,
			     int startat=0 ) { set(bss,startat); }

    BufferString	fullKey() const	    { return cat("."); }

    void		set( od_int64 i )   { set( toString(i) ); }
    void		set(const char* dot_sep_if_multilevel);
    void		set(const SeparString&);
    void		set(const BufferStringSet&,int startat=0);

};


/*!\brief hold values and other nodes, and is as such a complete (sub-)tree.
  Usually holds information read from a JSON file.

  You can get hold of sub-nodes through a Key (relative to this node).

  Every Node can have (many) values attached, of which you can obtain the
  DataType if you do not already know it (which is usually the case). The
  getValue and setValue functions are template functions, supporting:
  * OD int types (od_int16-64 and od_uint16-32)
  * float and double
  * BufferString; for setValue also const char* and OD::String
  * TypeSet and Array1D of od_[u]intxx, float and double
      (note that for getValue, the Array1D must be right-sized
				or return true to canSetInfo()).

  */

mExpClass(Basic) Node
{ mODTextTranslationClass(KeyedValue::Node)
public:

    typedef int		SzType;
    typedef SzType	IdxType;

			Node( Node* p ) : parent_(p)	{}
    virtual		~Node();

    BufferString	name() const;
    Key			key() const;

    inline bool		isEmpty() const
				{ return children_.empty() && values_.empty(); }
    inline SzType	nrChildren() const
				{ return (SzType)children_.size(); }
    inline SzType	nrValues() const
				{ return (SzType)values_.size(); }
    inline void		setEmpty();

    DataType		getDataType(const Key&) const;
    const Node*		getNode( IdxType idx ) const	{ return gtNode(idx); }
    Node*		getNode( IdxType idx )		{ return gtNode(idx); }
    const Node*		getNode( const Key& ky ) const	{ return gtNode(ky); }
    Node*		getNode( const Key& ky )	{ return gtNode(ky); }
    void		addNode(Node*,const char*);

    template<class T>
    bool		getValue(const Key&,T&) const;
    template<class T>
    bool		setValue(const Key&,const T&);

    bool		setValue(const Key&,char*);
    bool		setValue(const Key&,const char*);
    bool		setValue(const Key&,const OD::String&);

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

    typedef std::map<std::string,Node*>	    ChildrenMap;
    typedef std::map<std::string,Value*>    ValueMap;

    Node*		parent_;
    ChildrenMap		children_;
    ValueMap		values_;

    Node*		gtNode(IdxType) const;
    Node*		gtNode(const Key&) const;

    ChildrenMap::iterator childIter(const Node&);
    ChildrenMap::const_iterator childIter(const Node&) const;
    void		fillKey(const Node&,Key&) const;

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
{ mODTextTranslationClass(KeyedValue::Tree)
public:

			Tree() : Node(0)		{}
			Tree( const IOPar& iop )
			    : Node(0)			{ usePar(iop); }

    uiRetVal		readJSon(od_istream&);
    uiRetVal		writeJSon(od_ostream&);

};

} // namespace KeyedValue
