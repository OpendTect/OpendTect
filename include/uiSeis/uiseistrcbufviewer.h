#ifndef uiseistrcbufviewer_h
#define uiseistrcbufviewer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          November 2007
 RCS:           $Id: uiseistrcbufviewer.h,v 1.11 2009-01-08 08:31:03 cvsranojay Exp $
_______________________________________________________________________

-*/


#include "flatview.h"
#include "seistype.h"
#include "seisbufadapters.h"
#include "seisbuf.h"

#include "uiflatviewmainwin.h"


class BufferString;


mClass uiSeisTrcBufViewer : public uiFlatViewMainWin
{
public:

    mClass Setup : public uiFlatViewMainWin::Setup
    {
    public:
			Setup( const char* wintitl, const int nrvwrs)
			    : uiFlatViewMainWin::Setup(wintitl) 	
			    , nrvwrs_(nrvwrs)				{}
        mDefSetupMemb(BufferString, wintitl)
        mDefSetupMemb(int, nrvwrs)
    };		    
    				
			uiSeisTrcBufViewer(uiParent*,
				           const uiSeisTrcBufViewer::Setup&) ;
    			~uiSeisTrcBufViewer();

    SeisTrcBufDataPack*	setTrcBuf(SeisTrcBuf*,Seis::GeomType,
	                          const char*,const char* nm);
    SeisTrcBufDataPack*	setTrcBuf(const SeisTrcBuf&,Seis::GeomType,
	    			  const char*,const char* nm);
    void		handleBufChange();
    void		clearData();

    uiFlatViewer*	getViewer()		{ return &viewer(); }

protected:

};


#endif
