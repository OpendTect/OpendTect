#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uigroup.h"

#include "factory.h"
#include "multiid.h"
#include "uicoordsystem.h"

class uiCheckBox;
class uiColorInput;
class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiGenInput;
class uiIOObjSel;
class uiLabeledComboBox;
class uiPickPartServer;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

/*! \brief Dialog for pickset selection */

mExpClass(uiIo) uiPickSetImportGroup : public uiGroup
{
mODTextTranslationClass(uiImpPickSetGroup)
public:
			mDefineFactory1ParamInClass(uiPickSetImportGroup,
						    uiParent*,factory);

			~uiPickSetImportGroup();

    void		setPartServer(uiPickPartServer*);
    virtual bool	init();

    virtual bool	doImport(bool dodisplay)			= 0;
    virtual bool	triggerImportReady() const			= 0;
    virtual TypeSet<MultiID> storedIDs() const				= 0;
    bool		isPolygon() const;

protected:
			uiPickSetImportGroup(uiParent*);

    uiGenInput*		polyfld_			= nullptr;
    uiLabeledComboBox*	zfld_				= nullptr;
    uiLabeledComboBox*	horinpfld_			= nullptr;
    uiGenInput*		constzfld_			= nullptr;
    uiTableImpDataSel*	dataselfld_			= nullptr;
    uiFileInput*	filefld_;

    uiPickPartServer*	serv_					= nullptr;
};


mExpClass(uiIo) uiSinglePickSetImportGroup : public uiPickSetImportGroup
{
mODTextTranslationClass(uiSinglePickSetImportGroup)
public:
mDefaultFactoryInstantiation1Param(uiPickSetImportGroup,
		uiSinglePickSetImportGroup,uiParent*,
		"single_pickset",toUiString("Single PickSet"));

			~uiSinglePickSetImportGroup();

protected:
			uiSinglePickSetImportGroup(uiParent*);

    uiIOObjSel*		objfld_;
    uiColorInput*	colorfld_;

    bool		doImport(bool display) override;
    bool		triggerImportReady() const override
			{ return importready_; }
    TypeSet<MultiID>	storedIDs() const override;
    void		inputChgd(CallBacker*);
    void		formatSel(CallBacker*);
    bool		checkInpFlds();

    Table::FormatDesc&  fd_;
    MultiID		storedid_			= MultiID::udf();
    bool		importready_			= false;
};



mExpClass(uiIo) uiImportPickSet : public uiDialog
{
mODTextTranslationClass(uiImportPickSet)
public:
			uiImportPickSet(uiParent*,uiPickPartServer*);
			~uiImportPickSet();

    MultiID		getStoredID() const;

    Notifier<uiImportPickSet> importReady;

protected:
    uiComboBox*			optionfld_;
    ObjectSet<uiPickSetImportGroup> groups_;

    void		optionSelCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};


mExpClass(uiIo) uiExportPickSet : public uiDialog
{
mODTextTranslationClass(uiExportPickSet)
public:
			uiExportPickSet(uiParent*);
			~uiExportPickSet();

protected:

    uiIOObjSel*		objfld_;
    uiFileInput*	filefld_;
    Coords::uiCoordSystemSel* coordsysselfld_		= nullptr;

    bool		acceptOK(CallBacker*) override;
    bool		checkInpFlds();
    bool		doExport();

private:
    static uiString	sPicksetPolygon() { return tr("Pointset/Polygon"); }
};
