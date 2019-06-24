#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		31-7-1995
________________________________________________________________________

-*/


#include "generalmod.h"
#include "namedobj.h"
#include "dbkey.h"
#include "uistringset.h"

class Conn;
class IOObj;
class IOStream;
class CallBack;
class ascistream;
class ascostream;
class Translator;


/*\brief factory entry for IOObjs. Should deliver IOObj of certain type. */

mExpClass(General) IOObjProducer
{
public:

    virtual		~IOObjProducer()		{}
    virtual bool	canMake(const char*) const	= 0;
    virtual IOObj*	make(const char*,const DBKey&,
			     bool fill_defs) const	= 0;

};

/*\brief object describing how to do I/O for objects

The IOMan object manager manages IODir directories of IOObj objects. These
objects contain the information needed to do the I/O needed for the storage
that is accessed in OpendTect software.

Every IOObj has a unique key in the form of a DBKey. This key
holds (a) the position of the IOObj in the IOMan/IODir tree (b) a unique
integer as an index in the IOObj's own IODir.

If you know an IOObj is actually pointing to a simple file (i.e. if it's an
IOStream), then you can get the full filename (i.e. with path) using
mainFileName().

*/


mExpClass(General) IOObj : public NamedObject
{
public:

    typedef DBKey::ObjID	ObjID;
    typedef DBKey::ObjNrType	ObjNrType;
    typedef DBKey::GroupID	DirID;

    IOObj*			clone() const;
    virtual DBKey		key() const	{ return key_; }
    virtual ObjID		objID() const	{ return key_.objID(); }
    virtual bool		isInCurrentSurvey() const
				{ return key().isInCurrentSurvey(); }

    virtual			~IOObj();
    virtual bool		isBad() const			= 0;
    virtual void		copyFrom(const IOObj&);
    bool			isEqualTo(const IOObj&) const;
    virtual bool		hasConnType( const char* s ) const
				{ return connType() == s; }

    virtual const char*		connType() const		= 0;
    virtual bool		isStream() const		{ return false;}
    virtual IOStream*		asStream()			{ return 0; }
    virtual const IOStream*	asStream() const		{ return 0; }
    virtual Conn*		getConn(bool forread) const	= 0;

    virtual const OD::String&	translator() const	       {return transl_;}
    virtual void		setTranslator( const char* s ) {transl_ = s; }
    virtual const OD::String&	group() const			{return group_;}
    virtual void		setGroup( const char* s )	{group_ = s; }
    virtual const char*		fullUserExpr(bool forread=true) const = 0;
    virtual BufferString	mainFileName() const { return fullUserExpr(); }

    virtual bool		implExists(bool forread) const	= 0;
    virtual bool		implReadOnly() const		{ return true; }
    virtual bool		implRemove() const		{ return false;}
    virtual bool		implManagesObjects() const	{ return false;}
    virtual bool		implRename(const char*,const CallBack* cb=0)
							{ return false; }
    virtual bool		implSetReadOnly(bool) const	{ return false;}

    virtual const char*		dirName() const		{ return dirnm_; }
				//!< The directory ame within the tree
    virtual IOPar&		pars() const			{ return pars_;}
				//!< These are the extra parameters: #xxx: yyy
				//!< in .omf
    void			updateCreationPars() const;

    static bool			isKey(const char*);
				//!< Returns whether given string may be a valid
				//!< key
    static bool			isSurveyDefault(const DBKey&);
				//!<Checks the 'Default.' entries in SI().pars()
    void			setSurveyDefault(const char* subsel = 0) const;
				/*!<\param subsel may be a subselection lower
				    than the translator group, such as
				    "Velocity".*/

    Translator*			createTranslator() const;
				//!< returns a subclass of Translator according
				//!< to the translator name and group.
    void			setKeyForNewEntry(DirID);
				//!< This will mark the IOObj for DBM() as 'new'

    inline bool			isTmp() const
				{ return isTmpObjNr(objID().getI());}
    bool			isProcTmp() const;
    bool			isUserSelectable(bool forread=true) const;

    inline bool			isInvalid() const   { return key().isInvalid();}
    static const IOObj&		getInvalid();
    static inline ObjNrType	tmpObjNrStart()     { return 999999; }
    static inline bool		isTmpObjNr( ObjNrType nr )
				{ return nr >= tmpObjNrStart(); }

    uiPhrase			phrCannotOpenObj() const;
    uiPhrase			phrCannotReadObj() const;
    uiPhrase			phrCannotLoadObj() const;
    uiPhrase			phrCannotWriteObj() const;
    uiPhrase			phrCannotWriteToDB() const;

    uiRetVal			commitChanges() const;
    bool			removeFromDB() const;

protected:

    DBKey		key_;
    BufferString	transl_;
    BufferString	group_;
    BufferString	dirnm_;

			IOObj(const char* nm=0,DBKey ky=DBKey::getInvalid());
			IOObj(const IOObj&);
    static IOObj*	get(ascistream&,const char*,DirID,bool rejoldtmp=true);
    bool		put(ascostream&) const;
    virtual bool	isEqTo(const IOObj&) const	= 0;
    virtual bool	getFrom(ascistream&)		= 0;
    virtual bool	putTo(ascostream&) const	= 0;

private:

    friend class	DBDir;

    static IOObj*	produce(const char*,const char* nm,const DBKey& ky,
				bool initdefaults);
    void		copyClassData(const IOObj&);
    IOPar&		pars_;

public:

    void		setKey( const DBKey& ky )	{ key_ = ky; }
    virtual void	setDirName( const char* s )	{ dirnm_ = s; }
    virtual bool	isSubdir() const		{ return false; }
    virtual void	setAbsDirectory(const char*)	{}
    static int		addProducer(IOObjProducer*);
			//!< Factory for IOObj types. Not for casual use.

};

mGlobal(General) bool equalIOObj(const DBKey&,const DBKey&);
mGlobal(General) bool fullImplRemove(const IOObj&);
