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
    static void                     warn( const std::string &warning );


    // Construction/Destruction ------------------------------------------------
public:
    static MovieRef create( const ci::fs::path &directory, const std::string &extension=".DDS", const double fps=29.97 )
    { return (MovieRef)(new Movie( directory, extension, fps )); }

    Movie( const ci::fs::path &directory, const std::string &extension=".DDS", const double fps=29.97 );

    ~Movie();

    // Lifecycle ---------------------------------------------------------------
public:
    void                            update();
    void                            draw();


    // Play control ------------------------------------------------------------
public:
    void                            setFramerate( const double fps );
    double                          getFramerate() const;
    double                          getAverageFps() const;
protected:
    void                            updateAverageFps();
    double                          mAverageFps, mFpsLastSampleTime;
    uint32_t                        mFpsFrameCount, mFpsLastFrameCount;
    std::atomic< double >           mFramerate, mNextFrameTime;


    // Async -------------------------------------------------------------------
protected:
    std::atomic< bool >             mThreadIsRunning, mDataIsFresh;
    std::thread                     mThread;
    std::mutex                      mMutex;
    void                            updateFrameThreadFn();

    struct thread_data {
        thread_data() :
        buffer( nullptr ), extension("")
        {}

        std::string                 extension;
        ci::fs::path                directoryPath;
        ci::fs::directory_iterator  directoryIt;
        ci::DataSourceBufferRef     buffer;
    };
    thread_data                     mThreadData;


    // Texture -----------------------------------------------------------------
protected:
    ::mdds::Texture                 mTexture;
public:
    const ci::gl::Texture           getTexture() const { return mTexture; }


    // Position control --------------------------------------------------------
protected:
    std::atomic< bool >             mLoopEnabled;
    void                            incFramePosition();
    void                            resetFramePosition();
    
};
}
