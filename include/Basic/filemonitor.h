#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2016
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "notify.h"
class BufferStringSet;
class od_i_FSWatcher;


namespace File
{

/*!\brief Monitors files and directories for changes */

mExpClass(Basic) Monitor : public CallBacker
{
public:

			Monitor();
			~Monitor();

    void		watch(const char*);
    void		watch(const BufferStringSet&);
    void		forget(const char*);

    CNotifier<Monitor,BufferString>	dirChanged;
    CNotifier<Monitor,BufferString>	fileChanged;

protected:

    od_i_FSWatcher&	watcher_;
    friend class	od_i_FSWatcher;

};

} // namespace File
