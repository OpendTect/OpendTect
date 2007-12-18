#ifndef uiseistrcbufviewer_h
#define uiseistrcbufviewer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          November 2007
 RCS:           $Id: uiseistrcbufviewer.h,v 1.8 2007-12-18 10:05:03 cvssatyaki Exp $
_______________________________________________________________________

-*/


#include "flatview.h"
#include "seistype.h"
#include "seisbufadapters.h"
#include "seisbuf.h"

#include "uiflatviewmainwin.h"


class BufferString;


class uiSeisTrcBufViewer : public uiFlatViewMainWin
{
public:

    class Setup : public uiFlatViewMainWin::Setup
    {
    public:
			Setup( const char* wintitl)
			    : uiFlatViewMainWin::Setup(wintitl) 	{}

        mDefSetupMemb(BufferString, wintitl)
    };		    
    				
			uiSeisTrcBufViewer(uiParent*,
				           const uiSeisTrcBufViewer::Setup&) ;
    			~uiSeisTrcBufViewer();

    SeisTrcBufDataPack*	setTrcBuf(SeisTrcBuf*,Seis::GeomType,SeisTrcInfo::Fld, 
	                          const char*,const char* nm);
    void		setBuffer(SeisTrcBuf*,Seis::GeomType,
	    			  SeisTrcInfo::Fld,bool wva);
    void		update();
    void		removePacks();

    uiFlatViewer*	getViewer()		{ return vwr_; }

protected:

    SeisTrcBuf*		tbuf_;
    uiFlatViewer*	vwr_;

};


#endif
