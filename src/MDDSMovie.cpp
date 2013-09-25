#include "MDDSMovie.h"

using namespace std;
using namespace ci;
using namespace mdds;

Movie::Movie( const fs::path &directory, const std::string &extension, const float fps ) :
mThreadIsRunning( false ),
mDataIsFresh( false ),
mLoopEnabled( true )
{
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

void
Movie::warn( const string &warning )
{
    cout << warning << endl;
}

void
Movie::update()
{
    if ( mDataIsFresh )
    {
        mTexture = ::mdds::Texture::loadDds( mThreadData.buffer->createStream(), ::mdds::Texture::Format() );

        if ( mTexture == nullptr ) warn( "error creating texture" );
        	
        mDataIsFresh = false;
    }
}

void
Movie::draw()
{
    if ( mTexture ) gl::draw( mTexture );
}


void
Movie::updateFrameThreadFn()
{
    using namespace ci::fs;

    while ( mThreadIsRunning )
    {
        if ( mDataIsFresh ) continue; // spinlock
        if ( mThreadData.directoryIt->path().extension() != mThreadData.extension )
        {
            nextFrame();
            continue;
        }

        // Read data into memory buffer
        auto ds_path = DataSourcePath::create( mThreadData.directoryIt->path() );
        mThreadData.buffer = DataSourceBuffer::create( ds_path->getBuffer() );
        mDataIsFresh = true;

        nextFrame();
    }
}

void
Movie::nextFrame()
{
    ++mThreadData.directoryIt;
    if ( mThreadData.directoryIt == ci::fs::directory_iterator() && mLoopEnabled ) resetFramePosition();
}

void
Movie::resetFramePosition()
{
    mThreadData.directoryIt = ci::fs::directory_iterator( mThreadData.directoryPath );
}