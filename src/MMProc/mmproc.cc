/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmassetmgr.h"
#include "mmprogspec.h"


const ObjectSet<MMProc::AssetMgr>& MMProc::ASMGRS()
{
    mDefineStaticLocalObject( PtrMan<ObjectSet<MMProc::AssetMgr> >, mgrs, 
			      = new ObjectSet<MMProc::AssetMgr> );
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
    mDefineStaticLocalObject( PtrMan<ObjectSet<MMProc::ProgSpec> >, mgrs, 
			      = new ObjectSet<MMProc::ProgSpec> );
    return *mgrs;
}


int MMProc::ProgSpec::add( ProgSpec* ps )
{
    ObjectSet<ProgSpec>& mgrs = const_cast<ObjectSet<ProgSpec>&>( PRSPS() );
    mgrs += ps;
    return mgrs.size() - 1;
}
