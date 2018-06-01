#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Nov 2009
________________________________________________________________________

-*/

#include "generalmod.h"
#include "generalmod.h"
#include "callback.h"
#include "bufstringset.h"
#include "color.h"
#include "coltabseqmgr.h"
#include "coltabmapper.h"

class DataPointSet;

/*!
\brief Interface for DataPointSet Displays.

  Object must be locked before accessing any of the other functions, and should
  be unlocked when done.

  The datapointset can be displayed in a number of viewers.

  DispID not to be confused with Visid. It is used to keep an account for the
  DataPointSetDisplayMgr only.
*/

mClass(General) DataPointSetDisplayProp
{
public:
			DataPointSetDisplayProp(
				const ColTab::Sequence& cs,
				const ColTab::MapperSetup& msu, int id)
			    : colseq_(&cs)
			    , ctmapper_(new ColTab::Mapper(msu))
			    , showsel_(false), dpscolid_(id)	{}

			DataPointSetDisplayProp(
				const BufferStringSet& nms,
				const TypeSet<Color>& cols)
			    : selgrpnms_(nms), selgrpcols_(cols)
			    , showsel_(true), dpscolid_(-1)
			    , colseq_(ColTab::SeqMGR().getDefault())
			    , ctmapper_(new ColTab::Mapper)
			{}
    virtual		~DataPointSetDisplayProp()	{}

    virtual DataPointSetDisplayProp* clone() const
    {
        if ( showsel_ )
	    return new DataPointSetDisplayProp( selgrpnms_, selgrpcols_ );
        else
	    return new DataPointSetDisplayProp( *colseq_,
					ctmapper_->setup(), dpscolid_ );
    }

   int				dpsColID() const	{ return dpscolid_; }
   bool				showSelected() const	{ return showsel_; }
   const BufferStringSet&	selGrpNames() const	{ return selgrpnms_; }
   const TypeSet<Color>&	selGrpColors() const	{ return selgrpcols_; }
   const ColTab::Sequence&	colSequence() const	{ return *colseq_; }
   ColTab::Mapper&		colTabMapper()		{ return *ctmapper_; }
   const ColTab::Mapper&	colTabMapper() const	{ return *ctmapper_; }

virtual Color getColor( float val ) const
{
    if ( showsel_ )
    {
	return selgrpcols_.validIdx(mNINT32(val)) ? selgrpcols_[mNINT32(val)]
						: Color::NoColor();
    }

    if ( mIsUdf(val) )
	return colseq_->undefColor();

    const float pos = ctmapper_->seqPosition( val );
    return colseq_->color( pos );
}

protected:

   BufferStringSet		selgrpnms_;
   TypeSet<Color>		selgrpcols_;
   ConstRefMan<ColTab::Sequence> colseq_;
   RefMan<ColTab::Mapper>	ctmapper_;
   int				dpscolid_;
   bool				showsel_;

};


/*!
\brief DataPointSet display manager.
*/

mClass(General) DataPointSetDisplayMgr : public CallBacker
{
public:

    typedef int			DispID;
    virtual			~DataPointSetDisplayMgr() { clearDispProp(); }
    virtual void		lock()					= 0;
    virtual void		unLock()				= 0;

    virtual bool		hasDisplays() const			= 0;
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
    virtual void		getIconInfo(BufferString&,
					    uiString& tooltip) const = 0;

    DataPointSetDisplayProp*	dispProp()
				{ return dispprop_; }
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
