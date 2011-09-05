#ifndef uidatapointsetmerger_h
#define uidatapointsetmerger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
 RCS:           $Id: uidatapointsetmerger.h,v 1.2 2011-09-05 10:40:16 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "uidialog.h"
#include "ctxtioobj.h"
#include "datapointset.h"

class uiComboBox;
class uiIOObjSel;
class uiTable;

/*! \brief
CrossPlot manager
*/

mClass DPSMergerProp 
{
public:
    				DPSMergerProp( const MultiID& id, int mid,
					       int sid )
				    : masterdpsid_(mid), slavedpsid_(sid)
			       	    , newdpsid_(id)	{}
   void				setColid(int masterid,int slaveid);
   
   enum	MatchPolicy		{ Exact, Nearest, NoMatch };
   void				setMatchPolicy( MatchPolicy pol )
       				{ matchpol_ = pol; }
   MatchPolicy			matchPolicy() const	{ return matchpol_; }
   
   enum	ReplacePolicy		{ No, Yes, Average };
   void				setReplacePolicy( ReplacePolicy pol )
       				{ replacepol_ = pol; }

   ReplacePolicy		replacePolicy() const	{ return replacepol_; }

   int 				masterDPID() const	{ return masterdpsid_; }
   int 				slaveDPID() const	{ return slavedpsid_; }
   const MultiID&		newDPSID() const	{ return newdpsid_; }
   const TypeSet<int>&		masterColIDs() const	{ return mastercolids_;}
   const TypeSet<int>&		slaveColIDs() const	{ return slavecolids_; }
protected:

   MatchPolicy			matchpol_;
   ReplacePolicy		replacepol_;

   int 				masterdpsid_;
   int 				slavedpsid_;
   TypeSet<int>			mastercolids_;
   TypeSet<int>			slavecolids_;
   MultiID			newdpsid_;
};


mClass DPSMerger : public Executor
{
public:
    				DPSMerger(const DPSMergerProp&);
    
    od_int64			nrDone() const		{ return rowdone_; }
    od_int64			totalNr() const		{ return sdps_->size();}
    const char*			nrDoneText() const
    				{ return "Postion processed"; }
    DataPointSet*		getNewDPS()		{ return newdps_; }
protected:
    DPSMergerProp		prop_;
    DataPointSet*		mdps_;
    DataPointSet*		sdps_;
    DataPointSet*		newdps_;
    int 			rowdone_;

    int 			nextStep();

    void 			addNewCols();
    int 			getSlaveColID(int mcolid);
    DataPointSet::DataRow 	getDataRow(int,int);
    DataPointSet::DataRow 	getNewDataRow(int);
    int				findMatchingMrowID(int);
};


mClass uiDataPointSetMerger : public uiDialog
{
public:
    				uiDataPointSetMerger(uiParent*,DataPointSet*,
						     DataPointSet*);
				~uiDataPointSetMerger();
protected:

    DataPointSet*		mdps_;
    DataPointSet*		sdps_;
    CtxtIOObj			ctio_;

    uiTable*			tbl_;
    uiComboBox*			matchpolfld_;
    uiComboBox*			replacepolfld_;
    uiIOObjSel*			outfld_;

    void			setTable();
    bool			acceptOK(CallBacker*);
    void			matchPolChangedCB(CallBacker*);
};

#endif
