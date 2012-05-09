#ifndef uiseistrcbufviewer_h
#define uiseistrcbufviewer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          November 2007
 RCS:           $Id: uiseistrcbufviewer.h,v 1.16 2012-05-09 12:08:37 cvsbert Exp $
_______________________________________________________________________

-*/


#include "seistype.h"
#include "uiflatviewmainwin.h"
class BufferString;
class SeisTrcBuf;
class SeisTrcBufDataPack;


/*! Displays SeisTrcBuf's

  Sequence is: Construct - setTrcBuf - start() - handleBufChange().

*/

mClass uiSeisTrcBufViewer : public uiFlatViewMainWin
{
public:

			uiSeisTrcBufViewer(uiParent*,
				           const uiFlatViewMainWin::Setup&) ;
    			~uiSeisTrcBufViewer();

    void		setTrcBuf(const SeisTrcBuf*,Seis::GeomType,
			      const char* dp_cat,const char* nm,int compnr=0);
    			//!< This uses the buf in-place
    void		setTrcBuf(const SeisTrcBuf&,Seis::GeomType,
			      const char* dp_cat,const char* nm,int compnr=0);
    			//!< This makes a copy of the buf

    SeisTrcBufDataPack*	dataPack()		{ return dp_; }
    void		handleBufChange();
    void		clearData();

    // Convenience
    void		selectDispTypes(bool wva,bool vd);

    uiFlatViewer*	getViewer()		{ return &viewer(); }

protected:

    SeisTrcBufDataPack*	dp_;

    void	setBuf(const SeisTrcBuf&,Seis::GeomType,
		       const char*,const char* nm,int compnr,bool);
    void	releaseDP();


};


#endif
