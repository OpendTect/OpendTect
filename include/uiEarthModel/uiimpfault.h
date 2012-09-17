#ifndef uiimpfault_h
#define uiimpfault_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.h,v 1.15 2011/05/09 05:42:38 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;

namespace EM { class Fault3D; class Fault; }
namespace Table { class FormatDesc; }

/*! \brief Dialog for fault import */

mClass uiImportFault : public uiDialog
{
public:
			~uiImportFault();
    MultiID		getSelID() const;

    Notifier<uiImportFault> importReady;

protected:
			uiImportFault(uiParent*,const char*,bool is2d=false);

    void		createUI();
    void		typeSel(CallBacker*);
    void		stickSel(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();
    bool		handleLMKAscii();
    virtual bool	acceptOK(CallBacker*) { return false; }
    virtual bool	getFromAscIO(std::istream&,EM::Fault&);
    EM::Fault*		createFault() const;

    uiFileInput*	infld_;
    uiFileInput*	formatfld_;
    uiGenInput*		typefld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		sortsticksfld_;
    uiGenInput*		stickselfld_;
    uiGenInput*		thresholdfld_;
    CtxtIOObj&		ctio_;
    Table::FormatDesc*	fd_;
    uiTableImpDataSel*	dataselfld_;
    bool		isfss_;
    const char*		type_;
    bool		is2d_;

    static const char*	sKeyAutoStickSel();
    static const char*	sKeyInlCrlSep();
    static const char*	sKeySlopeThres();
    static const char*  sKeyGeometric();
    static const char*  sKeyIndexed();
    static const char*  sKeyFileOrder();
};


mClass uiImportFault3D : public uiImportFault
{
public:
    			uiImportFault3D(uiParent*,const char* type);
protected:
    bool		acceptOK(CallBacker*);			
};

#endif
