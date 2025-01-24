#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "seisbufadapters.h"
#include "seistype.h"
#include "uiflatviewmainwin.h"
class SeisTrcBuf;


/*! Displays SeisTrcBuf's

  Sequence is: Construct - setTrcBuf - start() - handleBufChange().

*/

mExpClass(uiSeis) uiSeisTrcBufViewer : public uiFlatViewMainWin
{
public:

			uiSeisTrcBufViewer(uiParent*,
					   const uiFlatViewMainWin::Setup&);
			~uiSeisTrcBufViewer();

    void		setTrcBuf(const SeisTrcBuf*,Seis::GeomType,
			      const char* dp_cat,const char* nm,
			      const ZDomain::Info&,int compnr=0);
			//!< This uses the buf in-place
    void		setTrcBuf(const SeisTrcBuf&,Seis::GeomType,
			      const char* dp_cat,const char* nm,
			      const ZDomain::Info&,int compnr=0);
			//!< This makes a copy of the buf

    // Convenience
    void		selectDispTypes(bool wva,bool vd);

    void		handleBufChange();

    ConstRefMan<SeisTrcBufDataPack> dataPack() const	{ return dp_; }

private:

    void	setBuf(const SeisTrcBuf&,Seis::GeomType,
		       const char*,const char* nm,const ZDomain::Info&,
		       int compnr,bool ismine);

    RefMan<SeisTrcBufDataPack>	dp_;

};
