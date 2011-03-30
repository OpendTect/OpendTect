#ifndef uiseistrcbufviewer_h
#define uiseistrcbufviewer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          November 2007
 RCS:           $Id: uiseistrcbufviewer.h,v 1.13 2011-03-30 04:43:53 cvssatyaki Exp $
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
	                          const char*,const char* nm,int compnr=0);
    SeisTrcBufDataPack*	setTrcBuf(const SeisTrcBuf&,Seis::GeomType,
	    			  const char*,const char* nm,int compnr=0);
    void		handleBufChange();
    void		clearData();

    uiFlatViewer*	getViewer()		{ return &viewer(); }

protected:

};


#endif
