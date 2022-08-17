#pragma once
/*+
 * ________________________________________________________________________
 *
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * Author:	  Wayne Mogg
 * Date:	  Apr 2022
 * ________________________________________________________________________
 *
 * -*/

#include "stringview.h"

namespace MMPStr
{
    inline StringView sMMPClient()		{ return "mmpclient"; }
    inline StringView sMMPServer()		{ return "mmpserver"; }
    inline StringView sDataRoot()		{ return "dataroot"; }
    inline StringView sDefaultDataRoot()	{ return "defdataroot"; }
    inline StringView sError()			{ return "error"; }
    inline StringView sSetDataRoot()		{ return "dataroot_set"; }
    inline StringView sGetDataRoot()		{ return "dataroot_get"; }
    inline StringView sCheckDataRoot()		{ return "dataroot_chk"; }
    inline StringView sLogFile()		{ return "logfile"; }
    inline StringView sGetLogFile()		{ return "logfile_get"; }
    inline StringView sODVersion()		{ return "odversion"; }
    inline StringView sODPlatform()		{ return "odplatform"; }
    inline StringView sOK()			{ return "ok"; }
    inline StringView sStartJob()		{ return "startjob"; }
    inline StringView sStatus()		{ return "status"; }

    inline StringView sProcName()		{ return "proc_name"; }
    inline StringView sParFile()		{ return "par_file"; }
    inline StringView sHostName()		{ return "host_name"; }
    inline StringView sPortName()		{ return "port_name"; }
    inline StringView sJobID()			{ return "job_id"; }

} // namespace MMPStr
