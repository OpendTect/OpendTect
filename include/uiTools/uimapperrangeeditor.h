#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uihistogramsel.h"

class uiPixmapItem;

namespace ColTab { class MapperSetup; class Sequence; }

mExpClass(uiTools) uiMapperRangeEditor : public uiHistogramSel
{
public:
				uiMapperRangeEditor(uiParent*,int id,
					bool fixdrawrg=true);
				~uiMapperRangeEditor();

    void			setEmpty();

    void			setColTabMapperSetup(
					const ColTab::MapperSetup&);
    const ColTab::MapperSetup&	getColTabMapperSetup()	{ return *ctmapper_; }
    void			setColTabSeq(const ColTab::Sequence&);
    const ColTab::Sequence&	getColTabSeq() const	{ return *ctseq_; }

    Notifier<uiMapperRangeEditor>	sequenceChanged;

protected:

    ColTab::MapperSetup*	ctmapper_;
    ColTab::Sequence*		ctseq_;

    uiPixmapItem*		leftcoltab_;
    uiPixmapItem*		centercoltab_;
    uiPixmapItem*		rightcoltab_;

    uiLineItem*			ctminline_;
    uiLineItem*			ctmaxline_;

    void 			initPixmaps();
    void			drawLines() override;
    void			drawPixmaps() override;
    void			makeSymmetricalIfNeeded(bool) override;
    void			useClipRange() override;

    void			wheelMoved(CallBacker*);
};
