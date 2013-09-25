#include "MDDSMovie.h"
#include "cinder/app/App.h"
#include "cinder/Utilities.h"

using namespace std;
using namespace ci;
using namespace mdds;

/*******************************************************************************
 * Construction
 */

Movie::Movie( const fs::path &directory, const std::string &extension, const double fps ) :
mThreadIsRunning( false ),
mDataIsFresh( false ),
mLoopEnabled( true ),
mAverageFps( 0 ),
mFpsLastSampleTime( 0 ),
mFpsFrameCount( 0 ),
mFpsLastFrameCount( 0 )
{
    setFrameRate( fps );

    using namespace ci::fs;

    if ( !exists( directory ) ) throw LoadError( directory.string() + " does not exist" );
    if ( !is_directory( directory ) ) throw LoadError( directory.string() + " is not a directory" );

    mThreadData.extension       = extension;
    mThreadData.directoryPath   = directory;
    resetFramePosition();
    mThread                     = thread( bind( &Movie::updateFrameThreadFn, this ) );
    mThreadIsRunning            = true;
}

Movie::~Movie()
{
    mThreadIsRunning = false;
    mThread.join();
}


/*******************************************************************************
 * Exception Handling
 */

void
Movie::warn( const string &warning )
{
    cout << warning << endl;
}

/*******************************************************************************
 * Lifecycle
 */

void
Movie::update()
{
    if ( mDataIsFresh )
    {
        mMutex.lock();
        mTexture = ::mdds::Texture::loadDds( mThreadData.buffer->createStream(), ::mdds::Texture::Format() );
        mMutex.unlock();

        if ( mTexture == nullptr ) warn( "error creating texture" );
        	
        mDataIsFresh = false;
    }
}

void
Movie::draw()
{
    if ( mTexture ) gl::draw( mTexture );
}

/*******************************************************************************
 * Play control
 */

void
Movie::setFrameRate( const double fps )
{
    mFrameRate = fps;
    mNextFrameTime = app::getElapsedSeconds();
}

double
Movie::getFrameRate() const
{
    return mFrameRate;
}

void
Movie::updateAverageFps()
{
    double now = app::getElapsedSeconds();
    mFpsFrameCount++;
    
    if( now > mFpsLastSampleTime + app::App::get()->getFpsSampleInterval() ) {
        //calculate average Fps over sample interval
        uint32_t framesPassed = mFpsFrameCount - mFpsLastFrameCount;
        mAverageFps = framesPassed / (now - mFpsLastSampleTime);
        mFpsLastSampleTime = now;
        mFpsLastFrameCount = mFpsFrameCount;
    }
}

double
Movie::getAverageFps() const
{
    return mAverageFps;
}


/*******************************************************************************
 * Async
 */

void
Movie::updateFrameThreadFn()
{
    using namespace ci::fs;

    ci::ThreadSetup threadSetup;


    mNextFrameTime = app::getElapsedSeconds();
    
    while ( mThreadIsRunning )
    {
        if ( mThreadData.directoryIt->path().extension() != mThreadData.extension )
        {
            // Skip this frame
            incFramePosition();
            continue;
        }

        // Read data into memory buffer
        auto ds_path = DataSourcePath::create( mThreadData.directoryIt->path() );
        mMutex.lock();
        mThreadData.buffer = DataSourceBuffer::create( ds_path->getBuffer() );
        mMutex.unlock();

        mDataIsFresh = true;
        updateAverageFps();

        incFramePosition();


        // FrameRate control, cribbed from AppImplMswBasic.cpp
        double currentSeconds   = app::getElapsedSeconds();
        double secondsPerFrame  = 1.0 / mFrameRate;
        mNextFrameTime          = mNextFrameTime + secondsPerFrame;
        if ( mNextFrameTime > currentSeconds )
            ci::sleep( (mNextFrameTime - currentSeconds) * 1000.0 );
    }
}

/*******************************************************************************
 * Position control
 */

void
Movie::incFramePosition()
{
    ++mThreadData.directoryIt;
    if ( mThreadData.directoryIt == ci::fs::directory_iterator() && mLoopEnabled ) resetFramePosition();
}

void
Movie::resetFramePosition()
{
    mThreadData.directoryIt = ci::fs::directory_iterator( mThreadData.directoryPath );
}