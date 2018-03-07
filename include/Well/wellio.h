#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "bufstring.h"
class IOObj;
class uiString;


namespace Well
{

/*!\brief base class for Reader and Writer. */

mExpClass(Well) odIO
{ mODTextTranslationClass(Well::odIO)
public:

    const OD::String&	baseName() const	{ return basenm_; }

    const char*		getFileName(const char* ext,int nr=0) const;
    bool		removeAll(const char* ext) const;

    static const char*	sKeyWell();
    static const char*	sKeyTrack();
    static const char*	sKeyLog();
    static const char*	sKeyMarkers();
    static const char*	sKeyD2T();
    static const char*	sKeyDispProps();
    static const char*	sExtWell();
    static const char*	sExtLog();
    static const char*	sExtMarkers();
    static const char*	sExtD2T();
    static const char*	sExtCSMdl();
    static const char*	sExtDispProps();
    static const char*	sExtWellTieSetup();

    static const char*	getMainFileName(const IOObj&);
    static const char*	getMainFileName(const DBKey&);

    const uiString&	errMsg() const		{ return errmsg_; }

protected:


			odIO(const char*,uiString&);

    uiString&		errmsg_;
    const BufferString	basenm_;

public:

    static const char*	mkFileName(const char* basfnm,const char* ext,int nr=0);

};


}; // namespace Well
