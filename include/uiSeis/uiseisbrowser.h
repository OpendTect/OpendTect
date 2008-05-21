#ifndef uiseisbrowser_h
#define uiseisbrowser_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
 RCS:           $Id: uiseisbrowser.h,v 1.15 2008-05-21 08:40:40 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "executor.h"
#include "multiid.h"
#include "safefileio.h"
#include "seistype.h"
#include "samplingdata.h"
#include "position.h"
#include "linekey.h"

class CBVSSeisTrcTranslator;
class SeisTrc;
class SeisTrcBuf;
class uiSeisTrcBufViewer;
class SeisTrcBufDataPack;
class uiTable;
class uiGenInput;


class uiSeisBrowserInfoDlg : public uiDialog
{
    public :
			uiSeisBrowserInfoDlg(uiParent*,const SeisTrc&,bool);
	void		setTrace(const SeisTrc&);
  
    protected:
	bool		is2d_;
	uiGenInput*	coordfld_;
	uiGenInput*	trcnrbinidfld_;
	uiGenInput*	zrangefld_;
	uiGenInput*	samplefld_;
};


class uiSeisBrowser : public uiDialog
{
public :

    class Setup : public uiDialog::Setup
    {
    public:
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
	mDefSetupMemb(bool,readonly)

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

protected:

    CBVSSeisTrcTranslator* tr_;
    CBVSSeisTrcTranslator* tro_;
    CBVSSeisTrcTranslator* tri_;
    SeisTrcBuf&		tbuf_;
    SeisTrcBuf&		tbufchgdtrcs_;
    SeisTrc&		ctrc_;
    uiSeisTrcBufViewer*	strcbufview_;
    uiSeisBrowserInfoDlg*  infodlg_;
    const uiSeisBrowser::Setup& setup_;
    BufferString	title;

    bool		crlwise_;
    int			crlwisebutidx_;
    int			showwgglbutidx_;

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
    void		fillTableColumn(const SeisTrc&,int);
    BinID		getNextBid(const BinID&,int,bool) const;
    void		addTrc(SeisTrcBuf&,const BinID&);
    void		updateWiggleButtonStatus();
    void		setTrcBufViewTitle();
	
    void		goToPush(CallBacker*);
    void		infoPush(CallBacker*);
    void		rightArrowPush(CallBacker*);
    void		leftArrowPush(CallBacker*);
    void		switchViewTypePush(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		showWigglePush(CallBacker*);
    void		trcbufViewerClosed(CallBacker*);
    void		trcselectionChanged(CallBacker*);
    void		valChgReDraw(CallBacker*);

private:

    SeisTrcBuf&		tbufbefore_;
    SeisTrcBuf&		tbufafter_;
};


#endif
