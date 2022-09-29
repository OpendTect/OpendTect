#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class od_istream;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;

namespace EM { class Fault3D; class Fault; }
namespace Table { class FormatDesc; }

/*! \brief Dialog for fault import */

mExpClass(uiEarthModel) uiImportFault : public uiDialog
{
mODTextTranslationClass(uiImportFault);
public:
			~uiImportFault();

    MultiID		getSelID() const;

    Notifier<uiImportFault> importReady;

protected:
			uiImportFault(uiParent*,const char*,bool is2d=false);

    void		createUI();
    void		inputChgd(CallBacker*);
    void		typeSel(CallBacker*);
    void		stickSel(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();
    bool		handleLMKAscii();
    bool		acceptOK(CallBacker*) override { return false; }
    virtual bool	getFromAscIO(od_istream&,EM::Fault&);
    EM::Fault*		createFault() const;

    uiFileInput*	infld_;
    uiFileInput*	formatfld_;
    uiGenInput*		typefld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		sortsticksfld_;
    uiGenInput*		stickselfld_;
    uiGenInput*		thresholdfld_;
    Table::FormatDesc*	fd_;
    uiTableImpDataSel*	dataselfld_;
    bool		isfss_;
    const char*		type_;
    bool		is2d_;

    static const char*	sKeyAutoStickSel();
    static const char*	sKeyInlCrlSep();
    static const char*	sKeySlopeThres();
    static const char*	sKeyGeometric();
    static const char*	sKeyIndexed();
    static const char*	sKeyFileOrder();
};


/*Brief Dialog for 3D Fault*/
mExpClass(uiEarthModel) uiImportFault3D : public uiImportFault
{
mODTextTranslationClass(uiImportFault3D);
public:
			uiImportFault3D(uiParent*,const char* type);
			~uiImportFault3D();

protected:
    bool		acceptOK(CallBacker*) override;
};


/*Brief Dialog for 2D FaultStickSet*/

mExpClass(uiEarthModel) uiImportFaultStickSet2D : public uiImportFault
{
mODTextTranslationClass(uiImportFaultStickSet2D);
public:
			uiImportFaultStickSet2D(uiParent*,const char* type);
			~uiImportFaultStickSet2D();

protected:

    bool		acceptOK(CallBacker*) override;
    bool		getFromAscIO(od_istream&,EM::Fault&) override;

};
