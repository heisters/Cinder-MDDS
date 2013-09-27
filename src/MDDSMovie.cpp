#include "MDDSMovie.h"
#include "cinder/app/App.h"
#include "cinder/Utilities.h"
#include "cinder/CinderMath.h"

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
mFpsLastFrameCount( 0 ),
mFrameRate( fps ),
mNextFrameTime( app::getElapsedSeconds() ),
mInterruptTriggeredFoRealz( false ),
mCurrentFrameIdx( 0 ),
mCurrentFrameIsFresh( false ),
mNumFrames( 0 )
{
    setPlayRate( 1.0 );

    using namespace ci::fs;

    if ( !exists( directory ) ) throw LoadError( directory.string() + " does not exist" );
    if ( !is_directory( directory ) ) throw LoadError( directory.string() + " is not a directory" );


    mThreadData.extension       = extension;
    mThreadData.directoryPath   = directory;

    readFramePaths();

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
        {
            lock_guard< mutex > lock( mMutex );
            mTexture = ::mdds::Texture::loadDds( mThreadData.buffer->createStream(), ::mdds::Texture::Format() );
        }

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

void
Movie::setPlayRate( const double newRate )
{
    mInterruptTriggeredFoRealz = newRate != mPlayRate;
    mPlayRate = newRate;

    if ( mInterruptTriggeredFoRealz )
    {
        lock_guard< mutex > lock( mMutex );
        mInterruptFrameRateSleepCv.notify_all();
    }
}

double
Movie::getPlayRate() const
{
    return mPlayRate;
}

void
Movie::readFramePaths()
{
    using namespace ci::fs;

    mThreadData.framePaths.clear();

    for ( auto it = directory_iterator( mThreadData.directoryPath ); it != directory_iterator(); it++ )
    {
        if ( it->path().extension() != mThreadData.extension ) continue;

        mThreadData.framePaths.push_back( it->path() );
    }

    mNumFrames = mThreadData.framePaths.size();
}

void
Movie::seekToTime( const double seconds )
{
    seekToFrame( seconds * mFrameRate );
}

void
Movie::seekToFrame( const size_t frame )
{
    mCurrentFrameIdx = frame;
    mCurrentFrameIsFresh = true;
    mInterruptTriggeredFoRealz = true;
    lock_guard< mutex > lock( mMutex );
    mInterruptFrameRateSleepCv.notify_all();
}

void
Movie::seekToStart()
{
    seekToFrame( 0 );
}

void
Movie::seekToEnd()
{
    seekToFrame( -1 );
}

size_t
Movie::getCurrentFrame() const
{
    return mCurrentFrameIdx;
}

size_t
Movie::getNumFrames() const
{
    return mNumFrames;
}

double
Movie::getCurrentTime() const
{
    return (double)mCurrentFrameIdx / mFrameRate;
}

double
Movie::getDuration() const
{
    return (double)mNumFrames / mFrameRate;
}

/*******************************************************************************
 * Async
 */

void
Movie::updateFrameThreadFn()
{
    using namespace ci::fs;

    ci::ThreadSetup threadSetup;

    while ( mThreadIsRunning )
    {
        mNextFrameTime = app::getElapsedSeconds();
        
        unique_lock< mutex > lock( mMutex );

        // Read data into memory buffer
        auto ds_path = DataSourcePath::create( mThreadData.framePaths[ mCurrentFrameIdx ] );
        mThreadData.buffer = DataSourceBuffer::create( ds_path->getBuffer() );


        mDataIsFresh = true;
        updateAverageFps();

        nextFramePosition();


        // FrameRate control, cribbed from AppImplMswBasic.cpp
        double currentSeconds   = app::getElapsedSeconds();
        double secondsPerFrame  = mPlayRate == 0.0 ? 1.0 : ((1.0 / math< double >::abs( mPlayRate )) / mFrameRate);
        mNextFrameTime          = mNextFrameTime + secondsPerFrame;
        if ( mNextFrameTime > currentSeconds )
        {
            int ms = (mNextFrameTime - currentSeconds) * 1000.0;
            mInterruptFrameRateSleepCv.wait_for( lock,
                                                 chrono::milliseconds( ms ),
                                                 [&]{ return mInterruptTriggeredFoRealz; } );
        }
        mInterruptTriggeredFoRealz = false;
    }
}

/*******************************************************************************
 * Position control
 */

void
Movie::nextFramePosition()
{
    using namespace ci::fs;

    if ( !mCurrentFrameIsFresh )
        mCurrentFrameIdx += mPlayRate == 0.0 ? 0 : (mPlayRate > 0 ? 1 : -1);

    mCurrentFrameIsFresh = false;

    if ( mCurrentFrameIdx == mNumFrames )
    {
        if ( mLoopEnabled ) mCurrentFrameIdx = 0;
        else mCurrentFrameIdx = mNumFrames - 1;
    }
    else if ( mCurrentFrameIdx == (size_t)-1 )
    {
        if ( mLoopEnabled ) mCurrentFrameIdx = mNumFrames -1;
        else mCurrentFrameIdx = 0;
    }
}
