#ifndef velocitypicksundo_h
#define velocitypicksundo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocitypicksundo.h,v 1.1 2009-03-05 13:25:32 cvskris Exp $
________________________________________________________________________


-*/

#include "undo.h"
#include "rowcol.h"
#include "position.h"
#include "velocitypicks.h"


namespace Vel
{

//!Undo event for changed velocity pick
class PickSetEvent : public BinIDUndoEvent
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
class PickAddEvent : public BinIDUndoEvent
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
class PickRemoveEvent : public BinIDUndoEvent
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

}; //namespace

#endif
