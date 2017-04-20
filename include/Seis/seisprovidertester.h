#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		April 2017
________________________________________________________________________

*/

#include "seismod.h"

#include "dbkey.h"
#include "ptrman.h"
#include "seistrc.h"

class SeisTrcBuf;
class TrcKeyZSampling;

namespace Seis
{

class Provider;
class SelData;

mExpClass(Seis) ProviderTester
{
public:
				ProviderTester();
				~ProviderTester()	{}

    uiRetVal			setInput(const char*);

    void			testGet(const TrcKey&,const char*txt="");

    void			testGetNext();
    void			testSubselection(SelData*,const char* txt);
    void			testPreLoadTrc(bool currenttrc=true);
				//!< Will reset if currenttrc is false.
    void			testComponentSelection(bool currenttrc=true);
				//!< Will reset if currenttrc is false.

protected:

    void			prTrc(const char* start,const uiRetVal&,
				      bool withcomps=false,
				      bool withoffs=false);
    void			prBuf(const char* start,const SeisTrcBuf&,
				      const uiRetVal&);

    DBKey			dbky_;
    SeisTrc			trc_;

    PtrMan<Provider>		prov_;
};

}
