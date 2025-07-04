/*+
________________________________________________________________________________

 Copyright:  (C) 2022 dGB Beheer B.V.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.
________________________________________________________________________________

-*/

#include "bufstring.h"
#include "bufstringset.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "ioman.h"
#include "oddirs.h"
#include "pythonaccess.h"
#include "settings.h"
#include "sharedlibs.h"

#include <string.h>

#include "odbind.h"

void cstring_del( char* str )
{
    if ( str )
	std::free( str );
}

const char* getUserDataDir()
{
    return strdup( GetSettingsDataDir() );
}

const char* getUserSurvey()
{
    return strdup( GetSettingsSurveyDir() );
}

hStringSet stringset_new()
{
    return new BufferStringSet;
}

hStringSet stringset_new1( const char* arr[], int len )
{
    return new BufferStringSet( arr, len );
}

hStringSet stringset_copy( hStringSet self )
{
    const auto* p = static_cast<BufferStringSet*>(self);
    return p ? new BufferStringSet(*p) : nullptr;
}

void stringset_del(hStringSet self)
{
    if ( self )
	delete static_cast<BufferStringSet*>(self);
}

int stringset_size( hStringSet self )
{
    const auto* p = static_cast<BufferStringSet*>(self);
    return p ? p->size() : 0;
}

hStringSet stringset_add( hStringSet self, const char* txt )
{
    auto* p = static_cast<BufferStringSet*>(self);
    if ( p )
	p->add( txt );
    return p;
}

const char* stringset_get( hStringSet self, int idx )
{
    const auto* p = static_cast<BufferStringSet*>(self);
    return p ? strdup( p->get( idx ).buf() ) : nullptr;
}


void ODBind::initDeveloperPythonPath()
{
#ifdef __odbind_dir__
    if ( !OD::isDeveloperBuild() ) //Installed project: do not add build dir
	return;

    const FilePath odbindfp( __odbind_dir__ );
    if ( !odbindfp.exists() || !File::isDirectory( odbindfp.fullPath() ) )
	return;

    OD::PythA().addBasePath( odbindfp );
#endif
}

