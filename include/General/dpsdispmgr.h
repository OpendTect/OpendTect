#ifndef dpsdipsmgr_h
#define dpsdipsmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Nov 2009
 RCS:		$Id: dpsdispmgr.h,v 1.2 2010-03-03 10:11:57 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "callback.h"

class Color;
class DataPointSet;
class BufferString;

/*!Interface for DataPointSet Displays. Object must be locked before
   accessing any of the other functions, and should be unlocked when
   done.

   The datapointset can be displayed in a number of viewers.

   DispID not to be confused with Visid. Its used to keep an account for the
   DataPointSetDisplayMgr only.
*/

mClass DataPointSetDisplayMgr : public CallBacker
{
public:
    typedef int			DispID;
    virtual			~DataPointSetDisplayMgr()		{}
    virtual void		lock()					= 0;
    virtual void		unLock()				= 0;

    virtual bool		hasDisplays() const 			= 0; 
    virtual DispID		getDisplayID(const DataPointSet&) const	= 0;
    virtual int			getNrViewers() const			= 0;
    virtual const char*		getViewerName(int) const		= 0;

    virtual void		setDisplayCol(DispID,const TypeSet<Color>&)	= 0;
    virtual DispID		addDisplay(const TypeSet<int>& parents,
	    				   const DataPointSet&)		= 0;
    virtual void		updateDisplay(DispID id,
				    const TypeSet<int>& parents,
				    const DataPointSet&)		= 0;
    virtual void		updateDisplay(DispID id,const DataPointSet&) =0;
    virtual void		removeDisplay(DispID)			= 0;
    const TypeSet<int>&		availableViewers() const
    				{ return availableviewers_; }

    virtual void		getIconInfo(BufferString& fnm,
	    				    BufferString& tootltip) const = 0;
protected:

    TypeSet<int>		availableviewers_;
};
	    				   


#endif
