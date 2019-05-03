#ifndef pythonaccess_h
#define pythonaccess_h

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

class FilePath;


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
	    static bool		validInternalEnvironment(const FilePath&);
	    static bool		getInternalEnvironmentLocation(FilePath&);
	};


} //namespace OD

#endif
