#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
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
    void			drawLines();
    void			drawPixmaps();
    void			makeSymmetricalIfNeeded(bool);
    void			useClipRange();

    void			wheelMoved(CallBacker*);
};

