#ifndef uiseisbrowser_h
#define uiseisbrowser_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"
#include "multiid.h"
#include "seistype.h"
#include "samplingdata.h"
#include "position.h"
#include "linekey.h"
class IOObj;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcBufDataPack;
class CBVSSeisTrcTranslator;
class uiTable;
class uiComboBox;
class uiSeisTrcBufViewer;
class uiSeisBrowserInfoVwr;
namespace ZDomain { class Def; }


mClass uiSeisBrowser : public uiDialog
{
public :

    mClass Setup : public uiDialog::Setup
    {
    public:
    			Setup(const MultiID&,Seis::GeomType);
	mDefSetupMemb(MultiID,id)
	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(BinID,startpos)
	mDefSetupMemb(float,startz)
	mDefSetupMemb(LineKey,linekey)
	mDefSetupMemb(bool,readonly)	//!< def: true

    };
			uiSeisBrowser(uiParent*,const Setup&,bool);
			~uiSeisBrowser();

    bool		isOK() const			{ return tbl_; }
    void		setPos(const BinID&);
    bool		doSetPos( const BinID&, bool );
    void		setZ(float) ;
    void		setCrlWise( bool yn=true )	{ crlwise_ = yn; }
    void 		commitChanges();
    bool 		storeChgdData();
    bool		goTo(const BinID&);
	

    int			stepOut()			{ return stepout_; }
    void		setStepout(int);
    bool		is2D() const;
    bool 		is2d_;

    const BinID&	curBinID() const;
    const float		curZ() const;
    void		setCompNr( int compnr )		{ compnr_ = compnr; }

    static void		doBrowse(uiParent*,const IOObj&,bool is2d,
	    			 const LineKey* lk=0);

protected:

    const Setup		setup_;
    uiSeisBrowserInfoVwr* infovwr_;
    uiSeisTrcBufViewer*	trcbufvwr_;

    SeisTrcBuf&		tbuf_;
    SeisTrcBuf&		tbufchgdtrcs_;
    SeisTrc&		ctrc_;
    CBVSSeisTrcTranslator* tr_;
    CBVSSeisTrcTranslator* tro_;
    CBVSSeisTrcTranslator* tri_;
    const ZDomain::Def*	zdomdef_;

    bool		crlwise_;
    int			crlwisebutidx_;
    int			showwgglbutidx_;
    uiComboBox*		selcompnmfld_;

    int			stepout_;

    uiTable*		tbl_;
    uiToolBar*		uitb_;
    int			compnr_;
    int			nrcomps_;
    BufferStringSet	compnms_;
    int			nrsamples_;
    SamplingData<float>	sd_;

    bool		openData(const Setup&);
    void		createMenuAndToolBar();
    void		createTable();
    void		fillTable();
    void		fillUdf(SeisTrc&);
    void		fillTableColumn(const SeisTrc&,int);
    BinID		getNextBid(const BinID&,int,bool) const;
    void		addTrc(SeisTrcBuf&,const BinID&);
    void		updateWiggleButtonStatus();
    void		setTrcBufViewTitle();
	
    void		goToPush(CallBacker*);
    void		infoPush(CallBacker*);
    void		infoClose(CallBacker*);
    void		rightArrowPush(CallBacker*);
    void		leftArrowPush(CallBacker*);
    void		switchViewTypePush(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		dispTracesPush(CallBacker*);
    void		trcbufViewerClosed(CallBacker*);
    void		trcselectionChanged(CallBacker*);
    void		valChgReDraw(CallBacker*);
    void		chgCompNrCB(CallBacker*);

private:

    SeisTrcBuf&		tbufbefore_;
    SeisTrcBuf&		tbufafter_;
};


#endif
