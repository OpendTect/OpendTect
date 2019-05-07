#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		May 2019
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"

#include "enums.h"

namespace File { class Path; }


namespace OD
{
	enum PythonSource
	{
		Internal, System, Custom
	};
	mDeclareNameSpaceEnumUtils(Basic,PythonSource);

	mExpClass(Basic) PythonAccess
	{
	public:

	    static bool		hasInternalEnvironment();

	    static const char*	sPythonExecNm(bool v3=false,bool v2=false);
	    static const char*	sKeyPythonSrc();
	    static const char*	sKeyEnviron();

	private:
	    static bool		validInternalEnvironment(const File::Path&);
	    static bool		getInternalEnvironmentLocation(File::Path&);
	};


} //namespace OD
