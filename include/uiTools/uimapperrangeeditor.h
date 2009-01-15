#ifndef uimapperrangeeditor_h
#define uimapperrangeeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditor.h,v 1.3 2009-01-15 10:57:30 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

#include "datapack.h"

class uiHistogramDisplay;
class uiLineItem;
class uiPixmapItem;
class uiTextItem;
namespace ColTab { struct MapperSetup; class Sequence; }

mClass uiMapperRangeEditor : public uiGroup
{
public:

    				uiMapperRangeEditor(uiParent*,int id);
				~uiMapperRangeEditor();

    int				ID()		       { return id_; }

    bool                        setDataPackID(DataPack::ID,DataPackMgr::ID);
    void                        setMarkValue(float,bool forx);

    void			setColTabMapperSetupWthSeq(
	    				const ColTab::MapperSetup&,
					const ColTab::Sequence&);
    const ColTab::MapperSetup&	getColTabMapperSetup()	{ return *ctbmapper_; }
    
    Notifier<uiMapperRangeEditor>	rangeChanged;

protected:
	
    uiHistogramDisplay*		histogramdisp_;    
    int 			id_;

    ColTab::MapperSetup*        ctbmapper_;
    ColTab::Sequence*		ctbseq_;

    uiLineItem*			minline_;
    uiLineItem*			maxline_;

    uiPixmapItem*		leftcoltab_;
    uiPixmapItem*		centercoltab_;
    uiPixmapItem*		rightcoltab_;

    uiTextItem*			minlineval_;
    uiTextItem*			maxlineval_;

    float			lefttminval_;
    float			rightmaxval_;
    float			minlinebasepos_;
    float			maxlinebasepos_;
    float			minlinecurpos_;
    float			maxlinecurpos_;

    bool			mousedown_;

    void			drawAgain();
    void			drawText();
    void			fixTextPos();
    void			drawLines();
    void			fixLinesPos();
    void			drawPixmaps();
    void			fixPixmapsPos();
    bool			changeLinePos(bool pressedonly=false);

    void			mousePressed(CallBacker*);
    void			mouseMoved(CallBacker*);
    void			mouseReleased(CallBacker*);

    void			histogramResized(CallBacker*);
};

#endif
