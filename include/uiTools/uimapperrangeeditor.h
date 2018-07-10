#ifndef uimapperrangeeditor_h
#define uimapperrangeeditor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

#include "datapack.h"

class uiAxisHandler;
class uiHistogramDisplay;
class uiLineItem;
class uiPixmapItem;
class uiTextItem;

template <class T> class Array2D;
namespace ColTab { class MapperSetup; class Sequence; }

mExpClass(uiTools) uiMapperRangeEditor : public uiGroup
{
public:
				uiMapperRangeEditor(uiParent*,int id,
					bool fixdrawrg=true);
				~uiMapperRangeEditor();

    int				ID()			{ return id_; }
    void			setID( int id )		{ id_ = id; }

    void			setEmpty();
    bool			setDataPackID(DataPack::ID,DataPackMgr::ID,
					      int version=0);
    void			setData(const Array2D<float>*);
    void			setMarkValue(float,bool forx);

    void			setColTabMapperSetup(
					const ColTab::MapperSetup&);
    const ColTab::MapperSetup&	getColTabMapperSetup()	{ return *ctmapper_; }
    void			setColTabSeq(const ColTab::Sequence&);
    const ColTab::Sequence&	getColTabSeq() const	{ return *ctseq_; }

    uiHistogramDisplay&		getDisplay()	{ return *histogramdisp_; }

    Notifier<uiMapperRangeEditor>	rangeChanged;
    Notifier<uiMapperRangeEditor>	sequenceChanged;

protected:

    uiHistogramDisplay*		histogramdisp_;
    int 			id_;
    uiAxisHandler*		xax_;

    ColTab::MapperSetup*	ctmapper_;
    ColTab::Sequence*		ctseq_;

    uiPixmapItem*		leftcoltab_;
    uiPixmapItem*		centercoltab_;
    uiPixmapItem*		rightcoltab_;

    uiLineItem*			minline_;
    uiLineItem*			maxline_;
    uiLineItem*			ctminline_;
    uiLineItem*			ctmaxline_;
    uiTextItem*			minvaltext_;
    uiTextItem*			maxvaltext_;

    Interval<float>		datarg_;
    Interval<float>		cliprg_;
    int				startpix_;
    int				stoppix_;

    bool			mousedown_;

    void 			init();
    void			drawAgain();
    void			drawText();
    void			drawLines();
    void			drawPixmaps();
    bool			changeLinePos(bool pressedonly=false);

    void			mousePressed(CallBacker*);
    void			mouseMoved(CallBacker*);
    void			mouseReleased(CallBacker*);
    void			wheelMoved(CallBacker*);

    void			histogramResized(CallBacker*);
    void			histDRChanged(CallBacker*);
};

#endif
