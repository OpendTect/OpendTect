#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Dec 2016
________________________________________________________________________


-*/

#include "uistring.h"
#include "monitorchangerecorder.h"

namespace EM
{

/*!\brief Holds one change in an EMObject . */

mExpClass(General) EMChangeRecord : public ChangeRecorder::Record
{
public:

    typedef ChangeRecorder::Action	Action;

    EM::PosID		posid_;
    Coord		pos_;

    virtual bool	apply(Monitorable&,Action) const;
    virtual bool	isValid() const		{ return posid_.isValid();}
    static const EMChangeRecord& udf();

protected:

			EMChangeRecord( PosID id, const Coord& pos )
			    : posid_(id), pos_(pos)	{}

//    virtual void	doApply(EMObject&,bool) const	= 0;

};


/*!\brief Keeps track of changes in an EMObject */

mExpClass(General) EMChangeRecorder : public ::ChangeRecorder
{
public:

			EMChangeRecorder(EMObject&);
			EMChangeRecorder(const EMObject&);
			mDeclMonitorableAssignment(EMChangeRecorder);

protected:

    void		handleObjChange(const ChangeData&);

};

} // namespace EM

