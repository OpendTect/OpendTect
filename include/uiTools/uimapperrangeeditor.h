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
class uiLineItem;
class uiPixmapItem;
class uiTextItem;
class uiAxisHandler;

template <class T> class Array2D;
namespace ColTab { class MapperSetup; }

mExpClass(uiTools) uiMapperRangeEditor : public uiGroup
{
public:

    typedef ColTab::MapperSetup	MapperSetup;

				uiMapperRangeEditor(uiParent*,int an_id=0,
						    bool fixdrawrg=true);
				~uiMapperRangeEditor();
    int				ID() const		{ return id_; }

    void			setEmpty();
    bool			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const Array2D<float>*);
    void			setData(const IOPar&);
    void			setMarkValue(float,bool forx);

    const ColTab::MapperSetup&	mapperSetup()	{ return *mappersetup_; }
    void			setMapperSetup(MapperSetup&);
    void			setColTabSeq(const ColTab::Sequence&);

    uiHistogramDisplay&		getDisplay()	{ return *histogramdisp_; }

protected:

    const int			id_;
    uiHistogramDisplay*		histogramdisp_;
    uiAxisHandler*		xax_;

    RefMan<MapperSetup>		mappersetup_;
    ConstRefMan<ColTab::Sequence> ctseq_;

    uiPixmapItem*		leftcoltab_;
    uiPixmapItem*		centercoltab_;
    uiPixmapItem*		rightcoltab_;

    uiLineItem*			minline_;
    uiLineItem*			maxline_;
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
};
