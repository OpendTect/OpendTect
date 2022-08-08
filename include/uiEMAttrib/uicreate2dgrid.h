#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2009
________________________________________________________________________

-*/


#include "uiemattribmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "grid2d.h"

class BufferStringSet;
class TrcKeyZSampling;
class TrcKeySampling;
class uiBatchJobDispatcherSel;
class uiCheckBox;
class uiGenInput;
class uiGrid2DMapObject;
class uiHorizonParSel;
class uiIOObjSel;
class uiLabel;
class uiPosSubSel;
class uiSeisSel;
class uiSelNrRange;
class uiSurveyMap;

namespace Geometry { class RandomLine; }

mExpClass(uiEMAttrib) ui2DGridLines : public uiGroup
{ mODTextTranslationClass(ui2DGridLines);
public:
				~ui2DGridLines();

    const Grid2D*		getGridLines() const	{ return grid_; }
    virtual void		updateRange();
    virtual const Grid2D::Line*	getBaseLine() const	{ return 0; }
    virtual void		getNrLinesLabelTexts(BufferString&,
	    					     BufferString&) const  = 0;

    virtual bool		fillPar(IOPar&) const;

    Notifier<ui2DGridLines>	gridChanged;

protected:

				ui2DGridLines(uiParent*,const TrcKeySampling&);

    virtual bool		computeGrid()			= 0;

    uiGenInput*			inlprefixfld_;
    uiGenInput*			crlprefixfld_;

    Grid2D*			grid_;
    const TrcKeySampling&		hs_;
};


mExpClass(uiEMAttrib) ui2DGridLinesFromInlCrl : public ui2DGridLines
{ mODTextTranslationClass(ui2DGridLinesFromInlCrl)
public:
    				ui2DGridLinesFromInlCrl(uiParent*,
							const TrcKeySampling&);

    bool			fillPar(IOPar&) const override;
    void			updateRange() override;
    void			getLineNames(BufferStringSet&) const;

protected:

    bool			computeGrid() override;
    void			paramsChgCB(CallBacker*);
    void			getNrLinesLabelTexts(BufferString&,
						 BufferString&) const override;
    void			modeChg(CallBacker*);

    uiGenInput*			inlmodefld_;
    uiGenInput*			crlmodefld_;
    uiGenInput*			inlsfld_;
    uiGenInput*			crlsfld_;
    uiSelNrRange*		inlrgfld_;
    uiSelNrRange*		crlrgfld_;
};


mExpClass(uiEMAttrib) ui2DGridLinesFromRandLine : public ui2DGridLines
{ mODTextTranslationClass(ui2DGridLinesFromRandLine)
public:
    				ui2DGridLinesFromRandLine(uiParent*,
						const TrcKeySampling&,
						const Geometry::RandomLine*);
    				~ui2DGridLinesFromRandLine();

    const Grid2D::Line*		getBaseLine() const override
				    { return baseline_; }

    bool			fillPar(IOPar&) const override;
    void			getLineNames(BufferStringSet&) const;
    bool			hasLine() const;
    void			processInput();

protected:

    bool			computeGrid() override;
    void			paramsChgCB(CallBacker*);
    void			getNrLinesLabelTexts(BufferString&,
						 BufferString&) const override;

    uiIOObjSel*			rdlfld_;
    uiGenInput*			pardistfld_;
    uiGenInput*			perdistfld_;

    Grid2D::Line*		baseline_;
};


mExpClass(uiEMAttrib) uiCreate2DGrid : public uiDialog
{ mODTextTranslationClass(uiCreate2DGrid)
public:
				uiCreate2DGrid(uiParent*,
					       const Geometry::RandomLine*);
				~uiCreate2DGrid();

    bool			fillPar();

protected:

    uiGroup*			createSeisGroup(const Geometry::RandomLine*);
    uiGroup*			createHorizonGroup();
    uiGroup*			createPreviewGroup();
    void			updatePreview(CallBacker*);

    uiSeisSel*			infld_;
    uiSeisSel*			outfld_;
    uiPosSubSel*		bboxfld_;
    uiGenInput*			sourceselfld_;

    ui2DGridLines*		inlcrlgridgrp_;
    ui2DGridLines*		randlinegrdgrp_;
    uiGrid2DMapObject*		preview_;
    uiSurveyMap*		previewmap_;
    uiLabel*			nrinlinesfld_;
    uiLabel*			nrcrlinesfld_;

    uiCheckBox*			horcheckfld_;
    uiHorizonParSel*		horselfld_;
    uiGenInput*			hornmfld_;
    uiBatchJobDispatcherSel*	batchfld_;

    TrcKeyZSampling&		tkzs_;

    void			horCheckCB(CallBacker*);
    void			inpSelCB(CallBacker*);
    void			outSelCB(CallBacker*);
    void			srcSelCB(CallBacker*);
    void			subSelCB(CallBacker*);
    void			gridChgCB(CallBacker*);
    void			finalizeCB(CallBacker*);

    void			fillSeisPar(IOPar&);
    void			fillHorPar(IOPar&);

    mDeprecated			("Use checkLineNames()")
    bool			checkInput(IOPar&) const;

    int				checkLineNames() const;
    bool			acceptOK(CallBacker*) override;
};


