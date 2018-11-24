#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2016
________________________________________________________________________

-*/

#include "namedmonitorable.h"
#include "uistring.h"
#include "objectset.h"


/*!\brief base class for recorder of changes in a Monitorable */

mExpClass(Basic) ChangeRecorder : public NamedMonitorable
{
public:

			~ChangeRecorder();
    mDeclAbstractMonitorableAssignment(ChangeRecorder);
    bool		isEmpty() const;
    void		setEmpty();

    enum Action		{ Undo, Redo };

    mExpClass(Basic) Record
    { mODTextTranslationClass(ChangeRecorder::Record)
    public:
	typedef ChangeRecorder::Action	Action;
	virtual			~Record()				{}
	virtual Record*		clone() const				= 0;
	virtual bool		isValid() const				= 0;
	virtual uiString	name() const				= 0;
	virtual uiString	actionText(Action) const;
	virtual bool		apply(Monitorable&,Action) const	= 0;
    };


    bool		canApply(Action) const;
    uiString		usrText(Action) const;
    bool		apply(Action);

protected:

    typedef ObjectSet<Record>::idx_type    idx_type;

			ChangeRecorder(Monitorable&,const char* nm);
			ChangeRecorder(const Monitorable&,const char*);

    Monitorable*	obj_;
    ObjectSet<Record>	recs_;
    idx_type		idx4redo_;
    bool		applying_;

			// fns with no locking:
    void		clear();
    void		objDel(CallBacker*);
    void		objChg(CallBacker*);
    idx_type		gtIdx(Action) const;
    void		addRec(Record*);

    virtual void	handleObjChange(const ChangeData&)	= 0;

private:

    void		init();

};
