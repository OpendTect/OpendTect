#ifndef uiseisbrowser_h
#define uiseisbrowser_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
 RCS:           $Id: uiseisbrowser.h,v 1.7 2007-12-10 04:24:38 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "arraynd.h"
#include "multiid.h"
#include "seistype.h"
#include "samplingdata.h"
#include "position.h"
#include "linekey.h"

class CBVSSeisTrcTranslator;
class CBVSWriteMgr;
class PosAuxInfo;
class SeisTrc;
class SeisTrcBuf;
class uiSeisTrcBufViewer;
class uiTable;
class uiToolButton;

struct VBrickSpec;

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
    CBVSWriteMgr* 	wrtr_;
    VBrickSpec* 	vbspec_;
    SeisTrcBuf&		tbuf_;
    SeisTrcBuf&		tbufchgdtrcs_;
    SeisTrc&		ctrc_;
    PosAuxInfo*		painfo_;
    uiSeisTrcBufViewer*	strcbufview_;
    const uiSeisBrowser::Setup& setup_;

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
    Array2D<float>* 	arr_;
    unsigned char**     targetptrs;
    int			coordpol;

    bool		openData(const Setup&);
    void		createMenuAndToolBar();
    void		createTable();
    void		fillTable();
    void		fillUdf(SeisTrc&);
    void		fillTableColumn(int);
    BinID		getNextBid(const BinID&,int,bool) const;
    void		addTrc(SeisTrcBuf&,const BinID&);
    void		updateWiggleButtonStatus();
	
    void		goToPush(CallBacker*);
    void		infoPush(CallBacker*);
    void		rightArrowPush(CallBacker*);
    void		leftArrowPush(CallBacker*);
    void		switchViewTypePush(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		showWigglePush(CallBacker*);
    void		trcbufViewerClosed(CallBacker*);
    void		valChgReDraw(CallBacker*);

private:

    SeisTrcBuf&		tbufbefore_;
    SeisTrcBuf&		tbufafter_;
};


#endif
