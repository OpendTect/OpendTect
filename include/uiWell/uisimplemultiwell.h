#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jun 2010
 * ID       : $Id$
-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "bufstringset.h"

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
    const BufferStringSet& createdWellIDs() const	{ return crwellids_; }

protected:

    uiTable*		tbl_;
    uiGenInput*		velfld_;
    const bool		zinft_;
    int			overwritepol_;
    float		vel_;
    Interval<float>	defzrg_;
    BufferStringSet	crwellids_;
    const UnitOfMeasure* zun_;

    bool		acceptOK(CallBacker*);

    void		rdFilePush(CallBacker*);
    bool		getWellCreateData(int,const char*,uiSMWCData&);
    bool		getWellCreateData(int,const char*,uiSMWCData&,
					  uiString&);
    bool		createWell(const uiSMWCData&,const IOObj&);
    bool		createWell(const uiSMWCData&,const IOObj&,uiString&);
    IOObj*		getIOObj(const char*);
    void		fillTable(const ObjectSet<uiSMWCData>&);
    void		fillRow(int,const uiSMWCData&);
    void		checkOverwritePolicy();

    mDeprecatedDef void addRow(const uiSMWCData&,int&);

    friend class	uiSimpleMultiWellCreateReadData;
    friend class	SimpleMultiWellImporter;

};

