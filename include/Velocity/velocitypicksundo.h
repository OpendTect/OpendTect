#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "undo.h"
#include "rowcol.h"
#include "position.h"
#include "velocitypicks.h"


namespace Vel
{

//!Undo event for changed velocity pick
mClass(Velocity) PickSetEvent : public BinIDUndoEvent
{
public:
    		PickSetEvent(Picks&,const Pick& oldpick,
			     const Pick& newpick,const BinID&);

    bool		unDo();
    bool		reDo();
    const char*		getStandardDesc() const;
    const BinID&	getBinID() const	{ return bid_; }

protected:

    Picks&		picks_;
    Pick		oldpick_;
    Pick		newpick_;
    BinID		bid_;
};


//!Undo event for added velocity pick
mClass(Velocity) PickAddEvent : public BinIDUndoEvent
{
public:
			PickAddEvent(Picks&, const RowCol&);

    bool		unDo();
    bool		reDo();
    const char*		getStandardDesc() const;
    const BinID&	getBinID() const	{ return newbid_; }

protected:

    Picks&		picks_;

    Pick		newpick_;
    BinID		newbid_;
};


//!Undo event for removed velocity pick
mClass(Velocity) PickRemoveEvent : public BinIDUndoEvent
{
public:
			PickRemoveEvent(Picks&, const RowCol&);

    bool		unDo();
    bool		reDo();
    const char*		getStandardDesc() const;
    const BinID&	getBinID() const	{ return oldbid_; }

protected:

    Picks&	picks_;
    RowCol	pos_;
    Pick	oldpick_;
    BinID	oldbid_;
};

} // namespace Vel

