#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uihistogramsel.h"

#include "coltabsequence.h"
#include "coltabmappersetup.h"

class uiPixmapItem;

namespace ColTab { class Mapper; }

mExpClass(uiTools) uiMapperRangeEditor : public uiHistogramSel
{
public:

    typedef ColTab::Mapper	Mapper;

				uiMapperRangeEditor(uiParent*,int an_id=0,
						    bool fixdrawrg=true);
				~uiMapperRangeEditor();

    const ColTab::Mapper&	mapper()	{ return *mapper_; }
    void			setMapper(Mapper&);
    void			setColTabSeq(const ColTab::Sequence&);

protected:

    RefMan<Mapper>		mapper_;
    ConstRefMan<ColTab::Sequence> ctseq_;

    uiPixmapItem*		leftcoltab_;
    uiPixmapItem*		centercoltab_;
    uiPixmapItem*		rightcoltab_;

    void			drawAgain();
    void			useClipRange();

    void			initPixmaps();
    void			drawPixmaps();

    void			colSeqChg(CallBacker*);
    void			mapperChg(CallBacker*);

};
