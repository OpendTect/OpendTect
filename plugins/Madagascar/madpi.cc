/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : July 2007
-*/

static const char* mUnusedVar rcsID = "$Id: madpi.cc,v 1.8 2012-05-02 11:52:47 cvskris Exp $";

#include "maddefs.h"
#include "odplugin.h"
#include "madprocflowtr.h"

mDefODPluginEarlyLoad(Madagascar)
mDefODPluginInfo(Madagascar)
{
    static PluginInfo retpii = {
	"Madagascar (base)",
	"dGB - Bert Bril",
	"=od",
	"The Madagascar batch-level tools." };
    return &retpii;
}


mDefODInitPlugin(Madagascar)
{
    ODMadProcFlowTranslatorGroup::initClass();
    dgbODMadProcFlowTranslator::initClass();
    
    static BufferString prescanmsg = ODMad::PI().errMsg();
    return prescanmsg.isEmpty() ? 0 : prescanmsg.buf();
}
