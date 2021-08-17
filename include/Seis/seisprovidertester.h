#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		April 2017
________________________________________________________________________

*/

#include "seiscommon.h"

#include "dbkey.h"
#include "ptrman.h"
#include "geomid.h"
#include "survsubsel.h"

class SeisTrc;
class SeisTrcBuf;
class CubeSubSel;
class LineSubSelSet;


namespace Seis
{

class PreLoader;
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

    mUseType( Pos,	GeomID );
    mUseType( Survey,	GeomSubSel );

			ProviderTester(bool bequiet,const char* survnm=0);
			ProviderTester(GeomType,bool bequiet,
					const char* survnm=0);
			~ProviderTester();
    const uiRetVal&	result() const	    { return uirv_; }

    void		setSurveyName(const char*);
    void		setInput(const DBKey&);

    bool		testGetAt(const TrcKey&,bool exists);
    bool		testGetAll();
    bool		testIOParUsage();

    PreLoader*		preLoad(const CubeSubSel&) const;
    PreLoader*		preLoad(const LineSubSelSet&) const;
    void		removePreLoad(PreLoader*) const;


    static const char*	survName()		{ return "F3_Test_Survey"; }
    static DBKey	volDBKey()		{ return DBKey("100010.2"); }
    static DBKey	volPSDBKey()		{ return DBKey("100010.6"); }
    static DBKey	lineDBKey()		{ return DBKey("100010.12"); }
    static DBKey	linePSDBKey()		{ return DBKey("100010.13"); }
    static int		nrLineGeometries()	{ return 3; }
    static GeomID	lineGeomID(int);
    static BufferString	lineGeomNameBase()	{ return "IOTest Line Geom "; }

    static BinID	existingbid_;
    static BinID	nonexistingbid_;
    static GeomID	nonexistinggeomid_;
    static int		nonexistingtrcnr_;
    GeomID		existinggeomid_;
    int			existingTrcNr(GeomID gid=GeomID()) const;

    Provider*		prov_			= 0;
    SeisTrc&		trc_;
    SeisTrcBuf&		gath_;
    TrcKey&		lastgoodtk_;
    uiRetVal		uirv_;

protected:

    bool		testGetViaSD(const TrcKey&,bool);
    void		prTrc(const SeisTrc* trc=0,const char* start=0) const;
    void		prGath(const SeisTrcBuf* tbuf=0) const;
    void		prResult(const char*,bool,bool istrc=true) const;

};

} // namespace Seis
