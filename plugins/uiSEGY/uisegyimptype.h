#ifndef uisegyimptype_h
#define uisegyimptype_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2015
 RCS:		$Id:$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "typeset.h"
#include "uigroup.h"

class uiComboBox;


namespace SEGY
{

mExpClass(uiSEGY) ImpType
{ mODTextTranslationClass(ImpType)
public:

			ImpType(bool isvsp=false);
			ImpType(Seis::GeomType);

    Seis::GeomType	geomType() const
			{ return (Seis::GeomType)(isVSP() ? types_[0]
							  : types_[tidx_]); }
    bool		isVSP() const
			{ return types_[tidx_] > Seis::LinePS; }
    bool		is2D() const	{ return Seis::is2D(geomType()); }
    bool		isPS() const	{ return Seis::isPS(geomType()); }

    void		setGeomType(Seis::GeomType);
    void		setIsVSP()
			{ tidx_ = types_.size()-1; }

    int			tidx_;
    TypeSet<int>	types_;

    uiString		dispText() const;

private:

    void		init();

};

} // namespace SEGY


mExpClass(uiSEGY) uiSEGYImpType : public uiGroup
{ mODTextTranslationClass(uiSEGYImpType)
public:

			uiSEGYImpType(uiParent*,bool withvsp,
				      const uiString* lbltxt=0);

    const SEGY::ImpType& impType();
    void		setTypIdx(int);

    Notifier<uiSEGYImpType> typeChanged;

protected:

    uiComboBox*		fld_;
    mutable SEGY::ImpType typ_;

    void		typChg( CallBacker* )	{ typeChanged.trigger(); }

};



#endif
