#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "enums.h"
#include "multiid.h"
#include "namedobj.h"
#include "uistring.h"

class Conn;
class IOObj;
class CallBack;
class ascistream;
class ascostream;
class Translator;
namespace OD { class DataSetKey; }


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

Every IOObj has a unique key in the form of a MultiID. This key
holds (a) the position of the IOObj in the IOMan/IODir tree (b) a unique
integer as an index in the IOObj's own IODir.

If you know an IOObj is actually pointing to a simple file (i.e. if it's an
IOStream), then you can get the full filename (i.e. with path) using
fullUserExpr().

*/


mExpClass(General) IOObj : public NamedObject
{
public:

    enum class Status
    {
	Unknown=0,
	OK=1,
	FileNotPresent=2,
	ReadPermissionInvalid=3,
	WrongObject=4,
	BrokenLink=5,
	FileEmpty=6,
	FileDataCorrupt=7,
	LibraryNotLoaded=8,
	DataVersionInvalid=9,
	Other=10
    };

    uiString			uiName() const { return ::toUiString(name()); }

    IOObj*			clone() const;
    virtual const MultiID&	key() const			{ return key_; }
    virtual bool		hasDSKey() const;
    virtual const OD::DataSetKey* DSKey() const;
    virtual void		setDSKey(const OD::DataSetKey&);

    virtual			~IOObj();
    virtual bool		isBad() const			= 0;
    virtual void		copyFrom(const IOObj*)		= 0;
    virtual bool		hasConnType( const char* s ) const
				{ return connType() == s; }

    virtual Status		status() const		    { return status_; }
    virtual void		setStatus( Status st )	    { status_ = st; }
    virtual const char*		connType() const		= 0;
    virtual Conn*		getConn(bool forread) const	= 0;

    virtual const OD::String&	translator() const	       {return transl_;}
    virtual void		setTranslator( const char* s ) {transl_ = s; }
    virtual const OD::String&	group() const			{return group_;}
    virtual void		setGroup( const char* s )	{group_ = s; }
    virtual const char*		fullUserExpr(bool forread=true) const = 0;
    virtual int			nrImpls() const;
    virtual BufferString	mainFileName() const { return fullUserExpr(); }
    virtual void		implFileNames(BufferStringSet&) const;

    virtual bool		implIsLink() const;
    virtual bool		implExists(bool forread) const	= 0;
    virtual bool		implReadOnly() const		{ return true; }
    virtual bool		implRemove() const		{ return false;}
    virtual bool		implRename(const char*)		{ return false;}
    virtual bool		implSetReadOnly(bool) const	{ return false;}

    virtual const char*		dirName() const		{ return dirnm_; }
				//!< The directory ame within the tree
    virtual IOPar&		pars() const			{ return pars_;}
				//!< These are the extra parameters: "#xxx: yyy"
				//!< in .omf
    void			updateCreationPars() const;

    static bool			isKey(const char*);
				//!< Returns whether given string may be a valid
				//!< key
    static bool			isSurveyDefault(const MultiID&);
				//!<Checks the 'Default.' entries in SI().pars()
    void			setSurveyDefault() const;
				/*!<\param subsel may be a subselection lower
				    than the translator group, such as
				    "Velocity".*/

    Translator*			createTranslator() const;
				//!< returns a subclass of Translator according
				//!< to the translator name and group.
    void			acquireNewKeyIn(const MultiID&);
				//!< This will give the IOObj a new (free) ID

    static int			tmpID()		{ return MultiID::cTmpObjID(); }
    inline bool			isTmp() const	{ return key_.isTmpObjectID(); }
    bool			isProcTmp() const;
    bool			isUserSelectable(bool forread=true) const;
    bool			isInCurrentSurvey() const;

    uiString			phrCannotOpenObj() const;
    uiString			phrCannotReadObj() const;
    uiString			phrCannotLoadObj() const;
    uiString			phrCannotWriteObj() const;
    uiString			phrCannotWriteToDB() const;

protected:

    BufferString	dirnm_;
    MultiID		key_;
    const OD::DataSetKey* dskey_	= nullptr;
    BufferString	transl_;
    BufferString	group_;
    Status		status_				= Status::Unknown;

			IOObj(const char* nm,const DBKey&);
			IOObj(const IOObj&);
    static IOObj*	get(ascistream&,const char* dirnm,int grpid);
    bool		put(ascostream&) const;
    virtual bool	getFrom(ascistream&)		= 0;
    virtual bool	putTo(ascostream&) const	= 0;
    virtual int		myKey() const;

private:

    friend class	IODir;

    static IOObj*	produce(const char* typ,const char* nm,const DBKey&,
				bool initdefaults=true);
    void		copyStuffFrom(const IOObj&);

    IOPar&		pars_;

public:

    mDeprecated("Use setKey(const MultiID&)")
    void		setKey( const char* nm )
			{ key_.fromString(nm); }
    void		setKey( const MultiID& key )	{ key_ = key; }
    virtual void	setDirName( const char* s )	{ dirnm_ = s; }
    virtual bool	isSubdir() const		{ return false; }
    static int		addProducer(IOObjProducer*);
			//!< Factory for IOObj types. Not for casual use.

protected:
			mDeprecated("Use with DBKey")
			IOObj(const char* nm,const MultiID&);
			mDeprecated("Use with DBKey")
			IOObj(const char* nm,const char* kystr);

};

mGlobal(General) bool equalIOObj(const MultiID&,const MultiID&);
mGlobal(General) bool areEqual(const IOObj*,const IOObj*);
