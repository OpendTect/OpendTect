#ifndef wellio_h
#define wellio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellio.h,v 1.5 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________


-*/

#include "bufstring.h"
#include "strmdata.h"

namespace Well
{

class IO
{
public:

    static const char*	mkFileName(const char*,const char* ext,int nr=0);
    const char*		getFileName(const char* ext,int nr=0) const;
    bool		removeAll(const char* ext) const;

    static const char*	sKeyWell;
    static const char*	sKeyLog;
    static const char*	sKeyMarkers;
    static const char*	sKeyD2T;
    static const char*	sExtWell;
    static const char*	sExtLog;
    static const char*	sExtMarkers;
    static const char*	sExtD2T;

protected:

			IO(const char*,bool);

    StreamData		mkSD(const char* ext,int nr=0) const;
    const BufferString	basenm;

private:

    const bool		isrdr;

};


}; // namespace Well

#endif
