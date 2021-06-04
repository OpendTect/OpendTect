#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          November 2007
_______________________________________________________________________

-*/


#include "uiseismod.h"
#include "seistype.h"
#include "uiflatviewmainwin.h"
class SeisTrcBuf;
class SeisTrcBufDataPack;


/*! Displays SeisTrcBuf's

  Sequence is: Construct - setTrcBuf - start() - handleBufChange().

*/

mExpClass(uiSeis) uiSeisTrcBufViewer : public uiFlatViewMainWin
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

    // Convenience
    void		selectDispTypes(bool wva,bool vd);

protected:

    SeisTrcBufDataPack*	dp_;

    void	setBuf(const SeisTrcBuf&,Seis::GeomType,
		       const char*,const char* nm,int compnr,bool);
    void	releaseDP();

};


