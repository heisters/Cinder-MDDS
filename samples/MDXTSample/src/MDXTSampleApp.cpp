#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class MDXTSampleApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void MDXTSampleApp::setup()
{
}

void MDXTSampleApp::mouseDown( MouseEvent event )
{
}

void MDXTSampleApp::update()
{
}

void MDXTSampleApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( MDXTSampleApp, RendererGl )
