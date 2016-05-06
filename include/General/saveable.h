#ifndef saveable_h
#define saveable_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "monitor.h"
#include "multiid.h"
#include "uistring.h"
class IOObj;


namespace OD
{


/*!\brief Object that can be saved at any time. */

mExpClass(General) Saveable : public Monitorable
{
public:

			Saveable(const Monitorable&);
			~Saveable();
    const Monitorable&	monitored() const		{ return obj_; }

    mImplSimpleMonitoredGetSet(inline,key,setKey,MultiID,key_,0)
    inline		mImplSimpleMonitoredGet(isFinished,bool,objdeleted_)

    virtual bool	store(const IOObj&) const;
    uiString		errMsg() const		{ return errmsg_; }

protected:

    const Monitorable&	obj_;
    MultiID		key_;
    bool		objdeleted_;
    mutable uiString	errmsg_;

			// This function can be called from any thread
    virtual bool	doStore(const IOObj&) const	= 0;

private:

    void		objDel(CallBacker*);

};


}; //namespace OD

#endif
