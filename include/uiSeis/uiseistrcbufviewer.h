#ifndef uiseistrcbufviewer_h
#define uiseistrcbufviewer_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          November 2007
 RCS:           $Id: uiseistrcbufviewer.h,v 1.1 2007-11-21 09:06:46 cvssatyaki Exp $
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
			Setup( const char* wintitl)
			    : uiFlatViewMainWin::Setup(wintitl)		{}

        mDefSetupMemb(BufferString, wintitl)
    };		    
    				
			uiSeisTrcBufViewer( uiParent*, Setup&, Seis::GeomType
					    , SeisTrcBuf* );
		    	uiSeisTrcBufViewer( uiParent*, Setup&, Seis::GeomType  					         , FlatDataPack* );	
    			
    			~uiSeisTrcBufViewer();
    void		initialise();
    void		setData( FlatDataPack* );

protected:

    SeisTrcBuf*		tbuf_;
    SeisTrcBufDataPack*	stdp_;
    Seis::GeomType	geom_;	    
    uiFlatViewMainWin*	viewwin_;
    FlatView::Appearance* app_;
    uiFlatViewer*	vwr_;
    const uiFlatViewMainWin::Setup& setup_;
};
#endif
