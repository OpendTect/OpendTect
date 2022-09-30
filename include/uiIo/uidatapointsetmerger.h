#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "bufstringset.h"
#include "datapointset.h"
#include "executor.h"

#include "uidialog.h"

class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiTable;

mExpClass(uiIo) DPSMergerProp
{ mODTextTranslationClass(DPSMergerProp);
public:
    typedef DataPackID	PackID;

				DPSMergerProp(const MultiID&,PackID primary,
					      PackID secondary);
				~DPSMergerProp();

   void				setColid(int primaryid,int secondaryid);

   enum MatchPolicy		{ Exact, Nearest, NoMatch };
   void				setMatchPolicy( MatchPolicy pol )
				{ matchpol_ = pol; }
   MatchPolicy			matchPolicy() const	{ return matchpol_; }

   enum ReplacePolicy		{ No, Yes, Average };
   void				setReplacePolicy( ReplacePolicy pol )
				{ replacepol_ = pol; }

   ReplacePolicy		replacePolicy() const	{ return replacepol_; }

   PackID			primaryDPID() const;
   PackID			secondaryDPID() const;
   const TypeSet<int>&		primaryColIDs() const;
   const TypeSet<int>&		secondaryColIDs() const;
   const MultiID&		newDPSID() const	{ return newdpsid_; }

   float			maxAllowedHorDist() const
				{ return maxhordist_; }
   void				setMaxAllowedHorDist( float maxdist )
				{ maxhordist_ = maxdist; }

   float			maxAllowedZDist() const { return maxz_; }
   void				setMaxAllowedZDist( float maxz )
				{ maxz_ = maxz; }

   bool				overWriteUndef() const
				{ return dooverwriteundef_; }
   void				setOverWriteUndef( bool yn )
				{ dooverwriteundef_ = yn; }
protected:

   MatchPolicy			matchpol_;
   ReplacePolicy		replacepol_;

   bool				dooverwriteundef_	= false;
   PackID			primarydpsid_;
   PackID			secondarydpsid_;
   TypeSet<int>			primarycolids_;
   TypeSet<int>			secondarycolids_;
   MultiID			newdpsid_;
   float			maxhordist_		= mUdf(float);
   float			maxz_			= mUdf(float);

public:
   mDeprecated("Use primaryDPID()")
   PackID			masterDPID() const
				{ return primarydpsid_; }
   mDeprecated("Use secondaryDPID()")
   PackID			slaveDPID() const
				{ return secondarydpsid_; }
   mDeprecated("Use primaryColIDs()")
   const TypeSet<int>&		masterColIDs() const
				{ return primarycolids_; }
   mDeprecated("Use secondaryColIDs()")
   const TypeSet<int>&		slaveColIDs() const
				{ return secondarycolids_; }
};


mExpClass(uiIo) DPSMerger : public Executor
{ mODTextTranslationClass(DPSMerger);
public:
				DPSMerger(const DPSMergerProp&);
				~DPSMerger();

    void			addNewCols(const BufferStringSet&);
    od_int64			nrDone() const override
				{ return rowdone_; }

    od_int64			totalNr() const override
				{ return sdps_->size(); }

    uiString			uiNrDoneText() const override
				{return uiStrings::phrJoinStrings(
				uiStrings::sPosition(mPlural),tr("processed"));}

    RefMan<DataPointSet>	getNewDPS()		{ return newdps_; }

protected:
    DPSMergerProp		prop_;
    RefMan<DataPointSet>	mdps_;
    RefMan<DataPointSet>	sdps_;
    RefMan<DataPointSet>	newdps_;
    int				rowdone_;

    int				nextStep() override;

    int				getSecondaryColID(int mcolid);
    mDeprecated("Use getSecondaryColID")
    int				getSlaveColID(int mcolid);
    DataPointSet::DataRow	getDataRow(int,int);
    DataPointSet::DataRow	getNewDataRow(int);
    int				findMatchingMrowID(int);
};


mExpClass(uiIo) uiDataPointSetMerger : public uiDialog
{ mODTextTranslationClass(uiDataPointSetMerger);
public:
				uiDataPointSetMerger(uiParent*,DataPointSet*,
						     DataPointSet*);
				~uiDataPointSetMerger();
protected:

    RefMan<DataPointSet>	mdps_;
    RefMan<DataPointSet>	sdps_;

    uiTable*			tbl_;
    uiComboBox*			matchpolfld_;
    uiComboBox*			replacepolfld_;
    uiGenInput*			overwritefld_;
    uiGenInput*			addcoloptfld_;
    uiGenInput*			distfld_;
    uiGenInput*			zgatefld_;
    uiIOObjSel*			outfld_;

    void			setTable();
    BufferStringSet		checkForNewColumns() const;
    void			checkForSameColNms(BufferStringSet&) const;
    bool			acceptOK(CallBacker*) override;
    void			attribChangedCB(CallBacker*);
    void			matchPolChangedCB(CallBacker*);
};
