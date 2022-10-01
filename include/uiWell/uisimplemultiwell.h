#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "bufstringset.h"

class IOObj;
class UnitOfMeasure;
class uiGenInput;
class uiProgressBar;
class uiSMWCData;
class uiTable;


mExpClass(uiWell) uiSimpleMultiWellCreate : public uiDialog
{ mODTextTranslationClass(uiSimpleMultiWellCreate);
public:
			uiSimpleMultiWellCreate(uiParent*);
			~uiSimpleMultiWellCreate();

    bool		wantDisplay() const;
    const TypeSet<MultiID>& createdWellIDs() const	{ return crwellids_; }

protected:

    uiTable*		tbl_;
    uiGenInput*		velfld_;
    uiProgressBar*	progbar_;

    const bool		zinft_;
    int			overwritepol_;
    float		vel_;
    Interval<float>	defzrg_;
    TypeSet<MultiID>	crwellids_;
    const UnitOfMeasure* zun_;

    bool		acceptOK(CallBacker*) override;

    void		rdFilePush(CallBacker*);
    bool		getWellCreateData(int,const char*,uiSMWCData&);
    bool		createWell(const uiSMWCData&,const IOObj&);
    IOObj*		getIOObj(const char*);
    void		fillTable(const ObjectSet<uiSMWCData>&);
    void		fillRow(int,const uiSMWCData&);

    mDeprecatedDef void addRow(const uiSMWCData&,int&);

    friend class	uiSimpleMultiWellCreateReadData;
    friend class	SimpleMultiWellImporter;
};
