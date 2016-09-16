#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "monitorchangerecorder.h"
#include "pickset.h"
#include "perthreadrepos.h"


namespace Pick
{


/*!\brief Holds one change in a Pick::Set. */

mExpClass(General) SetChangeRecord : public ChangeRecorder::Record
{
public:

    typedef Set::LocID			LocID;
    typedef ChangeRecorder::Action	Action;

    LocID		locid_;
    Location		loc_;

    virtual bool	apply(Monitorable&,Action) const;
    virtual bool	isValid() const		{ return locid_.isValid();}
    static const SetChangeRecord& udf();

protected:

			SetChangeRecord( LocID id, const Location& loc )
			    : locid_(id), loc_(loc)	{}

    void		replaceID(Set&,LocID tmpid,LocID realid) const;
    virtual void	doApply(Set&,bool) const	= 0;

};


mExpClass(General) SetLocCreateRecord : public SetChangeRecord
{
public:

			SetLocCreateRecord( LocID id, const Location& loc,
				  LocID beforeid=LocID::getInvalid() )
			    : SetChangeRecord(id,loc)
			    , beforeid_(beforeid)		{}

    LocID		beforeid_;

    virtual Record*	clone() const	{ return new SetLocCreateRecord(*this);}
    virtual uiString	name() const;
    virtual void	doApply(Set&,bool) const;

};


mExpClass(General) SetLocMoveRecord : public SetChangeRecord
{
public:

			SetLocMoveRecord( LocID id, const Location& from,
					    const Location& to )
			: SetChangeRecord(id,to)
			, prevloc_(from)		{}

    Location		prevloc_;

    virtual Record*	clone() const	{ return new SetLocMoveRecord(*this); }
    virtual uiString	name() const;
    virtual void	doApply(Set&,bool) const;

};


mExpClass(General) SetLocRemoveRecord : public SetChangeRecord
{
public:

			SetLocRemoveRecord( LocID id, const Location& loc,
				  LocID beforeid )
			    : SetChangeRecord(id,loc)
			    , beforeid_(beforeid)		{}

    LocID		beforeid_;

    virtual Record*	clone() const	{ return new SetLocRemoveRecord(*this);}
    virtual uiString	name() const;
    virtual void	doApply(Set&,bool) const;

};



/*!\brief Keeps track of changes in a Pick::Set. */

mExpClass(General) SetChangeRecorder : public ::ChangeRecorder
{
public:

    typedef Set::LocID	LocID;

			SetChangeRecorder(Set&);
			SetChangeRecorder(const Set&);
			mDeclMonitorableAssignment(SetChangeRecorder);

protected:

    PerThreadObjectRepository<Location>	movestartloc_;

    void		handleObjChange(const ChangeData&);

};

} // namespace Pick
