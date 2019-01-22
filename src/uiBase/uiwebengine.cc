/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A. Huck
 Date:          December 2018
________________________________________________________________________

-*/


#include "uiwebengine.h"

#include "uiobjbody.h"
#include "uifont.h"
#include "i_qwebengineview.h"

#include <QWebEngineView>

mUseQtnamespace


uiWebEngineBase::uiWebEngineBase( uiParent* p, const char* nm,
				  uiObjectBody& bdy )
    : uiObject(p,nm,bdy)
{
}



class uiWebEngineViewBody : public uiObjBodyImpl<uiWebEngine,QWebEngineView>
{
public:

			uiWebEngineViewBody(uiWebEngine&,uiParent*,
					    const char* nm);
			~uiWebEngineViewBody()	{ delete &messenger_; }

protected:
    i_WebEngineViewMessenger& messenger_;

};


uiWebEngineViewBody::uiWebEngineViewBody( uiWebEngine& hndl, uiParent* p,
					  const char* nm )
    : uiObjBodyImpl<uiWebEngine,QWebEngineView>( hndl, p, nm )
    , messenger_(*new i_WebEngineViewMessenger(this,&hndl))
{
}


//-------------------------------------------------------

uiWebEngine::uiWebEngine( uiParent* parnt, const char* nm )
    : uiWebEngineBase( parnt, nm, mkbody(parnt,nm) )
{
}


uiWebEngineViewBody& uiWebEngine::mkbody( uiParent* parnt, const char* nm )
{
    body_= new uiWebEngineViewBody( *this, parnt, nm );
    return *body_;
}


QWebEngineView& uiWebEngine::qte()
{ return *body_; }


void uiWebEngine::setUrl( const char* urlstr )
{
    body_->setUrl( QUrl(urlstr) );
}


void uiWebEngine::back()
{
    body_->back();
}


void uiWebEngine::forward()
{
    body_->forward();
}


void uiWebEngine::reload()
{
    body_->reload();
}


void uiWebEngine::stop()
{
    body_->stop();
}

