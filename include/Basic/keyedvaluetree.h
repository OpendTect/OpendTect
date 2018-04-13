#pragma once

#include "bufstringset.h"
#include "uistring.h"
#include "od_iosfwd.h"
#include <map>
#include <string>

class SeparString;


namespace KeyedValue
{


enum DataType
{
    Boolean, Number, String,
    ArrayBoolean, ArrayNumber, ArrayString
};

inline bool isArray( DataType dt ) { return dt > String; }


mExpClass(Basic) Key : public BufferStringSet
{
public:

			Key()					{}
    explicit		Key( od_int64 i )			{ set(i); }
    explicit		Key( const char* s )			{ set(s); }
			Key( const SeparString& ss )		{ set(ss); }
			Key( const BufferStringSet& bss,
			     int startat=0 ) { set(bss,startat); }

    BufferString	fullKey() const	    { return cat("."); }

    void		set( od_int64 i )   { set( toString(i) ); }
    void		set(const char* dot_sep_if_multilevel);
    void		set(const SeparString&);
    void		set(const BufferStringSet&,int startat=0);

};


class Value;
class Tree;


mExpClass(Basic) Node
{
public:

    typedef int		SzType;
    typedef SzType	IdxType;

			Node( Node* p ) : parent_(p)	{}
    virtual		~Node();

    BufferString	name() const;
    Key			key() const;

    inline bool		isEmpty() const
				{ return children_.empty() && values_.empty(); }
    inline SzType	nrSubNodes() const
				{ return (SzType)children_.size(); }
    inline SzType	nrValues() const
				{ return (SzType)values_.size(); }
    inline void		setEmpty();

    DataType		getDataType(const Key&) const;
    const Node*		getNode( IdxType idx ) const	{ return gtNode(idx); }
    Node*		getNode( IdxType idx )		{ return gtNode(idx); }
    const Node*		getNode( const Key& ky ) const	{ return gtNode(ky); }
    Node*		getNode( const Key& ky )	{ return gtNode(ky); }

    template<class T>
    bool		getValue(const Key&,T&) const;

    void		addNode(Node*,const char*);
    template<class T>
    bool		addValue(const Key&,const T&);
    bool		addValue(const Key&,const char*);
    bool		addValue(const Key&,const OD::String&);

    bool		isTop() const			{ return !parent_; }
    Node*		top()
			{ return parent_ ? parent_->top() : this; }
    const Node*		top() const
			{ return parent_ ? parent_->top() : this; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

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
    bool		getSubNodeValue(const Key&,T&) const;
    template<class IT>
    bool		getIValue(const Key&,IT&) const;
    template<class T>
    bool		implAddValue(const Key&,const T&);

};


mExpClass(Basic) Tree : public Node
{ mODTextTranslationClass(Tree)
public:

			Tree() : Node(0)		{}
			Tree( const IOPar& iop )
			    : Node(0)			{ usePar(iop); }

    uiRetVal		readJSon(od_istream&);
    uiRetVal		writeJSon(od_ostream&);

};

} // namespace KeyedValue
