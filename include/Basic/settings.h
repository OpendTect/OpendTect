#ifndef Settings_H
#define Settings_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		4-11-1995
 RCS:		$Id: settings.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

Settings hold the user settings in a free format list.

@$*/


#include <uidobjset.h>


class Settings
{
public:

			Settings(const char* strmopen);
			~Settings();
    int			write() const;

    const char*		get(const char* key) const;
    void		set(const char* key,const char* val);
    int			is(const char*,const char*) const;
    int			isMajor(const char*) const;
    PtrUserIDObjectSet	keys() const
			{ return (PtrUserIDObjectSet)list_; }

    static Settings&	common();

protected:

    PtrUserIDObjectSet	list_;
    FileNameString	fname;

    static Settings*	common_;

};


#endif
