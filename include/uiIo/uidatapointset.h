#ifndef uidatapointset_h
#define uidatapointset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uidatapointset.h,v 1.2 2008-04-04 12:40:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "rowcol.h"
#include "iopar.h"
class DataPointSet;
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

    typedef int			TRowID;
    typedef int			TColID;
    typedef int			DRowID;
    typedef int			DColID;

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

    DataPointSet&		pointSet()	{ return *dps_; }
    const DataPointSet&		pointSet() const { return *dps_; }

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

protected:

    DataPointSet*		dps_;
    Setup			setup_;
    float			zfac_;
    BufferString		zunitnm_;
    IOPar			storepars_;

    TypeSet<DRowID>		drowids_;
    TypeSet<TRowID>		trowids_;	//!< often -1
    TypeSet<TRowID>		sortidxs_;
    int				eachrow_;
    bool			dispxy_;
    TColID			xcol_;
    TColID			ycol_;
    TColID			y2col_;
    TColID			sortcol_;
    TColID			statscol_;
    mutable ObjectSet<Stats::RunCalc<float> > runcalcs_;

    uiTable*			tbl_;
    uiToolBar*			iotb_;
    uiToolBar*			disptb_;
    uiToolBar*			maniptb_;
    uiSpinBox*			eachfld_;
    int				xyictbid_;
    int				xplottbid_;
    int				statstbid_;

    void			mkToolBars();

    				// default returns current row/col
    DRowID			dRowID(TRowID tid=-99) const;
    TRowID			tRowID(DRowID did=-99) const;
    DColID			dColID(TColID tid=-99) const;
    TColID			tColID(DColID did=-99) const;
    int				nrPosCols() const	{ return is2D()?4:3; }
    float			getVal(DColID,DRowID) const;
    void			setVal(DColID,DRowID,float); //!< col >= 0

    void			calcIdxs();
    void			calcSortIdxs();
    void			redoAll();
    void			updColNames();
    void			fillPos(TRowID);
    void			fillData(TRowID);
    void			handleAxisColChg();

    void			selXCol(CallBacker*);
    void			selYCol(CallBacker*);
    void			unSelCol(CallBacker*);
    void			colStepL(CallBacker*);
    void			colStepR(CallBacker*);
    void			valChg(CallBacker*);
    void			eachChg(CallBacker*);
    void			toggXYIC(CallBacker*);
    void			setSortCol(CallBacker*);
    void			showCrossPlot(CallBacker*);
    void			showStatsWin(CallBacker*);
    void			retrieve(CallBacker*);
    void			save(CallBacker*);

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);

    friend class		uiDataPointSetCrossPlotter;
    uiDataPointSetCrossPlotWin*	xplotwin_;
    void			xplotSelChg(CallBacker*);
    void			xplotRemReq(CallBacker*);
    void			xplotClose(CallBacker*);
    void			getXplotPos(DRowID&,DColID&) const;

    uiStatsDisplayWin*		statswin_;
    void			statsClose(CallBacker*);
    void			showStats(DColID);

private:

    DataPointSet*		localdps_;
    int				initVars();
};


#endif
