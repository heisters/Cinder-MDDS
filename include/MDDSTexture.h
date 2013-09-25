#pragma once

#include "cinder/gl/Texture.h"

namespace mdds {

class Texture : public ci::gl::Texture
{
public:
    class Error : public std::runtime_error
    {
    public:
        Error ( const std::string &what ) : std::runtime_error( what ) {}
    };

    static void warn( const std::string &warning ) { std::cout << warning << std::endl; }


    static Texture loadDds( ci::IStreamRef ddsStream, Format format );

    //! Default initializer. Points to a null Obj
	Texture() : ci::gl::Texture() {}

    //! Constructs a Texture based on an externally initialized OpenGL texture. \a aDoNotDispose specifies whether the Texture is responsible for disposing of the associated OpenGL resource.
	Texture( GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose ) :
    ci::gl::Texture( aTarget, aTextureID, aWidth, aHeight, aDoNotDispose )
    {}
};

}