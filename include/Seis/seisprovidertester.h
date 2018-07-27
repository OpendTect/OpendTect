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

/*!
\brief Helper class for testing Seis::Provider. Input can be changed to
another seismic data type at any point in time by calling setInput().

Required functionality can be tested by calling appropriate functions,
which test multiple cases for each type of functionality they are supposed
to test. Will print out standard SeisTrc information and indicate if the
test is a success. Each function will not proceed to further test case if
any of the test case fails, while calls to other functions will still work.
*/

mExpClass(Seis) ProviderTester
{
public:
				ProviderTester();
				~ProviderTester()	{}

    uiRetVal			setSurveyName(const char*);
    uiRetVal			setInput(const char* dbky);

    bool			testGet(const TrcKey&,const char*txt="");

    bool			testGetNext();
    bool			testSubselection(SelData*,const char* txt,
						 bool outsidedatarg=false);
				/*!< Will subselect and checks no. of traces
				read by iterating through the subselection,
				after which the subselection will be
				removed. */
    bool			testPreLoadTrc(bool currenttrc=true);
				//!< Will reset if currenttrc is false.
    bool			testPreLoad(const TrcKeyZSampling&);
				/*!< Preloads the entire specified
				volume/line, subselects and iterates through
				it before unloading it. */
    bool			testComponentSelection(bool currenttrc=true);
				//!< Will reset if currenttrc is false.
    bool			testIOParUsage(bool currenttrc=true);
				//!< Will reset if currenttrc is false.

protected:

    bool			prTrc(const char* start,const uiRetVal&,
				      bool withcomps=false,
				      bool withoffs=false,
				      bool addnewline=true);
    bool			prBuf(const char* start,const SeisTrcBuf&,
				      const uiRetVal&,
				      bool addnewline=true);

    DBKey			dbky_;
    SeisTrc			trc_; //!< Current trace.
    PtrMan<Provider>		prov_;
};

}
