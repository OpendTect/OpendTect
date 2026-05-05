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

#include "pickset.h"
#include "randcolor.h"

#include "factory.h"
#include "multiid.h"
#include "uicoordsystem.h"

class uiCheckBox;
class uiColorInput;
class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiLabeledComboBox;
class uiPickPartServer;
class uiTableImpDataSel;
class IOObj;

namespace Coords { class uiCoordSystemSel; }
namespace Pick { class Set; }
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

    bool		isPointSet() const;
    bool		isPolyLine() const;
    bool		isPolygon() const;
    uiString		getPointSetShape();

    int			getImportCount();
    void		resetImportCount();

    bool		handleDuplicateNames(ObjectSet<Pick::Set>& pointsets,
					     BufferStringSet& duplicatenames);
    bool		setToNextAvailableName(Pick::Set&);
    uiRetVal		addPointSet(Pick::Set&,IOObj* ioobj=nullptr,
				    OD::Color col=OD::getRandomFillColor());
    void		displayPointSet(Pick::Set&,MultiID);

protected:
			uiPickSetImportGroup(uiParent*);

    uiGenInput*		shapefld_			= nullptr;
    uiLabeledComboBox*	zfld_				= nullptr;
    uiLabeledComboBox*	horinpfld_			= nullptr;
    uiGenInput*		constzfld_			= nullptr;
    uiTableImpDataSel*	dataselfld_			= nullptr;
    uiFileInput*	filefld_;
    int			impcount_			= 0;
    bool		importready_			= false;

    uiPickPartServer*	serv_				= nullptr;
};


mExpClass(uiIo) uiSinglePickSetImportGroup : public uiPickSetImportGroup
{
mODTextTranslationClass(uiSinglePickSetImportGroup)
public:
			mDefaultFactoryInstantiation1Param(uiPickSetImportGroup,
					uiSinglePickSetImportGroup,uiParent*,
					"single_pickset",
					toUiString("Single PointSet"));

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
    void		formatSelCB(CallBacker*);
    bool		checkInpFlds();

    Table::FormatDesc*	fd_				= nullptr;
    MultiID		storedid_			= MultiID::udf();
};



mExpClass(uiIo) uiImportPickSet : public uiDialog
{
mODTextTranslationClass(uiImportPickSet)
public:
			uiImportPickSet(uiParent*,uiPickPartServer*);
			~uiImportPickSet();

    TypeSet<MultiID>	getStoredIDs() const;

    Notifier<uiImportPickSet> importReady;

protected:
    uiComboBox*			    optionfld_		= nullptr;
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

mExpClass(uiIo) uiImpExpPickSet : public uiDialog
{ mODTextTranslationClass(uiImpExpPickSet);
public:
			uiImpExpPickSet(uiParent*,uiPickPartServer*,bool);
			~uiImpExpPickSet();

    const MultiID&	getStoredID() const	{ return storedid_; }

    Notifier<uiImpExpPickSet> importReady;

protected:

    uiIOObjSel*		objfld_;
    uiIOObjSel*		horinputfld_;
    uiColorInput*	colorfld_;
    uiGenInput*		polyfld_;
    uiLabeledComboBox*	zfld_;
    uiLabeledComboBox*  horinpfld_;
    uiGenInput*		constzfld_;
    uiTableImpDataSel*  dataselfld_;
    uiFileInput*	filefld_;
    Coords::uiCoordSystemSel* coordsysselfld_;
    uiPickPartServer*	serv_;

    bool		acceptOK(CallBacker*) override;
    void		inputChgd(CallBacker*);
    void		formatSel(CallBacker*);
    bool		checkInpFlds();
    bool		doImport();
    bool		doExport();

    bool		import_;
    Table::FormatDesc&  fd_;

    MultiID		storedid_;

private:
    static uiString	sPicksetPolygon() { return tr("Pointset/Polygon"); }
};
