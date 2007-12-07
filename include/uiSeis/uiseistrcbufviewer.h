#ifndef uiseistrcbufviewer_h
#define uiseistrcbufviewer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          November 2007
 RCS:           $Id: uiseistrcbufviewer.h,v 1.6 2007-12-07 08:23:01 cvssatyaki Exp $
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
    void		update(SeisTrcBuf*, int compnr);
    void		update();

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
