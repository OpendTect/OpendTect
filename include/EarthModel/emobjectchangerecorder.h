#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Dec 2016
________________________________________________________________________


-*/

#include "emcommon.h"
#include "uistring.h"
#include "monitorchangerecorder.h"

namespace EM
{

/*!\brief Holds one change in an EM::Object . */

mExpClass(General) ChangeRecord : public ChangeRecorder::Record
{
public:

    typedef ChangeRecorder::Action	Action;

    virtual bool		apply(Monitorable&,Action) const;
    static const ChangeRecord&	udf();

protected:

    virtual bool		doApply(Monitorable&,Action) const	= 0;
};


mExpClass(General) PosChangeRecord : public ChangeRecord
{
public:

    EM::PosID		posid_;
    Coord		pos_;

    virtual bool	isValid() const		{ return posid_.isValid();}

protected:

			PosChangeRecord( PosID id, const Coord& pos )
			    : posid_(id), pos_(pos)	{}

    virtual bool	doApply(Object&,bool) const;

};


/*!\brief Keeps track of changes in an EM::Object */

mExpClass(General) ChangeRecorder : public ::ChangeRecorder
{
public:

			ChangeRecorder(Object&);
			ChangeRecorder(const Object&);
			mDeclMonitorableAssignment(ChangeRecorder);

protected:

    void		handleObjChange(const ChangeData&);

};

} // namespace EM
