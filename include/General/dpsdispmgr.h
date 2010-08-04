#ifndef dpsdispmgr_h
#define dpsdispmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Nov 2009
 RCS:		$Id: dpsdispmgr.h,v 1.4 2010-08-04 14:49:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "bufstring.h"
#include "color.h"

class DataPointSet;

/*!Interface for DataPointSet Displays. Object must be locked before
   accessing any of the other functions, and should be unlocked when
   done.

   The datapointset can be displayed in a number of viewers.

   DispID not to be confused with Visid. Its used to keep an account for the
   DataPointSetDisplayMgr only.
*/

mStruct DataPointSetDisplayMgrGrp
{
public:
    				DataPointSetDisplayMgrGrp( const char* nm,
							   const Color& col )
				    : name_(nm), col_(col)	{}

   BufferString			name_;
   Color			col_;
};


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

    virtual void		setDisplayCol(DispID,
	    				      const TypeSet<Color>&)	= 0;
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

    void			addDispMgrGrp( const char* nm,const Color& col )
    { dispmgrgrps_ += new DataPointSetDisplayMgrGrp( nm, col ); }

    void			removeDispMgrGrp( int nr )
				{ if ( dispmgrgrps_.validIdx(nr) )
				    delete dispmgrgrps_.remove( nr ); }

    void			removeAllGrps()
				{
				    while ( !dispmgrgrps_.isEmpty() )
					removeDispMgrGrp(dispmgrgrps_.size()-1);
				}
    
    const ObjectSet<DataPointSetDisplayMgrGrp>& groups() const
				{ return dispmgrgrps_; }

protected:

    TypeSet<int>		availableviewers_;
    ObjectSet<DataPointSetDisplayMgrGrp> dispmgrgrps_;

};
	    				   
#endif
