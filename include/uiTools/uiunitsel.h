#ifndef uiunitsel_h
#define uiunitsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Feb 2010
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "uigroup.h"
#include "propertyref.h"

class uiComboBox;
class UnitOfMeasure;


/*!\brief Selector for UnitOfMeasure's

  Uses the UnitOfMeasure::currentDefaults() to get/set a table of 'last used'
  unit of measure values. This list provides the start-up seletion. Therefore,
  make sure the values you display are converted to user values after the
  uiUnitSel is constructed.

  The currentDefaults() key is taken from the label text, if you want no label
  then you probably want to set a key. If none is known then the name of the
  Property Type will be used.

  */

mExpClass(uiTools) uiUnitSel : public uiGroup
{
public:
    				uiUnitSel(uiParent*,PropertyRef::StdType,
					  const char* lbltxt=0,
					  bool dispsymbols=false,
					  bool withempty=false);

    void			setUnit(const UnitOfMeasure* uom=0);
    void			setUnit(const char*);
    const UnitOfMeasure*	getUnit() const;
    const char*			getUnitName() const;

    float			getUserValue(float internalval) const;
    double			getUserValue(double internalval) const;
    float			getInternalValue(float uservalue) const;
    double			getInternalValue(double uservalue) const;


    PropertyRef::StdType	propType() const	{ return proptype_; }
    void			setPropType(PropertyRef::StdType);

    uiComboBox*			inpFld() const	{ return inpfld_; }

    Notifier<uiUnitSel>		selChange;

    void			fillPar(IOPar&,const char* altkey=0) const;
    bool			usePar(const IOPar&,const char* altkey=0);

    void			setKey(const char*);
				    //!< For UnitOfMeasure::currentDefaults()
				    //!< default is lbltxt, or else prop std nm
    static IOPar&		lastUsed();
				    //!< == UnitOfMeasure::currentDefaults()

protected:

    PropertyRef::StdType	proptype_;
    ObjectSet<const UnitOfMeasure> units_;
    bool			symbolsdisp_;
    BufferString		tblkey_;

    uiComboBox*			inpfld_;
    bool 			withempty_;

    void			selChg( CallBacker* )	{ selChange.trigger(); }
    void			update();
    const UnitOfMeasure*	gtUnit() const;

};


#endif
