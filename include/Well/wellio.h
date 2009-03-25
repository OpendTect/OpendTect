#ifndef wellio_h
#define wellio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellio.h,v 1.11 2009-03-25 16:39:47 cvsbert Exp $
________________________________________________________________________


-*/

#include "bufstring.h"
#include "strmdata.h"
class IOObj;
class MultiID;

namespace Well
{

mClass IO
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
