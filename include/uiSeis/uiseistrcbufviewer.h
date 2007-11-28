#ifndef uiseistrcbufviewer_h
#define uiseistrcbufviewer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          November 2007
 RCS:           $Id: uiseistrcbufviewer.h,v 1.3 2007-11-28 03:21:38 cvssatyaki Exp $
_______________________________________________________________________

-*/


#include "flatview.h"
#include "seistype.h"
#include "seisbufadapters.h"
#include "seisbuf.h"

#include "uiflatviewmainwin.h"


class uiSeisTrcBufViewer : public uiFlatViewMainWin
{
public:

    class Setup : public uiFlatViewMainWin::Setup
    {
    public:
			Setup( const char* wintitl, const char* nm)
			    : uiFlatViewMainWin::Setup(wintitl)
			    , nm_(nm)			{}

        mDefSetupMemb(BufferString, wintitl)
        mDefSetupMemb(const char*, nm)
    };		    
    				
			uiSeisTrcBufViewer(uiParent*,const Setup&, 
				           Seis::GeomType,SeisTrcBuf*);
		    	uiSeisTrcBufViewer(uiParent*,const Setup&, 
				           FlatDataPack*);	
    			~uiSeisTrcBufViewer();

    void		setData(FlatDataPack*);

protected:

    SeisTrcBuf*		tbuf_;
    SeisTrcBufDataPack*	dp_;
    FlatDataPack*	datapack_;
    Seis::GeomType	geom_;	    
    FlatView::Appearance* app_;
    uiFlatViewer*	vwr_;
    uiFlatViewMainWin::Setup setup_;
    uiSeisTrcBufViewer::Setup stbufsetup_;

    void		initialise();
};


#endif
