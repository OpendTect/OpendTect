#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

#include "binid.h"
#include "bufstringset.h"
#include "multiid.h"
#include "seistype.h"
#include "samplingdata.h"

class CBVSSeisTrcTranslator;
class IOObj;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcBufDataPack;
class uiComboBox;
class uiSeisTrcBufViewer;
class uiSeisBrowserInfoVwr;
class uiSpinBox;
class uiTable;
namespace ZDomain { class Def; }


mExpClass(uiSeis) uiSeisBrowser : public uiDialog
{ mODTextTranslationClass(uiSeisBrowser)
public :

    mExpClass(uiSeis) Setup : public uiDialog::Setup
    { mODTextTranslationClass(Setup)
    public:
			Setup(const MultiID&,Seis::GeomType);
			~Setup();

	mDefSetupMemb(MultiID,inpmid)
	mDefSetupMemb(MultiID,outmid)
	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(BinID,startpos)
	mDefSetupMemb(float,startz)
	mDefSetupMemb(Pos::GeomID,geomid)
	mDefSetupMemb(bool,locked)
    };
			uiSeisBrowser(uiParent*,const Setup&,bool);
			~uiSeisBrowser();

    bool		isOK() const			{ return tbl_; }
    void		setPos(const BinID&,bool);
    bool		doSetPos(const BinID&,bool force,bool veryfirst=false);
    void		setZ(float) ;
    void		setCrlWise( bool yn=true )	{ crlwise_ = yn; }
    bool		goTo(const BinID&);


    int			stepOut()			{ return stepout_; }
    void		setStepout(int);
    bool		is2D() const;

    const BinID&	curBinID() const;
    float		curZ() const;
    void		setCompNr( int compnr )		{ compnr_ = compnr; }

    static void		doBrowse(uiParent*,const IOObj&,bool is2d,
				 const Pos::GeomID& = {});

protected:

    const Setup		    setup_;
    uiSeisBrowserInfoVwr*   infovwr_	    = nullptr;
    uiSeisTrcBufViewer*     trcbufvwr_	    = nullptr;

    SeisTrcBuf&		    tbuf_;
    SeisTrcBuf&		    tbufchgdtrcs_;
    SeisTrc&		    ctrc_;
    CBVSSeisTrcTranslator*  tr_		    = nullptr;
    CBVSSeisTrcTranslator*  tro_	    = nullptr;
    CBVSSeisTrcTranslator*  tri_;
    const ZDomain::Def*     zdomdef_;

    bool		    crlwise_	    = false;
    bool		    saveenabled_    = false;
    bool		    is2d_;
    int			    crlwisebutidx_;
    int			    showwgglbutidx_;
    int			    editbutidx_     = mUdf(int);
    int			    savebutidx_     = mUdf(int);
    int			    saveasbutidx_   = mUdf(int);
    uiComboBox*		    selcompnmfld_;
    uiSpinBox*		    nrtrcsfld_;

    int			    stepout_	    = 25;

    uiTable*		    tbl_	    = nullptr;
    uiToolBar*		    uitb_	    = nullptr;
    int			    compnr_	    = 0;
    int			    nrcomps_	    = 1;
    BufferStringSet	    compnms_;
    int			    nrsamples_;
    SamplingData<float>     sd_		    = 0;

    bool		openData(const Setup&);
    void		createMenuAndToolBar();
    void		createTable();
    void		fillTable();
    void		fillUdf(SeisTrc&);
    void		fillTableColumn(const SeisTrc&,int);
    BinID		getNextBid(const BinID&,int,bool) const;
    void		addTrc(SeisTrcBuf&,const BinID&);
    void		updateWiggleButtonStatus();
    void		setTrcBufViewTitle();
    void		commitChanges(bool isnew);
    bool		storeChgdData(bool isnew);
    void		updateSaveButtonState(bool setactive);

    void		goToPush(CallBacker*);
    void		infoPush(CallBacker*);
    void		infoClose(CallBacker*);
    void		rightArrowPush(CallBacker*);
    void		leftArrowPush(CallBacker*);
    void		switchViewTypePush(CallBacker*);
    void		dispTracesPush(CallBacker*);
    void		trcbufViewerClosed(CallBacker*);
    void		trcselectionChanged(CallBacker*);
    void		valChgReDraw(CallBacker*);
    void		chgCompNrCB(CallBacker*);
    void		nrTracesChgCB(CallBacker*);
    void		saveChangesCB(CallBacker*);
    void		saveAsChangesCB(CallBacker*);
    void		editCB(CallBacker*);

private:

    SeisTrcBuf&		tbufbefore_;
    SeisTrcBuf&		tbufafter_;
};
