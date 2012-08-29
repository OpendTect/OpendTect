#ifndef uiwellpropertyrefsel_h
#define uiwellpropertyrefsel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          April 2011
 RCS:           $Id: uiwellpropertyrefsel.h,v 1.10 2012-08-29 17:12:45 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "multiid.h"
#include "propertyref.h"
#include "welllogset.h"
#include "uigroup.h"

class ElasticPropSelection;
class PropertyRef;
class UnitOfMeasure;

class uiLabel;
class uiComboBox;
class uiCheckBox;
class uiPushButton;
class uiUnitSel;

namespace Well { class LogSet; }


mClass(uiWell) uiPropSelFromList : public uiGroup
{
public:
			uiPropSelFromList(uiParent*,const PropertyRef&,
					const PropertyRef* alternatepr=0);
			~uiPropSelFromList();

    void                setNames(const BufferStringSet& nms);

    void                set(const char* txt,bool alt,const UnitOfMeasure* u=0);
    void                setCurrent(const char*);
    void                setUOM(const UnitOfMeasure&);

    const char*         text() const;
    const UnitOfMeasure* uom() const;
    void                getData(BufferString& nm,UnitOfMeasure& uom) const;

    void                setUseAlternate(bool yn);
    bool                isUseAlternate() const;

    const PropertyRef&  propRef() const;
    const PropertyRef*  altPropRef() const { return altpropref_; }

    uiComboBox*   	typeFld() const         { return typefld_; }

protected:
    const PropertyRef&  propref_;
    const PropertyRef*  altpropref_;

    uiLabel*            typelbl_;
    uiComboBox*         typefld_;
    uiUnitSel*          unfld_;
    uiCheckBox*         checkboxfld_;

    void                switchPropCB(CallBacker*);
};


mClass(uiWell) uiWellPropSel : public uiGroup
{
public:
			uiWellPropSel(uiParent*,const PropertyRefSelection&,
					bool withcreatelogs=false);

    void		setWellID(const MultiID& wid) { wellid_ = wid; }
    void		setLogs(const Well::LogSet&);

			//return true if succeded (std type found)
    bool		setLog(const PropertyRef::StdType,const char*,
	    			bool check,const UnitOfMeasure*);
    bool		getLog(const PropertyRef::StdType,BufferString&,
	    			bool&,BufferString& uom) const;

    virtual bool	isOK() const; 

   Notifier<uiWellPropSel>	logscreated; 

protected:
    void				initFlds();
    MultiID				wellid_;
    BufferStringSet			lognms_;
    const Well::LogSet*			logs_;

    const PropertyRefSelection&  	proprefsel_;
    ObjectSet<uiPropSelFromList> 	propflds_;
    ObjectSet<uiPushButton> 		createbuts_;

    static const char*			sKeyPlsSel() { return "Please select"; }
    void				createLogPushed(CallBacker*);

};




mClass(uiWell) uiWellElasticPropSel : public uiWellPropSel
{
public:
			uiWellElasticPropSel(uiParent*,bool withswaves=false);
			~uiWellElasticPropSel();

    bool		setDenLog(const char*,const UnitOfMeasure*);
    bool		getDenLog(BufferString&,BufferString& uom) const;

    bool		setVelLog(const char*,const UnitOfMeasure*,bool);
    bool		getVelLog(BufferString&,BufferString& uom,
	    			bool& isrev)const;
};


#endif

