#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

#include "datapack.h"
#include "coltabsequence.h"
#include "coltabmappersetup.h"

class uiHistogramDisplay;
class uiManipHandleItem;
class uiPixmapItem;
class uiTextItem;
class uiAxisHandler;

template <class T> class Array2D;
namespace ColTab { class Mapper; }

mExpClass(uiTools) uiMapperRangeEditor : public uiGroup
{
public:

    typedef ColTab::Mapper	Mapper;

				uiMapperRangeEditor(uiParent*,int an_id=0,
						    bool fixdrawrg=true);
				~uiMapperRangeEditor();

    int				ID() const		{ return id_; }
    void			setID( int id )		{ id_ = id; }

    void			setEmpty();
    bool			setDataPackID(DataPack::ID,DataPackMgr::ID,
					      int version);
    void			setData(const Array2D<float>*);
    bool			setData(const IOPar&);
    void			setMarkValue(float,bool forx);

    const ColTab::Mapper&	mapper()	{ return *mapper_; }
    void			setMapper(Mapper&);
    void			setColTabSeq(const ColTab::Sequence&);

    uiHistogramDisplay&		getDisplay()	{ return *histogramdisp_; }

protected:

    int				id_;
    uiHistogramDisplay*		histogramdisp_;
    uiAxisHandler*		xax_;

    RefMan<Mapper>		mapper_;
    ConstRefMan<ColTab::Sequence> ctseq_;

    uiPixmapItem*		leftcoltab_;
    uiPixmapItem*		centercoltab_;
    uiPixmapItem*		rightcoltab_;

    uiManipHandleItem*		minhandle_;
    uiManipHandleItem*		maxhandle_;
    uiTextItem*			minvaltext_;
    uiTextItem*			maxvaltext_;

    Interval<float>		datarg_;
    Interval<float>		cliprg_;
    int				startpix_;
    int				stoppix_;

    bool			mousedown_;

    void			init();
    void			drawAgain();
    void			drawText();
    void			drawLines();
    void			drawPixmaps();
    bool			changeLinePos(bool pressedonly=false);

    void			mousePressed(CallBacker*);
    void			mouseMoved(CallBacker*);
    void			mouseReleased(CallBacker*);

    void			histogramResized(CallBacker*);
    void			histDRChanged(CallBacker*);
    void			colSeqChg(CallBacker*);
    void			mapperChg(CallBacker*);

};
