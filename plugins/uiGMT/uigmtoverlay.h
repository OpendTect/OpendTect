#ifndef uigmtoverlay_h
#define uigmtoverlay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiPushButton;
class GMTPar;

mClass(uiGMT) uiGMTOverlayGrp : public uiDlgGroup
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

mClass(uiGMT) uiGMTOverlayGrpFactory
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
