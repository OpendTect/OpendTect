/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "moddepmgr.h"

#include "crssystem.h"
#include "filepath.h"
#include "genc.h"
#include "legal.h"
#include "oddirs.h"
#include "survinfo.h"

static uiString* legalText()
{
    return new uiString(toUiString(

	"All source, data files and other contents of the PROJ.4 package are\n"
	"available under the following terms. Note that the PROJ 4.3 and earlier\n"
	"was \"public domain\" as is common with US government work, but apparently\n"
	"this is not a well defined legal term in many countries.  I am placing \n"
	"everything under the following MIT style license because I believe it is\n"
	"effectively the same as public domain, allowing anyone to use the code as\n"
	"they wish, including making proprietary derivatives. \n"
	"\n"
	"Though I have put my own name as copyright holder, I don't mean to imply\n"
	"I did the work.  Essentially all work was done by Gerald Evenden. \n"
	"\n"
	"--------------\n"
	"\n"
	"Copyright (c) 2000, Frank Warmerdam\n"
	"\n"
	"Permission is hereby granted, free of charge, to any person obtaining a\n"
	"copy of this software and associated documentation files (the \"Software\"),\n"
	"to deal in the Software without restriction, including without limitation\n"
	"the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
	"and/or sell copies of the Software, and to permit persons to whom the\n"
	"Software is furnished to do so, subject to the following conditions:\n"
	"\n"
	"The above copyright notice and this permission notice shall be included\n"
	"in all copies or substantial portions of the Software.\n"
	"\n"
	"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS\n"
	"OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
	"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL\n"
	"THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
	"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\n"
	"FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER\n"
	"DEALINGS IN THE SOFTWARE.\n"

	));
}


mDefModInitFn(CRS)
{
    mIfNotFirstTime(return);

    legalInformation().addCreator(legalText, "PROJ.4");
    if ( !NeedDataBase() )
	return;

    Coords::initCRSDatabase();
    Coords::ProjectionBasedSystem::initClass();
    SI().readSavedCoordSystem();
}
