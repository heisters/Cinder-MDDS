#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "MDDSMovie.h"

class MDDSSampleApp : public ci::app::AppNative {
public:
    void                    prepareSettings( Settings *settings );
	void                    setup();
	void                    update();
	void                    draw();

protected:
    mdds::MovieRef          mMovie;
};


using namespace ci;
using namespace ci::app;
using namespace std;


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
}

CINDER_APP_NATIVE( MDDSSampleApp, RendererGl )
