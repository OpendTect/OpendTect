#ifndef wellio_h
#define wellio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellio.h,v 1.14 2012-08-03 13:00:45 cvskris Exp $
________________________________________________________________________


-*/

#include "wellmod.h"
#include "bufstring.h"
#include "strmdata.h"
class IOObj;
class MultiID;

namespace Well
{

mClass(Well) IO
{
public:

    const BufferString&	baseName() const	{ return basenm; }

    const char*		getFileName(const char* ext,int nr=0) const;
    bool		removeAll(const char* ext) const;

    static const char*	sKeyWell();
    static const char*	sKeyTrack();
    static const char*	sKeyLog();
    static const char*	sKeyMarkers();
    static const char*	sKeyD2T();
    static const char*	sKeyDispProps();
    static const char*	sExtWell();
    static const char*	sExtTrack();
    static const char*	sExtLog();
    static const char*	sExtMarkers();
    static const char*	sExtD2T();
    static const char*	sExtCSMdl();
    static const char*	sExtDispProps();
    static const char*	sExtWellTieSetup();

    static const char*	getMainFileName(const IOObj&);
    static const char*	getMainFileName(const MultiID&);

protected:


			IO(const char*,bool);

    StreamData		mkSD(const char* ext,int nr=0) const;
    const BufferString	basenm;

private:

    const bool		isrdr;

public:

    static const char*	mkFileName(const char* basefnm,const char* ext,int nr=0);

};


}; // namespace Well

#endif

