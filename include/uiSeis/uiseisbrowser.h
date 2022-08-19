#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

#include "bufstringset.h"
#include "linekey.h"
#include "multiid.h"
#include "position.h"
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
	mDefSetupMemb(MultiID,id)
	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(BinID,startpos)
	mDefSetupMemb(float,startz)
	mDefSetupMemb(LineKey,linekey)
	mDefSetupMemb(bool,readonly)	//!< def: true

    };
			uiSeisBrowser(uiParent*,const Setup&,bool);
			~uiSeisBrowser();

    bool		isOK() const			{ return tbl_; }
    void		setPos(const BinID&,bool);
    bool		doSetPos(const BinID&,bool force,bool veryfirst=false);
    void		setZ(float) ;
    void		setCrlWise( bool yn=true )	{ crlwise_ = yn; }
    void		commitChanges();
    bool		storeChgdData();
    bool		goTo(const BinID&);


    int			stepOut()			{ return stepout_; }
    void		setStepout(int);
    bool		is2D() const;
    bool		is2d_;

    const BinID&	curBinID() const;
    float		curZ() const;
    void		setCompNr( int compnr )		{ compnr_ = compnr; }

    static void		doBrowse(uiParent*,const IOObj&,bool is2d,
				 const LineKey* lk=0);

protected:

    const Setup		setup_;
    uiSeisBrowserInfoVwr* infovwr_;
    uiSeisTrcBufViewer*	trcbufvwr_;

    SeisTrcBuf&		tbuf_;
    SeisTrcBuf&		tbufchgdtrcs_;
    SeisTrc&		ctrc_;
    CBVSSeisTrcTranslator* tr_;
    CBVSSeisTrcTranslator* tro_;
    CBVSSeisTrcTranslator* tri_;
    const ZDomain::Def*	zdomdef_;

    bool		crlwise_;
    int			crlwisebutidx_;
    int			showwgglbutidx_;
    uiComboBox*		selcompnmfld_;
    uiSpinBox*		nrtrcsfld_;

    int			stepout_;

    uiTable*		tbl_;
    uiToolBar*		uitb_;
    int			compnr_;
    int			nrcomps_;
    BufferStringSet	compnms_;
    int			nrsamples_;
    SamplingData<float>	sd_;

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

    void		goToPush(CallBacker*);
    void		infoPush(CallBacker*);
    void		infoClose(CallBacker*);
    void		rightArrowPush(CallBacker*);
    void		leftArrowPush(CallBacker*);
    void		switchViewTypePush(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		dispTracesPush(CallBacker*);
    void		trcbufViewerClosed(CallBacker*);
    void		trcselectionChanged(CallBacker*);
    void		valChgReDraw(CallBacker*);
    void		chgCompNrCB(CallBacker*);
    void		nrTracesChgCB(CallBacker*);

private:

    SeisTrcBuf&		tbufbefore_;
    SeisTrcBuf&		tbufafter_;
};
