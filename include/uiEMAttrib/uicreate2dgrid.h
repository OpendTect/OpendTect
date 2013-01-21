#ifndef uicreate2dgrid_h
#define uicreate2dgrid_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2009
 RCS:		$Id$
________________________________________________________________________

-*/


#include "uiemattribmod.h"
#include "uibatchlaunch.h"
#include "uigroup.h"
#include "grid2d.h"

class BufferStringSet;
class CubeSampling;
class HorSampling;
class IOPar;
class uiCheckBox;
class uiGenInput;
class uiGrid2DMapObject;
class uiIOObjSel;
class uiIOObjSelGrp;
class uiPosSubSel;
class uiSeisSel;
class uiSelNrRange;
class uiSurveyMap;

namespace Geometry { class RandomLine; }

mExpClass(uiEMAttrib) ui2DGridLines : public uiGroup
{
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

    				ui2DGridLines(uiParent*,const HorSampling&);

    virtual bool		computeGrid()			= 0;

    uiGenInput*			inlprefixfld_;
    uiGenInput*			crlprefixfld_;

    Grid2D*			grid_;
    const HorSampling&		hs_;
};


mExpClass(uiEMAttrib) ui2DGridLinesFromInlCrl : public ui2DGridLines
{
public:
    				ui2DGridLinesFromInlCrl(uiParent*,
							const HorSampling&);

    bool			fillPar(IOPar&) const;
    void			updateRange();
    void			getLineNames(BufferStringSet&) const;

protected:

    bool			computeGrid();
    void			paramsChgCB(CallBacker*);
    void			getNrLinesLabelTexts(BufferString&,
	    					     BufferString&) const;
    void			modeChg(CallBacker*);

    uiGenInput*			inlmodefld_;
    uiGenInput*			crlmodefld_;
    uiGenInput*			inlsfld_;
    uiGenInput*			crlsfld_;
    uiSelNrRange*		inlrgfld_;
    uiSelNrRange*		crlrgfld_;
};


mExpClass(uiEMAttrib) ui2DGridLinesFromRandLine : public ui2DGridLines
{
public:
    				ui2DGridLinesFromRandLine(uiParent*,
						const HorSampling&,
						const Geometry::RandomLine*);
    				~ui2DGridLinesFromRandLine();

    const Grid2D::Line*		getBaseLine() const	{ return baseline_; }
    bool			fillPar(IOPar&) const;
    void			getLineNames(BufferStringSet&) const;

protected:

    bool			computeGrid();
    void			paramsChgCB(CallBacker*);
    void			getNrLinesLabelTexts(BufferString&,
	    					     BufferString&) const;

    uiIOObjSel*			rdlfld_;
    uiGenInput*			pardistfld_;
    uiGenInput*			perdistfld_;

    Grid2D::Line*		baseline_;
};


mExpClass(uiEMAttrib) uiCreate2DGrid : public uiFullBatchDialog
{
public:
				uiCreate2DGrid(uiParent*,
					       const Geometry::RandomLine*);
				~uiCreate2DGrid();

    bool			fillPar(IOPar&);
    bool			prepareProcessing()	{ return true; }

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
    uiIOObjSelGrp*		horselfld_;
    uiGenInput*			hornmfld_;

    CubeSampling&		cs_;

    void			horCheckCB(CallBacker*);
    void			inpSelCB(CallBacker*);
    void			outSelCB(CallBacker*);
    void			srcSelCB(CallBacker*);
    void			subSelCB(CallBacker*);
    void			gridChgCB(CallBacker*);
    void			finaliseCB(CallBacker*);

    void			fillSeisPar(IOPar&);
    void			fillHorPar(IOPar&);

    bool			checkInput() const;

};


#endif

