#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jun 2010
-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "dbkey.h"

class IOObj;
class UnitOfMeasure;
class uiGenInput;
class uiSMWCData;
class uiTable;


mExpClass(uiWell) uiSimpleMultiWellCreate : public uiDialog
{ mODTextTranslationClass(uiSimpleMultiWellCreate);
public:
			uiSimpleMultiWellCreate(uiParent*);

    bool		wantDisplay() const;
    const DBKeySet&	createdWellIDs() const	{ return crwellids_; }

protected:

    uiTable*		tbl_;
    uiGenInput*		velfld_;
    const bool		zinft_;
    int			overwritepol_;
    float		vel_;
    Interval<float>	defzrg_;
    DBKeySet		crwellids_;
    const UnitOfMeasure* zun_;

    bool		acceptOK();

    void		rdFilePush(CallBacker*);
    bool		getWellCreateData(int,const char*,uiSMWCData&);
    bool		createWell(const uiSMWCData&,const IOObj&);
    IOObj*		getIOObj(const char*);
    void		addRow(const uiSMWCData&,int&);

    friend class	uiSimpleMultiWellCreateReadData;

};
