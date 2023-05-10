#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "typeset.h"

class SurveyDiskLocation;

/*!
\brief Compound key consisting of ints.
*/

mExpClass(Basic) MultiID
{
public:
			MultiID();
			MultiID(int grpid, int objid);
			MultiID(const MultiID&);
    explicit		MultiID(const char* idstr);
			MultiID(int grpid,int objid,int subgrpid,int subobjid);
    virtual		~MultiID();

    inline int		nrIDs() const			{ return ids_.size(); }
    int			ID(int idx) const;
    inline int		groupID() const			{ return ID(0); }
    inline int		objectID() const		{ return ID(1); }
    inline int		subGroupID() const		{ return ID(2); }
    inline int		subObjectID() const		{ return ID(3); }
    MultiID		mainID() const;
    bool		isDatabaseID() const;
    bool		isInMemoryID() const;
    bool		isTmpObjectID() const;
    bool		isSyntheticID() const;

    MultiID&		setID(int idx,int id);
    inline MultiID&	setGroupID( int id )		{ return setID(0,id); }
    inline MultiID&	setObjectID( int id )		{ return setID(1,id); }
    inline MultiID&	setSubGroupID( int id )		{ return setID(2,id); }
    inline MultiID&	setSubObjectID( int id )	{ return setID(3,id); }

    bool		fromString(const char*);
    BufferString	toString() const;
    bool		isEqualTo(const char*) const;

    MultiID&		operator =(const MultiID&);
    bool		operator ==(const MultiID&) const;
    bool		operator !=(const MultiID&) const;

    static const MultiID& udf();
    bool		isUdf() const;
    inline MultiID&	setUdf()		{ *this = udf(); return *this; }

    static int		cLastInMemoryGrpID()	{ return 99; }
    static int		cFirstDatabaseGrpID()	{ return 100000; }
    static int		cSyntheticObjID()	{ return 999998; }
    static int		cTmpObjID()		{ return 999999; }

    virtual const SurveyDiskLocation& surveyDiskLocation() const;
    virtual void	setSurveyDiskLocation(const SurveyDiskLocation&);
    virtual bool	hasSurveyLocation() const	{ return false; }
    virtual bool	isInCurrentSurvey() const	{ return true; }

private:
    MultiID&		add(int id);
    TypeSet<int>	ids_;

public:
// Obsolete stuff
			MultiID(int id)				= delete;
			MultiID(const OD::String&)		= delete;

    void		setEmpty()				= delete;
    bool		isEmpty() const				= delete;

    inline bool		operator ==(const char*) const		= delete;
    inline bool		operator !=(const char*) const		= delete;
    inline MultiID&	operator =(const CompoundKey&)		= delete;
    inline MultiID&	operator =(const StringView&)		= delete;
    inline MultiID&	operator =(const char*)			= delete;
    MultiID&		operator +=(const char*)		= delete;

    mDeprecated("Use objectID() or subObjectID")
    int			leafID() const;
    mDeprecated("Use mainID()")
    MultiID		parent() const;

// From CompoundKey
    mDeprecated("Use nrIDs()")
    int			nrKeys() const		{ return nrIDs(); }
    mDeprecated("Use toString()")
    const char*		buf() const;
};
