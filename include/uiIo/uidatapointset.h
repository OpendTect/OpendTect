#ifndef uidatapointset_h
#define uidatapointset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uidatapointset.h,v 1.6 2008-04-09 07:05:35 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "datapointset.h"
#include "rowcol.h"
#include "iopar.h"
class uiTable;
class uiSpinBox;
class uiToolBar;
class uiStatsDisplayWin;
class uiDataPointSetCrossPlotWin;
namespace Stats { template <class T> class RunCalc; }

/*!\brief Edit DataPointSet */


class uiDataPointSet : public uiDialog
{ 	
public:

    typedef int			TColID;
    typedef int			TRowID;
    typedef DataPointSet::ColID	DColID;
    typedef DataPointSet::RowID	DRowID;

    struct Setup : public uiDialog::Setup
    {
				Setup(const char* wintitl,bool ismodal=false);

	mDefSetupMemb(BufferString,wintitle)	//!< "Extracted data"
	mDefSetupMemb(bool,isconst)		//!< false
	mDefSetupMemb(bool,allowretrieve)	//!< true
	mDefSetupMemb(int,initialmaxnrlines)	//!< 4000

    };


				uiDataPointSet(uiParent*,const DataPointSet&,
					      const Setup&);
				~uiDataPointSet();

    DataPointSet&		pointSet()	{ return dps_; }
    const DataPointSet&		pointSet() const { return dps_; }

    bool			is2D() const;
    int				size() const	{ return drowids_.size(); }
    				//!< number of displayable rows

    void			setZFactor( float f, const char* unnm )
    				{ zfac_ = f; zunitnm_ = unnm; }
					//!< Default is SI().zFactor()

    static void			createNotify(const CallBack&);
				    //!< Will be called at end of constructor
				    //!< Makes it possible to add buttons etc
    static void			stopCreateNotify(CallBacker*);

    uiTable*			table()		{ return tbl_; }
    uiToolBar*			ioToolBar()	{ return iotb_; }
    uiToolBar*			dispToolBar()	{ return disptb_; }
    uiToolBar*			manipToolBar()	{ return maniptb_; }

    const char*			userName(DRowID did) const;
    Stats::RunCalc<float>&	getRunCalc(DColID) const;

    IOPar&			storePars()	{ return storepars_; }
    Notifier<uiDataPointSet>	valueChanged;
    void			getChanges(DataPointSet::DataRow& before,
	    				   DataPointSet::DataRow& after) const
				{ before = beforechgdr_; after = afterchgdr_; }

    void			setCurrent(DColID,DRowID);
    void			setCurrent(const DataPointSet::Pos&,DColID);

protected:

    DataPointSet&		dps_;
    DataPointSet		orgdps_;
    Setup			setup_;
    float			zfac_;
    BufferString		zunitnm_;
    IOPar			storepars_;

    TypeSet<DRowID>		drowids_;
    TypeSet<TRowID>		trowids_;	//!< often -1
    TypeSet<TRowID>		sortidxs_;
    TypeSet<TRowID>		revsortidxs_;
    int				eachrow_;
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

    uiTable*			tbl_;
    uiToolBar*			iotb_;
    uiToolBar*			disptb_;
    uiToolBar*			maniptb_;
    uiSpinBox*			eachfld_;
    int				xplottbid_;
    int				dispxytbid_;
    int				dispztbid_;

    void			mkToolBars();

    				// default returns current row/col
    DRowID			dRowID(TRowID tid=-99) const;
    TRowID			tRowID(DRowID did=-99) const;
    DColID			dColID(TColID tid=-99) const;
    TColID			tColID(DColID did=-99) const;
    float			getVal(DColID,DRowID,bool userunits) const;

    void			calcIdxs();
    void			calcSortIdxs();
    void			redoAll();
    void			updColNames();
    void			fillPos(TRowID);
    void			fillData(TRowID);
    void			handleAxisColChg();
    bool			saveOK();
    bool			doSave();

    void			selXCol(CallBacker*);
    void			selYCol(CallBacker*);
    void			unSelCol(CallBacker*);
    void			colStepL(CallBacker*);
    void			colStepR(CallBacker*);
    void			rowSel(CallBacker*);
    void			valChg(CallBacker*);
    void			eachChg(CallBacker*);
    void			toggleXYZ(CallBacker*);
    void			setSortCol(CallBacker*);
    void			showCrossPlot(CallBacker*);
    void			showStatsWin(CallBacker*);
    void			retrieve(CallBacker*);
    void			save(CallBacker*);
    void			delSelRows(CallBacker*);

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);

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

private:

    int				initVars();
};


#endif
