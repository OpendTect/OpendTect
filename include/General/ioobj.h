#ifndef ioobj_H
#define ioobj_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		31-7-1995
 RCS:		$Id: ioobj.h,v 1.11 2003-03-20 17:07:56 bert Exp $
________________________________________________________________________

-*/
 
 
#include <conn.h>
#include <multiid.h>
class ascistream;
class ascostream;
class Translator;
class IOPar;

/*\brief object describing how to do I/O for objects

The IOMan object manager manages IODir directories of IOObj objects. These
objects contain the information needed to do the I/O needed for the storage
that is accessed in the dGB software. IOObj objects not in the root IODir will
have a parent object, which may or may not be useful in relation to the IOObj.

In any case, every IOObj has a unique key in the form of a MultiID. This key
holds (a) the position of the IOObj in the IOMan/IODir tree (b) a unique
integer as an index in the IOObj's own IODir.

A special type of IOObj is the IOLink, which is the object that links to the
'main object' of a sub-IODir. It will usually return info as if it's the main
object itself, but will also allow changing the IOMan's dir with it.
If a link is removed, the entire tree below it is removed.

*/


class IOObj : public DefObject
	    , public UserIDObject
{	      isUidAbstractDefObject(IOObj)

    friend class	IODir;
    friend class	IOLink;

public:

    IOObj*		clone() const;
    IOObj*		getParent() const;
    			//!< Will return null for objects in the survey dir
    virtual MultiID	key() const			{ return key_; }

    virtual		~IOObj();
    virtual bool	bad() const			= 0;
    virtual bool	isLink() const			{ return false; }
    virtual void	copyFrom(const IOObj*)		= 0;
    virtual bool	hasConnDef( const ClassDef& cd ) const
			{ return &cd == connclassdef_; }

    virtual const ClassDef& connType() const		= 0;
    virtual Conn*	getConn(Conn::State) const	= 0;
    virtual bool	slowOpen() const		{ return false; }

    virtual const MultiID& parentKey() const	= 0;
    virtual void	setParentKey(const char*)	= 0;
    virtual const char*	translator() const		= 0;
    virtual void	setTranslator(const char*)	= 0;
    virtual const char*	group() const			= 0;
    virtual void	setGroup(const char*)		= 0;
    virtual const char*	fullUserExpr(bool forread) const= 0;

    virtual bool	implExists(bool forread) const	= 0;
    virtual bool	implRemovable() const		{ return false; }
    virtual bool	implRemove() const		{ return false; }
    virtual bool	implRename(const char*)		{ return false; }
    virtual bool	removeQuery() const		{ return false; }
    virtual void	genDefaultImpl()		{}

    virtual int		setName(const char*);
    			//!< Set the user reference
    virtual const char*	dirName() const;
    			//!< The full path to the position in the tree
			//!<\note may not be the directory of an implementation
    bool		isStandAlone() const { return dirname_ ? true : false; }
    			//!< IOObjs can be dependent on the IODir
    void		setStandAlone(const char* dirnm);
    			//!< uncouple IOObj from IODir
    virtual IOPar&	pars() const			{ return pars_; }
    			//!< These are the extra parameters: #xxx: yyy in .omf

    static bool		isKey(const char*);
    			//!< Returns whether given string may be a valid key

    Translator*		getTranslator() const;
    			//!< returns a subclass of Translator according to
			//!< the translator name and group.
    virtual void	acquireNewKey();
    			//!< This will give the IOObj a new (free) ID

protected:

    FileNameString*	dirname_;
    const ClassDef*	connclassdef_;
    IOLink*		mylink_;
    MultiID		key_;

			IOObj(const char* nm=0,const char* ky=0);
			IOObj(IOObj*,const char* ky=0);
    static IOObj*	get(ascistream&,const char*,const char*);
    int			put(ascostream&) const;
    virtual int		getFrom(ascistream&)		= 0;
    virtual int		putTo(ascostream&) const	= 0;
    void		setKey( const char* nm )	{ key_ = nm; }

private:

    IOPar&		pars_;
    int			myKey() const;

    static IOObj*	produce(const char*,const char* nm=0,const char* ky=0,
				bool initdefaults=true);

};


bool equalIOObj(const MultiID&,const MultiID&);
bool areEqual(const IOObj*,const IOObj*);
bool fullImplRemove(const IOObj&);


#endif
