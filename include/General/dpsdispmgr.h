#ifndef dpsdispmgr_h
#define dpsdispmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Nov 2009
 RCS:		$Id: dpsdispmgr.h,v 1.9 2012/07/10 13:05:56 cvskris Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "bufstringset.h"
#include "color.h"
#include "coltabsequence.h"
#include "coltabmapper.h"

class DataPointSet;

/*!Interface for DataPointSet Displays. Object must be locked before
   accessing any of the other functions, and should be unlocked when
   done.

   The datapointset can be displayed in a number of viewers.

   DispID not to be confused with Visid. Its used to keep an account for the
   DataPointSetDisplayMgr only.
*/

mStruct DataPointSetDisplayProp
{
public:
				DataPointSetDisplayProp( 
					const ColTab::Sequence& cs,
				        const ColTab::MapperSetup& cm,int id)
				    : coltab_(cs), coltabmappersu_(cm)
				    , showsel_(false), dpscolid_(id)	{}
				
				DataPointSetDisplayProp( 
					const BufferStringSet& nms,
				        const TypeSet<Color>& cols)
				    : selgrpnms_(nms), selgrpcols_(cols)
				    , showsel_(true), dpscolid_(-1)	{}

DataPointSetDisplayProp* clone() const
{
    if ( showsel_ )
	return new DataPointSetDisplayProp( selgrpnms_, selgrpcols_ );
    else
	return new DataPointSetDisplayProp(coltab_,coltabmappersu_,dpscolid_);
}

   int				dpsColID() const	{ return dpscolid_; }
   bool				showSelected() const	{ return showsel_; }
   const BufferStringSet&	selGrpNames() const	{ return selgrpnms_; }
   const TypeSet<Color>&	selGrpColors() const	{ return selgrpcols_; }
   const ColTab::Sequence&	colSequence() const	{ return coltab_; }
   const ColTab::MapperSetup&	colMapperSetUp() const
   				{ return coltabmappersu_; }

Color getColor( float val ) const
{
    if ( showsel_ )
    {
	return selgrpcols_.validIdx(mNINT32(val)) ? selgrpcols_[mNINT32(val)]
	    					: Color::NoColor();
    }

    if ( mIsUdf(val) )
	return coltab_.undefColor();

    ColTab::Mapper mapper;
    mapper.setup_ = coltabmappersu_;
    const float pos = mapper.position( val );
    Color col = coltab_.color( pos );
    col.setTransparency( mNINT32(coltab_.transparencyAt(pos)) );
    return col;
}    

protected:

   BufferStringSet		selgrpnms_;
   TypeSet<Color>		selgrpcols_;
   ColTab::Sequence		coltab_;
   ColTab::MapperSetup		coltabmappersu_;
   int				dpscolid_;
   bool				showsel_;
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

    const DataPointSetDisplayProp* dispProp() const
    				{ return dispprop_; }
    void			setDispProp( DataPointSetDisplayProp* prop )
				{ delete dispprop_; dispprop_ = prop; }

    void			clearDispProp()
    				{ delete dispprop_; dispprop_ = 0; }
    
protected:

    				DataPointSetDisplayMgr()
				    : dispprop_( 0 )	{}
    TypeSet<int>		availableviewers_;
    DataPointSetDisplayProp*	dispprop_;

};
	    				   
#endif
