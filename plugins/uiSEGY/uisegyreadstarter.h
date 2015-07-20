#ifndef uisegyreadstarter_h
#define uisegyreadstarter_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "segyfiledef.h"

class uiGenInput;
class uiFileInput;


/*!\brief Starts reading process of 'any SEG-Y file'. */

mExpClass(uiSEGY) uiSEGYReadStarter : public uiDialog
{ mODTextTranslationClass(uiSEGYReadStarter);
public:

    			uiSEGYReadStarter(uiParent*,const char* filenm=0);
    			~uiSEGYReadStarter();

    Seis::GeomType	geomType() const	{ return geomtype_; }
    bool		isVSP() const		{ return isvsp_; }
    bool		isMulti() const		{ return filespec_.isMulti(); }
    const char*		fileName( int nr=0 ) const
			{ return filespec_.fileName(nr); }
    const SEGY::FileSpec& fileSpec() const	{ return filespec_; }

protected:

    Seis::GeomType	geomtype_;
    bool		isvsp_;
    SEGY::FileSpec	filespec_;

    uiFileInput*	inpfld_;
    uiGenInput*		typfld_;
    TypeSet<int>	inptyps_; // Seis::GeomType, or -1 for VSP

    void		addTyp(uiStringSet&,int);
    void		inpSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};


#endif
