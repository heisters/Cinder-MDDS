#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/Text.h"
#include "cinder/Rand.h"

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
        mMovie = mdds::Movie::create( getFolderPath(), ".DDS", 29.97 );
    }
    catch ( mdds::Movie::LoadError boom )
    {
        console() << "Error loading movie: " << boom.what() << endl;
    }
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
    info.addLine( (boost::format( "Movie FPS: %.2d" ) % mMovie->getFrameRate()).str() );
    info.addLine( (boost::format( "Play rate: %.2d" ) % mMovie->getPlayRate()).str() );
    info.addLine( (boost::format( "Average playback FPS: %.2d" ) % mMovie->getAverageFps()).str() );
    info.addLine( "Controls:" );
    info.addLine( "↑: double playback rate" );
    info.addLine( "↓: halve playback rate" );
    info.addLine( "f: play forward at normal rate" );
    info.addLine( "r: play reverse at normal rate" );
    info.addLine( "space: pause" );
    info.addLine( "↵: jump to random frame" );
    gl::draw( gl::Texture( info.render( true ) ), Vec2f( 10, 10 ) );
}

void
MDDSSampleApp::keyDown( KeyEvent event )
{
    if ( event.getChar() == 'f' )
        mMovie->setPlayRate( 1.0 );
    else if ( event.getChar() == 'r' )
        mMovie->setPlayRate( -1.0 );
    else if ( event.getCode() == KeyEvent::KEY_UP )
        mMovie->setPlayRate( mMovie->getPlayRate() * 2.0 );
    else if ( event.getCode() == KeyEvent::KEY_DOWN )
        mMovie->setPlayRate( mMovie->getPlayRate() * 0.5 );
    else if ( event.getCode() == KeyEvent::KEY_SPACE )
        mMovie->setPlayRate( 0.0 );
    else if ( event.getCode() == KeyEvent::KEY_RETURN )
        mMovie->seekToFrame( Rand::randInt( mMovie->getNumFrames() ) );

}

CINDER_APP_NATIVE( MDDSSampleApp, RendererGl )
