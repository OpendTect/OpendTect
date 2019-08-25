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
#include "seistype.h"
#include "survgeom.h"

class SeisTrc;
class SeisTrcBuf;
class uiComboBox;
class uiSeisTrcBufViewer;
class uiSeisSampleEditorInfoVwr;
class uiSpinBox;
class uiTable;
namespace Pos { class IdxPairDataSet; }
namespace PosInfo { class CubeData; class LineData; }
namespace Seis { class Provider; class Provider2D; class Provider3D; }


mExpClass(uiSeis) uiSeisSampleEditor : public uiDialog
{ mODTextTranslationClass(uiSeisSampleEditor)
public :

    mExpClass(uiSeis) Setup : public uiDialog::Setup
    { mODTextTranslationClass(uiSeisSampleEditor::Setup)
    public:
				Setup(const DBKey&,
				      Pos::GeomID geomid=mUdfGeomID);

	mDefSetupMemb(DBKey,	id)
	mDefSetupMemb(Pos::GeomID,geomid)
	mDefSetupMemb(bool,	readonly)
	mDefSetupMemb(BinID,	startpos)
	mDefSetupMemb(float,	startz)

    };
			uiSeisSampleEditor(uiParent*,const Setup&);
			~uiSeisSampleEditor();

    bool		isOK() const		{ return prov_; }
    bool		is2D() const		{ return is2d_; }
    Seis::GeomType	geomType() const;

    void		setCrlWise(bool yn)	{ crlwise_ = yn; }
    int			stepOut()		{ return stepout_; }
    void		setStepout(int);
    Bin2D		curBin2D() const;
    BinID		curBinID() const;
    bool		setPos(const BinID&);
    bool		setPos(const Bin2D&);
    float		curZ() const;
    void		setZ(float);
    int			compNr() const		{ return compnr_; }
    void		setCompNr(int);

    static void		launch(uiParent*,const DBKey&,
				 Pos::GeomID geomid=mUdfGeomID);

protected:

    uiComboBox*		selcompnmfld_;
    uiSpinBox*		nrtrcsfld_;
    uiTable*		tbl_;
    uiToolBar*		toolbar_;

    const Setup		setup_;
    Seis::Provider*	prov_;
    SeisTrcBuf&		tbuf_;
    SeisTrcBuf&		viewtbuf_;
    Pos::IdxPairDataSet& edtrcs_;
    SeisTrc*		ctrc_;
    const SurvGeom*	survgeom_;
    ZDomain::Def	zdomdef_;
    PosInfo::CubeData&	cubedata_;
    PosInfo::LineData&	linedata_;

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
    uiSeisSampleEditorInfoVwr* infovwr_;
    uiSeisTrcBufViewer*	trcbufvwr_;

    int			nrComponents() const	{ return compnms_.size(); }
    TrcKey		trcKey4BinID(const BinID&) const;
    SeisTrc*		curTrace(bool);
    BinID		getBinID4RelIdx(const BinID&,int) const;

    void		createMenuAndToolBar();
    void		createTable();
    void		clearEditedTraces();
    void		fillTable();
    void		fillUdf(SeisTrc&);
    void		fillTableColumn(const SeisTrc&,int);
    void		commitChanges();
    bool		doSetPos(const BinID&,bool force);
    void		addTrc(const BinID&);
    void		updateWiggleButtonStatus();
    void		setTrcBufViewTitle();
    bool		storeChgdData();
    bool		goTo( const BinID& bid )
			{ return doSetPos(bid,true); }

    void		goToPush(CallBacker*);
    void		infoPush(CallBacker*);
    void		infoClose(CallBacker*)	    { infovwr_ = 0; }
    void		bufVwrClose(CallBacker*);
    void		rightArrowPush(CallBacker*);
    void		leftArrowPush(CallBacker*);
    void		switchViewTypePush(CallBacker*);
    void		dispTracesPush(CallBacker*);
    void		selChgCB(CallBacker*);
    void		tblValChgCB(CallBacker*);
    void		chgCompNrCB(CallBacker*);
    void		nrTracesChgCB(CallBacker*);

    bool		acceptOK();

    friend class	uiSeisSampleEditorGoToDlg;
    friend class	uiSeisSampleEditorWriter;
    friend class	uiSeisSampleEditorInfoVwr;

};
