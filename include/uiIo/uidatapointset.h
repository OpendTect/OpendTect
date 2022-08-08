#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "datapointset.h"
#include "bufstringset.h"
#include "iopar.h"

class uiTable;
class uiSpinBox;
class uiCheckBox;
class uiToolBar;
class uiIOObjSelDlg;
class uiStatsDisplayWin;
class uiDataPointSetCrossPlotWin;
class uiDPSDispPropDlg;
class uiVariogramDisplay;

class DataPointSetDisplayMgr;
class DataPointSetDisplayProp;
namespace Stats { template <class T> class RunCalc; }

/*!\brief Edit DataPointSet.

  The DataPointSet will be edited in-place. If you want to be able to
  rollback on user cancel, you'll have to make a copy of the original set
  yourself, and check the return value for the cancel.

 */


mExpClass(uiIo) uiDataPointSet : public uiDialog
{ mODTextTranslationClass(uiDataPointSet);
public:

    typedef int			TColID;
    typedef int			TRowID;
    typedef DataPointSet::ColID	DColID;
    typedef DataPointSet::RowID	DRowID;

    mStruct(uiIo) Setup : public uiDialog::Setup
    { mODTextTranslationClass(uiDataPointSet::Setup);
    public:
	Setup(const uiString& wintitl,bool ismodal=false);

	mDefSetupMemb(uiString,wintitle)	//!< "Extracted data"
	mDefSetupMemb(bool,isconst)		//!< false
	mDefSetupMemb(bool,canaddrow)		//!< false
	mDefSetupMemb(bool,directremove)	//!< false
	mDefSetupMemb(bool,allowretrieve)	//!< true
	mDefSetupMemb(int,initialmaxnrlines)	//!< 4000

    };


				uiDataPointSet(uiParent*,const DataPointSet&,
					      const Setup&,
					      DataPointSetDisplayMgr* mgr=0);
				~uiDataPointSet();

    DataPointSet&		pointSet()	{ return dps_; }
    const DataPointSet&		pointSet() const { return dps_; }

    bool			is2D() const;
    int				size() const	{ return drowids_.size(); }
				//!< number of displayable rows

    void			setZFactor( float f, const uiString& unnm )
				{ zfac_ = f; zunitnm_ = unnm; }
				//!< Default is SI().zDomain().userFactor()

    uiTable*			table()		{ return tbl_; }
    uiToolBar*			ioToolBar()	{ return iotb_; }
    uiToolBar*			dispToolBar()	{ return disptb_; }
    uiToolBar*			manipToolBar()	{ return maniptb_; }

    const char*			userName(DRowID did) const;
    Stats::RunCalc<float>&	getRunCalc(DColID) const;

    IOPar&			storePars()	{ return storepars_; }
    const IOPar&		storePars() const { return storepars_; }
    Notifier<uiDataPointSet>	valueChanged;

    Notifier<uiDataPointSet>	selPtsToBeShown;	// to show in 3D

    Notifier<uiDataPointSet>	rowAdded;
    CNotifier<uiDataPointSet,int> rowToBeRemoved;
    CNotifier<uiDataPointSet,int> rowRemoved;

    void			getChanges(DataPointSet::DataRow& before,
					   DataPointSet::DataRow& after) const
				{ before = beforechgdr_; after = afterchgdr_; }

    void			setCurrent(DColID,DRowID);
    void			setCurrent(const DataPointSet::Pos&,DColID);

				// Note that groups start at 1!
				// Thus bss.get(0) => group 1.
    void			setGroupNames(	const BufferStringSet& bss )
							{ grpnames_ = bss; }
    const BufferStringSet&	groupNames() const	{ return grpnames_; }
    const char*			groupName(int) const;
    void			setGroupType( const char* nm )
							{ grptype_ = nm; }
    const char*			groupType() const	{ return grptype_; }

    DRowID			dRowID(TRowID tid=-99) const;
    TRowID			tRowID(DRowID did=-99) const;
    DColID			dColID(TColID tid=-99) const;
    TColID			tColID(DColID did=-99) const;

    bool			isSelectionValid(DRowID) const;
    void			addRow(const DataPointSet::DataRow&);

    void			notifySelectedCell();
    void			reDoTable();
    bool			posDispTypeChgd()   { return posdisptypechgd_; }

    int			getSelectionGroupIdx(int selaareaid) const;

    const DataPointSetDisplayMgr* displayMgr() const	{ return dpsdispmgr_; }
    void			setDisplayMgr( DataPointSetDisplayMgr* dispmgr )
				{ dpsdispmgr_ = dispmgr; }

    void			calcSelectedness();
    void			setDisp(DataPointSetDisplayProp*);
    void			showXY(bool yn);
    void			showZ(bool yn);

    mDeclInstanceCreatedNotifierAccess(uiDataPointSet);

protected:

    DataPointSet&		dps_;
    Setup			setup_;
    float			zfac_;
    uiString			zunitnm_;
    IOPar			storepars_;
    BufferStringSet		grpnames_;
    BufferString		grptype_;

    TypeSet<DRowID>		drowids_;
    TypeSet<TRowID>		trowids_;	//!< often -1
    TypeSet<TRowID>		sortidxs_;
    TypeSet<TRowID>		revsortidxs_;
    float			eachrow_;
    float			plotpercentage_;
    float			percentage_;
    TColID			xcol_;
    TColID			ycol_;
    TColID			y2col_;
    TColID			sortcol_;
    TColID			statscol_;
    mutable ObjectSet<Stats::RunCalc<float> > runcalcs_;
    DataPointSet::DataRow	beforechgdr_;
    DataPointSet::DataRow	afterchgdr_;
    bool			unsavedchgs_;
    bool			fillingtable_;
    bool			showbids_;
    bool			posdisptypechgd_;

    DataPointSetDisplayMgr*	dpsdispmgr_;

    static const char*		sKeyMinDPPts()
				{ return "Minimum pts for Density Plot"; }
    uiTable*			tbl_;
    uiToolBar*			iotb_;
    uiToolBar*			disptb_;
    uiToolBar*			maniptb_;
    uiSpinBox*			percfld_;
    uiCheckBox*			showbidsfld_;
    int				xplottbid_;
    int				dispxytbid_;
    int				dispztbid_;
    uiIOObjSelDlg*		curseldlg_;
    uiDPSDispPropDlg*		dpsdisppropdlg_;

    void			mkToolBars();

				// default returns current row/col
    float			getVal(DColID,DRowID,bool userunits) const;

    void			calcIdxs();
    void			calcSortIdxs();
    void			redoAll();
    void			updColNames();
    void			fillPos(TRowID);
    void			fillData(TRowID);
    void			handleAxisColChg();
    void			removeHiddenRows();
    bool			saveOK();
    bool			doSave();
    void			setSortedCol(TColID);
    void			unSelXCol();

    void			rowAddedCB(CallBacker*);
    void			initWin(CallBacker*);
    void			selXCol(CallBacker*);
    void			selYCol(CallBacker*);
    void			unSelYCol(CallBacker*);
    void			colStepL(CallBacker*);
    void			colStepR(CallBacker*);
    void			rowClicked(CallBacker*);
    void			colClicked(CallBacker*);
    void			selChg(CallBacker*);
    void			valChg(CallBacker*);
    void			eachChg(CallBacker*);
    void			toggleXYZ(CallBacker*);
    void			setSortCol(CallBacker*);
    void			showCrossPlot(CallBacker*);
    void			showStatsWin(CallBacker*);
    void			retrieve(CallBacker*);
    void			save(CallBacker*);
    void			manage(CallBacker*);
    void			delSelRows(CallBacker*);
    void			showStatusMsg(CallBacker*);
    void			closeNotify(CallBacker*);
    void			showSelPts(CallBacker*);
    void			removeSelPts(CallBacker*);
    void			addColumn(CallBacker*);
    void			removeColumn(CallBacker*);
    void			compVertVariogram(CallBacker*);
    void                        chgPosDispType(CallBacker*);

    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
    void			showPtsInWorkSpace(CallBacker*);
    void			applClosingCB(CallBacker*);

    friend class		uiDataPointSetCrossPlotter;
    uiDataPointSetCrossPlotWin*	xplotwin_;
    void			xplotSelChg(CallBacker*);
    void			xplotRemReq(CallBacker*);
    void			xplotClose(CallBacker*);
    void			getXplotPos(DColID&,DRowID&) const;

    uiStatsDisplayWin*		statswin_;
    void			statsClose(CallBacker*);
    void			showStats(DColID);

    bool			isDisp(bool) const;
    void			handleSelRows();
    void			setStatsMarker(DRowID);
    void			handleGroupChg(DRowID);

    ObjectSet<uiVariogramDisplay>       variodlgs_;

private:

    int				initVars();

public:
    float			getValue( DColID did, DRowID rid ,
					  bool userunits ) const
				{ return getVal(did,rid,userunits); }
    void			setUnsavedChg( bool chg )
				{ unsavedchgs_ = chg; }
    int				nrPosCols() const;
};


