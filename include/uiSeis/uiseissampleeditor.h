#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Stayaki / Bert
 Date:          Oct 2007 / May 2017
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "bufstringset.h"
#include "dbkey.h"
#include "position.h"
#include "samplingdata.h"
#include "zdomain.h"

class SeisTrc;
class SeisTrcBuf;
class uiComboBox;
class uiSeisTrcBufViewer;
class uiSeisSampleEditorInfoVwr;
class uiSpinBox;
class uiTable;
namespace Pos { class IdxPairDataSet; }
namespace PosInfo { class CubeData; class Line2DData; }
namespace Seis { class Provider; class Provider2D; class Provider3D; }


mExpClass(uiSeis) uiSeisSampleEditor : public uiDialog
{ mODTextTranslationClass(uiSeisSampleEditor)
public :

    mExpClass(uiSeis) Setup : public uiDialog::Setup
    { mODTextTranslationClass(uiSeisSampleEditor::Setup)
    public:
				Setup(const DBKey&);

	mDefSetupMemb(DBKey,	id)
	mDefSetupMemb(Pos::GeomID,geomid)
	mDefSetupMemb(bool,	readonly)
	mDefSetupMemb(TrcKey,	startpos)
	mDefSetupMemb(float,	startz)

    };
			uiSeisSampleEditor(uiParent*,const Setup&);
			~uiSeisSampleEditor();

    bool		isOK() const			{ return prov_; }
    void		setPos(const BinID&);
    void		setCrlWise(bool yn)		{ crlwise_ = yn; }
    void		commitChanges();
    bool		storeChgdData();
    bool		goTo(const BinID&);

    int			stepOut()			{ return stepout_; }
    void		setStepout(int);
    bool		is2D() const;

    const BinID&	curBinID() const;
    float		curZ() const;
    void		setCompNr( int compnr )		{ compnr_ = compnr; }

    static void		doBrowse(uiParent*,const DBKey&,
				 Pos::GeomID geomid=mUdfGeomID);

protected:

    uiComboBox*		selcompnmfld_;
    uiSpinBox*		nrtrcsfld_;
    uiTable*		tbl_;
    uiToolBar*		toolbar_;

    const Setup		setup_;
    uiSeisSampleEditorInfoVwr* infovwr_;
    uiSeisTrcBufViewer*	trcbufvwr_;
    SeisTrcBuf&		tbuf_;
    Pos::IdxPairDataSet& changedtraces_;
    SeisTrc*		ctrc_;
    Seis::Provider*	prov_;
    ZDomain::Def	zdomdef_;
    PosInfo::CubeData&	cubedata_;
    PosInfo::Line2DData& linedata_;

    Seis::DataType	datatype_;
    bool		is2d_;
    int			compnr_;
    BufferStringSet	compnms_;
    int			nrsamples_;
    SamplingData<float>	sampling_;
    BinID		stepbid_;
    int			stepout_;
    bool		crlwise_;
    int			crlwisebutidx_;
    int			showwgglbutidx_;
    uiString		toinlwisett_;
    uiString		tocrlwisett_;

    int			nrComponents() const	{ return compnms_.size(); }
    Seis::Provider2D&	prov2D();
    Seis::Provider3D&	prov3D();

    void		createMenuAndToolBar();
    void		createTable();
    void		clearEditedTraces();
    void		fillTable();
    void		fillUdf(SeisTrc&);
    void		fillTableColumn(const SeisTrc&,int);
    void		doSetPos(const BinID&,bool force);
    void		addTrc(SeisTrcBuf&,const BinID&);
    void		updateWiggleButtonStatus();
    void		setTrcBufViewTitle();

    void		goToPush(CallBacker*);
    void		infoPush(CallBacker*);
    void		infoClose(CallBacker*);
    void		rightArrowPush(CallBacker*);
    void		leftArrowPush(CallBacker*);
    void		switchViewTypePush(CallBacker*);
    bool		acceptOK();
    void		dispTracesPush(CallBacker*);
    void		trcbufViewerClosed(CallBacker*);
    void		trcselectionChanged(CallBacker*);
    void		tblValChgCB(CallBacker*);
    void		chgCompNrCB(CallBacker*);
    void		nrTracesChgCB(CallBacker*);

private:

    SeisTrcBuf&		tbufbefore_;
    SeisTrcBuf&		tbufafter_;
};
