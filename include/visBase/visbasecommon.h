#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2017
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "gendefs.h"

class TaskRunner;
class uiString;
namespace ColTab { class Mapper; class Sequence; }

namespace visBase
{

class DataObject;
class UserShowWaitImpl;


mExpClass(visBase) UserShowWait
{
public:

			UserShowWait(const DataObject*,const uiString&,
					int statusbarfld=0);
			~UserShowWait()		{ readyNow(); }

    void		setMessage(const uiString&);
    void		readyNow();

protected:

    UserShowWaitImpl*    impl_;

};

} // namespace visBase
