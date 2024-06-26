#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "bufstringset.h"
#include "callback.h"
#include "color.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "integerid.h"

class DataPointSet;

/*!
\brief Interface for DataPointSet Displays.

  Object must be locked before accessing any of the other functions, and should
  be unlocked when done.

  The datapointset can be displayed in a number of viewers.

  DispID not to be confused with Visid. It is used to keep an account for the
  DataPointSetDisplayMgr only.
*/

mExpClass(General) DataPointSetDisplayProp
{
public:
				DataPointSetDisplayProp(
					const ColTab::Sequence& cs,
					const ColTab::MapperSetup& cm,int id);
				DataPointSetDisplayProp(
					const BufferStringSet& nms,
					const TypeSet<OD::Color>& cols);
    virtual			~DataPointSetDisplayProp();

    virtual DataPointSetDisplayProp*	clone() const;
    virtual OD::Color			getColor(float val) const;

    int				dpsColID() const	{ return dpscolid_; }
    bool			showSelected() const	{ return showsel_; }
    const BufferStringSet&	selGrpNames() const	{ return selgrpnms_; }
    const TypeSet<OD::Color>&	selGrpColors() const	{ return selgrpcols_; }
    const ColTab::Sequence&	colSequence() const	{ return coltab_; }
    const ColTab::MapperSetup&	colMapperSetUp() const
				{ return coltabmappersu_; }

protected:

   BufferStringSet		selgrpnms_;
   TypeSet<OD::Color>		selgrpcols_;
   ColTab::Sequence		coltab_;
   ColTab::MapperSetup		coltabmappersu_;
   int				dpscolid_;
   bool				showsel_;
};


/*!
\brief DataPointSet display manager.
*/

mExpClass(General) DataPointSetDisplayMgr : public CallBacker
{
public:

    mExpClass(General) DispID : public IntegerID<od_int32>
    {
    public:
	using IntegerID::IntegerID;
	static inline DispID	udf()		{ return DispID(); }
    };

    mExpClass(General) ParentID : public IntegerID<od_int32>
    {
    public:
	using IntegerID::IntegerID;
	static inline ParentID	udf()		{ return ParentID(); }
    };

    virtual			~DataPointSetDisplayMgr();

    virtual void		lock()					= 0;
    virtual void		unLock()				= 0;

    virtual bool		hasDisplays() const			= 0;
    virtual DispID		getDisplayID(const DataPointSet&) const	= 0;
    virtual int			getNrViewers() const			= 0;
    virtual const char*		getViewerName(int) const		= 0;

    virtual DispID		addDisplay(const TypeSet<ParentID>& parents,
					   const DataPointSet&)		= 0;
    virtual void		updateDisplay(const DispID&,
				    const TypeSet<ParentID>& parents,
				    const DataPointSet&)		= 0;
    virtual void		updateDisplay(const DispID&,
					      const DataPointSet&)	= 0;
    virtual void		removeDisplay(const DispID&)		= 0;
    const TypeSet<ParentID>&	availableViewers() const
				{ return availableviewers_; }

    virtual void		getIconInfo(BufferString& fnm,
					    BufferString& tootltip) const = 0;

    const DataPointSetDisplayProp* dispProp() const
				{ return dispprop_; }
    void			setDispProp( DataPointSetDisplayProp* prop )
				{ delete dispprop_; dispprop_ = prop; }

    void			clearDispProp()
				{ delete dispprop_; dispprop_ = nullptr; }

protected:

				DataPointSetDisplayMgr();

    TypeSet<ParentID>		availableviewers_;
    DataPointSetDisplayProp*	dispprop_	= nullptr;
};
