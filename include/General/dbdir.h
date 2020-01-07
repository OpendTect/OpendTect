#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2016
________________________________________________________________________

-*/


#include "generalmod.h"
#include "sharedobject.h"
#include "monitoriter.h"
#include "dbkey.h"
#include "objectset.h"
#include "od_iosfwd.h"
#include "threadlock.h"
#include "uistring.h"
#include "ioobjctxt.h"
#include "ioobj.h"
class TranslatorGroup;
class SurveyDiskLocation;


/*\brief 'Directory' of IOObj objects.

The DBDir class is responsible for finding all IOObj's in one of the
subdirectories of an OpendTect data tree. A DBDir instance will actually load
all IOObj's, provides access to keys and allows searching.

*/


mExpClass(General) DBDir : public SharedObject
{ mODTextTranslationClass(DBDir);
public:

    typedef ObjectSet<IOObj>::size_type	size_type;
    typedef size_type			idx_type;
    typedef DBKey::ObjNrType		ObjNrType;
    typedef DBKey::DirID		DirID;
    typedef DBKey::ObjID		ObjID;

			DBDir(const char* dirname);
			DBDir(DirID);
			DBDir(IOObjContext::StdSelType);
			mDeclMonitorableAssignment(DBDir);

    bool		isBad() const;
    bool		isOutdated() const;
    DirID		dirID() const		{ return dirid_; }
    const char*		dirName() const		{ return dirname_; }
    od_int64		readTime() const	{ return readtime_; }
    uiString		errMsg() const		{ return errmsg_; }

    size_type		size() const;
    bool		isEmpty() const		{ return size() < 1; }

    bool		isPresent(ObjID) const;
    bool		isPresent(const DBKey&) const;
    idx_type		indexOf(ObjID) const;
    bool		hasObjectsWithGroup(const char*) const;

    IOObj*		getEntry(ObjID) const;
    IOObj*		getEntryByName(const char* nm,
					const char* trgrpnm=0) const;
    IOObj*		getEntryByIdx(idx_type) const;

    bool		commitChanges(const IOObj&);
    bool		permRemove(ObjID);
    DBKey		newKey() const;
    DBKey		newTmpKey() const;

    static ChangeType	cEntryChanged()		{ return 2; }
    static ChangeType	cEntryAdded()		{ return 3; }
    static ChangeType	cEntryToBeRemoved()	{ return 4; }
    static ChangeType	cEntryRemoved()		{ return 5; }

private:

    bool		isok_;
    ObjectSet<IOObj>	objs_;
    const DirID		dirid_;
    const BufferString	dirname_;
    od_int64		readtime_	= -1;
    mutable ObjNrType	curnr_		= 0;
    mutable ObjNrType	curtmpnr_	= IOObj::tmpObjNrStart();
    mutable uiString	errmsg_;

			DBDir();
			~DBDir();

    void		fromDirID(DirID,bool);
    bool		readFromFile(bool);
    bool		readOmf(od_istream&,bool);
    bool		writeToFile() const;
    bool		wrOmf(od_ostream&) const;
    const IOObj*	gtObjByName(const char*,const char*) const;
    idx_type		gtIdx(ObjID) const;
    bool		gtIsOutdated() const;
    bool		setObj(IOObj*,bool writeafter);
    bool		addAndWrite(IOObj*);

    friend class	DBMan;
    friend class	DBDirIter;

    DBKey		gtNewKey(const ObjNrType&) const;
    void		setObjDirName(IOObj&);

public:

    // 'Usually not needed' section

    bool		reRead(bool force) const;
				// Done a lot already
    bool		prepObj(IOObj&) const;
				// Will be done before storage anyway

    static void		getTmpIOObjs(DirID,ObjectSet<IOObj>&,
					const IOObjSelConstraints* c=0);

    mDeprecated bool	commitChanges( const IOObj* obj )
			{ return obj ? commitChanges(*obj) : false; }

};


/*\brief iterates a DBDir */

mExpClass(General) DBDirIter : public MonitorableIterBase<DBDir::size_type>
{
public:

    typedef DBDir::ObjID ObjID;

			DBDirIter(const DBDir&);
			DBDirIter(const DBDirIter&);

    virtual size_type	size() const	    { return dbDir().size(); }
    const DBDir&	dbDir() const;

    const IOObj&	ioObj() const;
    ObjID		objID() const;
    DBKey		key() const;

    mDefNoAssignmentOper(DBDirIter)

};


/*!\brief list of DBDir entries, sorted by name, conforming to a context.
    Can be Filtered using GlobExpr. */

mExpClass(General) DBDirEntryList
{
public:

    typedef ObjectSet<IOObj>::size_type	size_type;
    typedef size_type			idx_type;

			DBDirEntryList(const IOObjContext&,bool dofill=true);
			DBDirEntryList(const IOObjContext&,
				       const SurveyDiskLocation&,
				       bool dofill=true);
			DBDirEntryList(const TranslatorGroup&,
					const char* translator_globexpr=0);
			~DBDirEntryList();
    const char*		name() const	{ return name_; }
    size_type		size() const	{ return entries_.size(); }
    bool		isEmpty() const	{ return entries_.isEmpty(); }

    void		fill(const char* nmfiltglobexpr=0);
    idx_type		indexOf(const char*) const;

    const IOObj&	ioobj( idx_type idx ) const { return *entries_[idx]; }
    DBKey		key(idx_type) const;
    BufferString	name(idx_type) const;
    BufferString	dispName(idx_type) const;
    BufferString	iconName(idx_type) const;
    BufferStringSet	getParValuesFor(const char*) const;

protected:

    ObjectSet<IOObj>	entries_;
    IOObjContext&	ctxt_;
    SurveyDiskLocation&	survloc_;
    BufferString	name_;

    void		sort();

};
