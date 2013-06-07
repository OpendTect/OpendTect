#ifndef uigmtoverlay_h
#define uigmtoverlay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtoverlay.h,v 1.3 2009/07/22 16:01:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiPushButton;
class GMTPar;

class uiGMTOverlayGrp : public uiDlgGroup
{
public:

    			uiGMTOverlayGrp(uiParent*,const char*);

    virtual bool	fillPar(IOPar&) const			=0;
    virtual bool	usePar(const IOPar&)			=0;
    virtual void	reset()					=0;

protected:

    static const char*	sKeyProgName();
    static const char*	sKeyUserRef();
};


typedef uiGMTOverlayGrp* (*uiGMTOverlayGrpCreateFunc)(uiParent*);

class uiGMTOverlayGrpFactory
{
public:

    int			add(const char* nm, uiGMTOverlayGrpCreateFunc);
    uiGMTOverlayGrp*	create(uiParent*,const char* nm) const;

    const char*		name(int) const;
    int			size() const	{ return entries_.size(); }

protected:

    struct Entry
    {
				Entry(	const char* nm,
					uiGMTOverlayGrpCreateFunc fn )
				    : name_(nm)
				    , crfn_(fn)		{}

	BufferString		name_;
	uiGMTOverlayGrpCreateFunc	crfn_;
    };

    ObjectSet<Entry>	entries_;

    Entry*		getEntry(const char*) const;

    friend uiGMTOverlayGrpFactory&	uiGMTOF();
};

uiGMTOverlayGrpFactory& uiGMTOF();

#define mGetColorString( col, str ) \
    str = (int) col.r(); \
    str += "/"; str += (int) col.g(); \
    str += "/"; str += (int) col.b();

#endif
