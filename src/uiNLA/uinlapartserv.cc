/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uinlapartserv.cc,v 1.1 2003-10-01 12:51:43 bert Exp $
________________________________________________________________________

-*/

#include "uinlapartserv.h"
#include "featset.h"
#include "uidset.h"

const int uiNLAPartServer::evPrepareWrite	= 0;
const int uiNLAPartServer::evPrepareRead	= 1;
const int uiNLAPartServer::evReadFinished	= 2;
const int uiNLAPartServer::evGetInputNames	= 3;
const int uiNLAPartServer::evGetStoredInput	= 4;
const int uiNLAPartServer::evGetData		= 5;
const int uiNLAPartServer::evSaveMisclass	= 6;
const int uiNLAPartServer::evCreateAttrSet	= 7;


uiNLAPartServer::uiNLAPartServer( uiApplService& a )
	: uiApplPartServer(a)
	, fstrain(*new FeatureSet)
	, fstest(*new FeatureSet)
	, fsmc(*new FeatureSet)
{
}


uiNLAPartServer::~uiNLAPartServer()
{
    deepErase( inpnms );
    delete &fstrain;
    delete &fstest;
    delete &fsmc;
}
