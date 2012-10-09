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

#include "uigroup.h"

#include "datapack.h"

class uiHistogramDisplay;
class uiLineItem;
class uiPixmapItem;
class uiTextItem;
class uiAxisHandler;

template <class T> class Array2D;
namespace ColTab { class MapperSetup; class Sequence; }

mClass uiMapperRangeEditor : public uiGroup
{
public:

    				uiMapperRangeEditor(uiParent*,int id);
				~uiMapperRangeEditor();

    int				ID()		       { return id_; }

    bool                        setDataPackID(DataPack::ID,DataPackMgr::ID);
    void                        setData(const Array2D<float>*);
    void                        setMarkValue(float,bool forx);

    void			setColTabMapperSetup(
	    				const ColTab::MapperSetup&);
    void			setColTabSeq(const ColTab::Sequence&);
    const ColTab::MapperSetup&	getColTabMapperSetup()	{ return *ctmapper_; }

    uiHistogramDisplay&		getDisplay()	{ return *histogramdisp_; }    

    Notifier<uiMapperRangeEditor>	rangeChanged;

protected:
	
    uiHistogramDisplay*		histogramdisp_;    
    int 			id_;
    uiAxisHandler*		xax_;

    ColTab::MapperSetup*        ctmapper_;
    ColTab::Sequence*		ctseq_;

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

    void 			init();
    void			drawAgain();
    void			drawText();
    void			drawLines();
    void			drawPixmaps();
    bool			changeLinePos(bool pressedonly=false);

    void			mousePressed(CallBacker*);
    void			mouseMoved(CallBacker*);
    void			mouseReleased(CallBacker*);

    void			histogramResized(CallBacker*);
};

#endif
