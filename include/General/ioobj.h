#ifndef ioobj_h
#define ioobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		31-7-1995
 RCS:		$Id: ioobj.h,v 1.32 2010-12-14 11:15:20 cvsbert Exp $
________________________________________________________________________

-*/
 
 
#include "conn.h"
#include "multiid.h"
#include "namedobj.h"
class IOPar;
class CallBack;
class ascistream;
class ascostream;
class Translator;


/*\brief factory entry for IOObjs. Should deliver IOObj of certain type. */

mClass IOObjProducer
{
public:

    virtual bool	canMake(const char*) const	= 0;
    virtual IOObj*	make(const char*,const MultiID&,
	    		     bool fill_defs) const	= 0;

};

/*\brief object describing how to do I/O for objects

The IOMan object manager manages IODir directories of IOObj objects. These
objects contain the information needed to do the I/O needed for the storage
that is accessed in OpendTect software. IOObj objects not in the root IODir will
have a parent object, which may or may not be useful in relation to the IOObj.

In any case, every IOObj has a unique key in the form of a MultiID. This key
holds (a) the position of the IOObj in the IOMan/IODir tree (b) a unique
integer as an index in the IOObj's own IODir.

*/


mClass IOObj : public NamedObject
{
public:

    IOObj*		clone() const;
    virtual const MultiID& key() const			{ return key_; }

    virtual		~IOObj();
    virtual bool	bad() const			= 0;
    virtual void	copyFrom(const IOObj*)		= 0;
    virtual bool	hasConnType( const char* s ) const
			{ return s && !strcmp(s,connType()); }

    virtual const char*	connType() const		= 0;
    virtual Conn*	getConn(Conn::State) const	= 0;

    virtual const char*	translator() const		{ return transl_; }
    virtual void	setTranslator( const char* s )	{ transl_ = s; }
    virtual const char*	group() const			{ return group_; }
    virtual void	setGroup( const char* s )	{ group_ = s; }
    virtual const char*	fullUserExpr(bool forread) const= 0;

    virtual bool	implExists(bool forread) const	= 0;
    virtual bool	implReadOnly() const		{ return true; }
    virtual bool	implRemove() const		{ return false; }
    virtual bool	implShouldRemove() const	{ return true; }
    virtual bool	implRename(const char*,const CallBack* cb=0)
    							{ return false; }
    virtual bool	implSetReadOnly(bool) const	{ return false; }
    virtual bool	removeQuery() const		{ return false; }
    virtual void	genDefaultImpl()		{}

    const char*		dirName() const			{ return dirnm_; }
    			//!< The full path to the position in the tree
			//!<\note may not be the directory of an implementation
    virtual IOPar&	pars() const			{ return pars_; }
    			//!< These are the extra parameters: #xxx: yyy in .omf

    static bool		isKey(const char*);
    			//!< Returns whether given string may be a valid key
    static bool		isSurveyDefault(const MultiID&);
    			//!< Checks the 'Default.' entries in SI().pars()

    Translator*		getTranslator() const;
    			//!< returns a subclass of Translator according to
			//!< the translator name and group.
    virtual void	acquireNewKey();
    			//!< This will give the IOObj a new (free) ID

    static const int	tmpID()		{ return  999999; }
    inline bool		isTmp() const	{ return key_.leafID() == tmpID(); }
    bool		isReadDefault() const;

protected:

    BufferString	dirnm_;
    MultiID		key_;
    BufferString	transl_;
    BufferString	group_;

			IOObj(const char* nm=0,const char* ky=0);
			IOObj(const IOObj&);
    static IOObj*	get(ascistream&,const char*,const char*);
    bool		put(ascostream&) const;
    virtual bool	getFrom(ascistream&)		= 0;
    virtual bool	putTo(ascostream&) const	= 0;
    void		setKey( const char* nm )	{ key_ = nm; }
    int			myKey() const;
    void		setDirName( const char* s )	{ dirnm_ = s; }

private:

    friend class	IODir;

    static IOObj*	produce(const char*,const char* nm=0,const char* ky=0,
				bool initdefaults=true);
    void		copyStuffFrom(const IOObj&);

    IOPar&		pars_;

public:

    virtual bool	isSubdir() const		{ return false; }
    static int		addProducer(IOObjProducer*);
    			//!< Factory for IOObj types. Not for casual use.

};

mGlobal bool equalIOObj(const MultiID&,const MultiID&);
mGlobal bool areEqual(const IOObj*,const IOObj*);
mGlobal bool fullImplRemove(const IOObj&);

#endif
