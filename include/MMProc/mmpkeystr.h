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

#include "fixedstring.h"

namespace MMPStr
{
    inline FixedString sMMPClient()		{ return "mmpclient"; }
    inline FixedString sMMPServer()		{ return "mmpserver"; }
    inline FixedString sDataRoot()		{ return "dataroot"; }
    inline FixedString sDefaultDataRoot()	{ return "defdataroot"; }
    inline FixedString sError()			{ return "error"; }
    inline FixedString sSetDataRoot()		{ return "dataroot_set"; }
    inline FixedString sGetDataRoot()		{ return "dataroot_get"; }
    inline FixedString sCheckDataRoot()		{ return "dataroot_chk"; }
    inline FixedString sLogFile()		{ return "logfile"; }
    inline FixedString sGetLogFile()		{ return "logfile_get"; }
    inline FixedString sODVersion()		{ return "odversion"; }
    inline FixedString sODPlatform()		{ return "odplatform"; }
    inline FixedString sOK()			{ return "ok"; }
    inline FixedString sStartJob()		{ return "startjob"; }
    inline FixedString sStatus()		{ return "status"; }

    inline FixedString sProcName()		{ return "proc_name"; }
    inline FixedString sParFile()		{ return "par_file"; }
    inline FixedString sHostName()		{ return "host_name"; }
    inline FixedString sPortName()		{ return "port_name"; }
    inline FixedString sJobID()			{ return "job_id"; }

};
