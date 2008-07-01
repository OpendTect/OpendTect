/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : Sep 2006
-*/

static const char* rcsID = "$Id: mmproc.cc,v 1.1 2008-07-01 14:18:23 cvsbert Exp $";

#include "mmassetmgr.h"
#include "mmprogspec.h"


static const ObjectSet<MMProc::AssetMgr>& ASMGRS()
{
    static ObjectSet<MMProc::AssetMgr>* mgrs = 0;
    if ( !mgrs ) mgrs = new ObjectSet<MMProc::AssetMgr>;
    return *mgrs;
}


int MMProc::AssetMgr::add( AssetMgr* am )
{
    ObjectSet<AssetMgr>& mgrs = const_cast<ObjectSet<AssetMgr>&>( ASMGRS() );
    mgrs += am;
    return mgrs.size() - 1;
}


static const ObjectSet<MMProc::ProgSpec>& PRSPS()
{
    static ObjectSet<MMProc::ProgSpec>* mgrs = 0;
    if ( !mgrs ) mgrs = new ObjectSet<MMProc::ProgSpec>;
    return *mgrs;
}


int MMProc::ProgSpec::add( ProgSpec* ps )
{
    ObjectSet<ProgSpec>& mgrs = const_cast<ObjectSet<ProgSpec>&>( PRSPS() );
    mgrs += ps;
    return mgrs.size() - 1;
}
