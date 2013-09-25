#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/Text.h"

#include <boost/format.hpp>

#include "MDDSMovie.h"



class MDDSSampleApp : public ci::app::AppNative {
public:
    MDDSSampleApp();

    // Lifecycle ---------------------------------------------------------------
    void                    prepareSettings( Settings *settings );
	void                    setup();
	void                    update();
	void                    draw();

    // Events ------------------------------------------------------------------
    void                    keyDown( ci::app::KeyEvent event );

protected:
    mdds::MovieRef          mMovie;
    ci::Font                mFont;
};


using namespace ci;
using namespace ci::app;
using namespace std;

MDDSSampleApp::MDDSSampleApp() :
mFont( "Helvetica", 14 )
{

}

void
MDDSSampleApp::prepareSettings( Settings *settings )
{
    settings->setWindowSize( 1920, 1080 );
    settings->setFrameRate( 60 );
}

void
MDDSSampleApp::setup()
{
    try
    {
        mMovie = mdds::Movie::create( getFolderPath() );
    }
    catch ( mdds::Movie::LoadError boom )
    {
        console() << "Error loading movie: " << boom.what() << endl;
    }

    mMovie->setFramerate( 30 );
}

void
MDDSSampleApp::update()
{
    if ( mMovie ) mMovie->update();
}

void
MDDSSampleApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );

    if ( mMovie ) mMovie->draw();


    TextLayout info;
    info.clear( ColorA( 0.2f, 0.2f, 0.2f, 0.5f ) );
    info.setColor( ColorA::white() );
    info.setBorder( 4, 2 );
    info.setFont( mFont );
    info.addLine( (boost::format( "App FPS: %.2d" ) % getAverageFps()).str() );
    info.addLine( (boost::format( "Target playback FPS: %.2d" ) % mMovie->getFramerate()).str() );
    info.addLine( (boost::format( "Average playback FPS: %.2d" ) % mMovie->getAverageFps()).str() );
    info.addLine( "Use up/down arrows to adjust movie playback rate" );
    gl::draw( gl::Texture( info.render( true ) ), Vec2f( 10, 10 ) );
}

void
MDDSSampleApp::keyDown( KeyEvent event )
{
    if ( event.getCode() == KeyEvent::KEY_DOWN )
        mMovie->setFramerate( mMovie->getFramerate() - 1 );
    else if ( event.getCode() == KeyEvent::KEY_UP )
        mMovie->setFramerate( mMovie->getFramerate() + 1 );
}

CINDER_APP_NATIVE( MDDSSampleApp, RendererGl )
