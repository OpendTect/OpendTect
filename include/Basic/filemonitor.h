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
mFDQtclass(i_QFileSystemWatcher);


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
    void		updateWatcher(const char* fnm);

    CNotifier<Monitor,BufferString>	dirChanged;
    CNotifier<Monitor,BufferString>	fileChanged;

protected:

    mQtclass(i_QFileSystemWatcher)&	watcher_;
    friend class			mQtclass(i_QFileSystemWatcher);

};

} // namespace File
