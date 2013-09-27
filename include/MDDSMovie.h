#pragma once

#include "cinder/gl/gl.h"
#include "cinder/Thread.h"
#include "cinder/DataSource.h"

#include "MDDSTexture.h"

#include <atomic>

namespace mdds {

class Movie;
typedef std::shared_ptr< Movie > MovieRef;

class Movie
{
    // Exception handling ------------------------------------------------------
public:
    class Error : public std::runtime_error
    {
    public:
        Error( const std::string &what ) : std::runtime_error( what ) {}
    };

    class LoadError : public Error
    {
    public:
        LoadError( const std::string &what ) : Error( what ) {}
    };
protected:
    static void                     warn( const std::string &warning );


    // Construction/Destruction ------------------------------------------------
public:
    //! Factory method to create a reference to a movie that plays all frames in
    // \a directory.
    static MovieRef create( const ci::fs::path &directory, const std::string &extension=".DDS", const double fps=29.97 )
    { return (MovieRef)(new Movie( directory, extension, fps )); }

    //! Construct a movie that plays all frames in \a directory. Files in the
    // directory that don't have a file extension that matches (case-sensitively
    // \a extension will be skipped. \a fps sets the framerate of the movie.
    Movie( const ci::fs::path &directory, const std::string &extension=".DDS", const double fps=29.97 );

    ~Movie();

    // Lifecycle ---------------------------------------------------------------
public:
    //! Call in your app's update() method
    void                            update();
    //! Draws the movie texture using default arugments. For better control over
    // drawing, call getTexture().
    void                            draw();


    // Play control ------------------------------------------------------------
public:

    //! Returns the "native" frame rate passed to the constructor.
    double                          getFrameRate() const;

    //! Returns the actual rate at which frames are being read.
    double                          getAverageFps() const;

    //! Set the rate at which frames are played back. 1 is normal, -1 is
    // backwards, 0 is paused.
    void                            setPlayRate( const double newRate );

    //! Get the rate at which frames are played back, relative to the frame
    // rate.
    double                          getPlayRate() const;

    //! Jump to \a seconds position in the movie
    void                            seekToTime( const double seconds );

    //! Jump to \a frame position in the movie
    void                            seekToFrame( const size_t frame );

    //! Jump to the beginning of the movie
    void                            seekToStart();

    //! Jump to the end of the movie
    void                            seekToEnd();

    //! Get the current position in frames
    size_t                          getCurrentFrame() const;

    //! Get the total number of frames in the movie
    size_t                          getNumFrames() const;

    //! Get the current position in seconds
    double                          getCurrentTime() const;

    //! Get the total number of seconds in the movie
    double                          getDuration() const;
protected:
    void                            updateAverageFps();
    double                          mAverageFps, mFpsLastSampleTime;
    uint32_t                        mFpsFrameCount, mFpsLastFrameCount;
    std::atomic< double >           mFrameRate, mNextFrameTime, mPlayRate;
    void                            readFramePaths();
    std::atomic< size_t >           mCurrentFrameIdx, mNumFrames;
    std::atomic< bool >             mCurrentFrameIsFresh;


    // Async -------------------------------------------------------------------
protected:
    std::atomic< bool >             mThreadIsRunning, mDataIsFresh;
    std::thread                     mThread;
    std::mutex                      mMutex;
    bool                            mInterruptTriggeredFoRealz;
    std::condition_variable         mInterruptFrameRateSleepCv;
    void                            updateFrameThreadFn();

    struct thread_data {
        thread_data() :
        buffer( nullptr ), extension("")
        {}

        std::string                 extension;
        ci::fs::path                directoryPath;
        ci::DataSourceBufferRef     buffer;
        std::vector< ci::fs::path > framePaths;
    };
    thread_data                     mThreadData;


    // Texture -----------------------------------------------------------------
protected:
    ::mdds::Texture                 mTexture;
public:
    //! Returns a reference to the current frame's texture
    const ci::gl::Texture &         getTexture() const { return mTexture; }


    // Position control --------------------------------------------------------
protected:
    std::atomic< bool >             mLoopEnabled;
    void                            nextFramePosition();
    
};
}
