#ifndef uiseisbrowser_h
#define uiseisbrowser_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
 RCS:           $Id: uiseisbrowser.h,v 1.2 2007-10-25 15:08:26 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
#include "seistype.h"
#include "samplingdata.h"
#include "position.h"
#include "linekey.h"
class SeisTrc;
class SeisTrcBuf;
class CBVSSeisTrcTranslator;
class uiTable;
class uiToolButton;

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
			    , startpos_(mUdf(int),mUdf(int))
			    , startz_(mUdf(float))	{}
	mDefSetupMemb(MultiID,id)
	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(BinID,startpos)
	mDefSetupMemb(float,startz)
	mDefSetupMemb(LineKey,linekey)

    };
			uiSeisBrowser(uiParent*,const Setup&);
			~uiSeisBrowser();

    bool		isOK() const			{ return tbl_; }
    void		setPos(const BinID&);
    bool		doSetPos( const BinID&, bool );
    void		setZ(float) ;
    void		setCrlWise( bool yn=true )	{ crlwise_ = yn; }
    void 		commitChanges();
    void		goTo( const BinID& );
	

    int			stepOut()			{ return stepout_; }
    void		setStepout(int);
    bool		is2D() const;

    const BinID&	curBinID() const;
    const float		curZ() const;

protected:

    CBVSSeisTrcTranslator* tr_;
    SeisTrcBuf&		tbuf_;
    SeisTrcBuf&		tbufchgdtrcs_;
    SeisTrc&		ctrc_;

    bool		crlwise_;
    int			crlwisebutidx_;

    int			stepout_;

    uiTable*		tbl_;
    uiToolBar*		uitb_;
    int			compnr_;
    int			nrcomps_;
    int			nrsamples_;
    SamplingData<float>	sd_;

    bool		openData(const Setup&);
    void		createMenuAndToolBar();
    void		createTable();
    void		fillTable();
    void		fillUdf(SeisTrc&);
    void		fillTableColumn(int);
    BinID		getNextBid(const BinID&,int,bool) const;
    void		addTrc(SeisTrcBuf&,const BinID&);
	
    void		goToPush(CallBacker*);
    void		infoPush(CallBacker*);
    void		rightArrowPush(CallBacker*);
    void		leftArrowPush(CallBacker*);
    void		switchViewTypePush(CallBacker*);
    bool		acceptOk( CallBacker* );

private:

    SeisTrcBuf&		tbufbefore_;
    SeisTrcBuf&		tbufafter_;
};


#endif
