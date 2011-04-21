#ifndef uiseistrcbufviewer_h
#define uiseistrcbufviewer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          November 2007
 RCS:           $Id: uiseistrcbufviewer.h,v 1.14 2011-04-21 13:09:13 cvsbert Exp $
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
			Setup( const char* wt, const int nv )
			    : uiFlatViewMainWin::Setup(wt) 	
			    , nrvwrs_(nv)				{}
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
