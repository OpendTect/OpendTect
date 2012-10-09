/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Sep 2006
-*/

static const char* rcsID = "$Id$";

#include "mmassetmgr.h"
#include "mmprogspec.h"


const ObjectSet<MMProc::AssetMgr>& MMProc::ASMGRS()
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


ObjectSet<MMProc::ProgSpec>& MMProc::PRSPS()
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


