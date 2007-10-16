#ifndef uiseisbrowser_h
#define uiseisbrowser_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
 RCS:           $Id: uiseisbrowser.h,v 1.1 2007-10-16 16:27:58 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
#include "seistype.h"
#include "position.h"
#include "linekey.h"
class uiTable;
class SeisTrcReader;


class uiSeisBrowser : public uiDialog
{
public :

    struct Setup : public uiDialog::Setup
    {
    			Setup( const MultiID& mid, Seis::GeomType gt )
			    : uiDialog::Setup("Browse seismic data",
				    	      "", "103.1.5")
			    , id_(mid)
			    , geom_(gt)
			    , startpos_(mUdf(int),0)
			    , startz_(mUdf(float))	{}
	mDefSetupMemb(MultiID,id)
	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(BinID,startpos)
	mDefSetupMemb(float,startz)
	mDefSetupMemb(LineKey,linekey)

    };
			uiSeisBrowser(uiParent*,const Setup&);

    bool		isOK() const		{ return tbl_; }
    bool		setPos(const BinID&,float);

protected:

    uiTable*		tbl_;
    SeisTrcReader*	rdr_;
    uiToolBar*          tb_;

    bool		openData(const Setup&);
    void		createMenuAndToolBar();
    void		createTable();
    void		goToSelected( CallBacker* );
};


#endif
