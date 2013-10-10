#ifndef wellio_h
#define wellio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "bufstring.h"
class IOObj;


namespace Well
{

/*!\brief base class for Reader and Writer. */

mExpClass(Well) IO
{
public:

    const BufferString&	baseName() const	{ return basenm_; }

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


			IO(const char*);

    const BufferString	basenm_;

public:

    static const char*	mkFileName(const char* basfnm,const char* ext,int nr=0);

};


}; // namespace Well

#endif

