#include "MDDSTexture.h"

using namespace std;

// Fixes from @gaborpapp, with modifications
// https://github.com/gaborpapp/Cinder/blob/loadDds-mipmap-fix/src/cinder/gl/Texture.cpp#L561

//computes the number of bytes per texture image stored in the dds (taking into account mipmaps and compression)
static
unsigned int
getMipMappedSize(int bytes_per_pixel, int bytes_per_block, unsigned int w, unsigned int h, unsigned int mip_level)
{
	unsigned int size = 0;
	for(unsigned int i=0; i<mip_level; i++)
	{
		w >>=1;
		h >>=1;
		if (w == 0) w = 1;
		if (h == 0) h = 1;
	}
	if ( bytes_per_pixel == 0 ) //compressed
	{
		size += ((w + 3) >> 2) * ((h + 3) >> 2);
		size *= bytes_per_block;
	}
	else
	{
		size += w * h;
		size *= bytes_per_pixel;
	}
	return size;
}


mdds::Texture
mdds::Texture::loadDds( ci::IStreamRef ddsStream, mdds::Texture::Format format )
{
	typedef struct { // DDCOLORKEY
		uint32_t dw1;
		uint32_t dw2;
	} ddColorKey;
	typedef struct  { // DDSCAPS2
		uint32_t dwCaps1;
		uint32_t dwCaps2;
		uint32_t Reserved[2];
	} ddCaps2;
	typedef struct _DDPIXELFORMAT { // DDPIXELFORMAT
		uint32_t  dwSize;
		uint32_t  dwFlags;
		uint32_t  dwFourCC;
		union {
			uint32_t  dwRGBBitCount;
			uint32_t  dwYUVBitCount;
			uint32_t  dwZBufferBitDepth;
			uint32_t  dwAlphaBitDepth;
			uint32_t  dwLuminanceBitCount;
			uint32_t  dwBumpBitCount;
			uint32_t  dwPrivateFormatBitCount;
		} ;
		union {
			uint32_t  dwRBitMask;
			uint32_t  dwYBitMask;
			uint32_t  dwStencilBitDepth;
			uint32_t  dwLuminanceBitMask;
			uint32_t  dwBumpDuBitMask;
			uint32_t  dwOperations;
		} ;
		union {
			uint32_t  dwGBitMask;
			uint32_t  dwUBitMask;
			uint32_t  dwZBitMask;
			uint32_t  dwBumpDvBitMask;
			struct {
				int16_t wFlipMSTypes; //bug fix here, dds header has short here, no words
				int16_t wBltMSTypes;
			} MultiSampleCaps;
		};
		union {
			uint32_t  dwBBitMask;
			uint32_t  dwVBitMask;
			uint32_t  dwStencilBitMask;
			uint32_t  dwBumpLuminanceBitMask;
		};
		union {
			uint32_t  dwRGBAlphaBitMask;
			uint32_t  dwYUVAlphaBitMask;
			uint32_t  dwLuminanceAlphaBitMask;
			uint32_t  dwRGBZBitMask;
			uint32_t  dwYUVZBitMask;
		} ;
	} ddPixelFormat;
	typedef struct ddSurface // this is lifted and adapted from DDSURFACEDESC2
	{
		uint32_t               dwSize;                 // size of the DDSURFACEDESC structure
		uint32_t               dwFlags;                // determines what fields are valid
		uint32_t               dwHeight;               // height of surface to be created
		uint32_t               dwWidth;                // width of input surface
		union
		{
			int32_t            lPitch;                 // distance to start of next line (return value only)
			uint32_t           dwLinearSize;           // Formless late-allocated optimized surface size
		};
		union
		{
			uint32_t           dwBackBufferCount;      // number of back buffers requested
			uint32_t           dwDepth;                // the depth if this is a volume texture
		};
		union
		{
			uint32_t            dwMipMapCount;          // number of mip-map levels requestde
			// dwZBufferBitDepth removed, use ddpfPixelFormat one instead
			uint32_t            dwRefreshRate;          // refresh rate (used when display mode is described)
			uint32_t            dwSrcVBHandle;          // The source used in VB::Optimize
		};
		uint32_t                dwAlphaBitDepth;        // depth of alpha buffer requested
		uint32_t                dwReserved;             // reserved
		uint32_t                lpSurface;              // pointer to the associated surface memory
		union
		{
			ddColorKey            ddckCKDestOverlay;      // color key for destination overlay use
			uint32_t            dwEmptyFaceColor;       // Physical color for empty cubemap faces
		};
		ddColorKey          ddckCKDestBlt;          // color key for destination blt use
		ddColorKey          ddckCKSrcOverlay;       // color key for source overlay use
		ddColorKey          ddckCKSrcBlt;           // color key for source blt use
		union
		{
			ddPixelFormat        ddpfPixelFormat;        // pixel format description of the surface
			uint32_t            dwFVF;                  // vertex format description of vertex buffers
		};
		ddCaps2            ddsCaps;                // direct draw surface capabilities
		uint32_t        dwTextureStage;         // stage in multitexture cascade
	} ddSurface;

	enum { FOURCC_DXT1 = 0x31545844, FOURCC_DXT3 = 0x33545844, FOURCC_DXT5 = 0x35545844 };

    ddSurface ddsd;
    char filecode[4];
    ddsStream->readData( filecode, 4 );
    if( strncmp( filecode, "DDS ", 4 ) != 0 ) {
        throw Error( "file does not appear to be a DDS texture: " + string( filecode ) );
    }
    ddsStream->readData( &ddsd, 124/*sizeof(ddsd)*/ );
    uint32_t width = ddsd.dwWidth;
    uint32_t height = ddsd.dwHeight;
    //int numComponents  = (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1) ? 3 : 4;
    uint32_t numMipMaps = ddsd.dwMipMapCount;
    if (numMipMaps == 0) numMipMaps = 1;
    int dataFormat;
    bool is_compressed = true;
    bool is_cubemap = (ddsd.ddsCaps.dwCaps2 & 0x00000200L)!=0;
    uint32_t bytes_per_pixel = 0; //only if not compressed
    switch( ddsd.ddpfPixelFormat.dwFourCC ) {
        case FOURCC_DXT1:
            dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;
        case FOURCC_DXT3:
            dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case FOURCC_DXT5:
            dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        default:
            is_compressed = false;
            bytes_per_pixel = ddsd.ddpfPixelFormat.dwRGBBitCount / 8;
            switch (ddsd.ddpfPixelFormat.dwRGBBitCount) //warning: not all supported
        {
            case 1: dataFormat = GL_LUMINANCE; break;
            case 3: dataFormat = GL_RGB; break;
            case 4: dataFormat = GL_RGBA; break;
            default: throw Error( "could not determine pixel format" );
        }
            break;
    }
    uint32_t blockSize = 0;
    if (is_compressed)
        blockSize = (dataFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    int numLayers = 1;
    if (is_cubemap)
    {
        format.setTarget( GL_TEXTURE_CUBE_MAP );
        numLayers = 6;
    }
    // how big is it going to be including all mipmaps?
    //uint32_t bufSize = ( ddsd.dwMipMapCount > 1 ) ? ( ddsd.dwLinearSize * 2 ) : ( ddsd.dwLinearSize ); //this only works in 2D
    uint32_t bufSize = 0;
    for (int iMipmap = 0; iMipmap < numMipMaps; iMipmap++)
        bufSize += getMipMappedSize(bytes_per_pixel, blockSize, width, height, iMipmap);
    bufSize *= numLayers; //cubemaps have 6 textures
    shared_ptr<uint8_t> pixels( new uint8_t[bufSize+1], checked_array_deleter<uint8_t>() );
    ddsStream->readData( pixels.get(), bufSize );
    //*****************************************************
    off_t offset = 0;
    // Create the texture
    GLenum target = format.getTarget();
    GLuint texID;
    glGenTextures( 1, &texID );

    Texture result( target, texID, width, height, false );
    result.mObj->mWidth = width;
    result.mObj->mHeight = height;
    result.mObj->mInternalFormat = dataFormat;
    glBindTexture( result.mObj->mTarget, result.mObj->mTextureID );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    // load the mipmaps
    for( int i = 0; i < numMipMaps && (width || height); ++i ) {
        if( width == 0 )
            width = 1;
        if( height == 0 )
            height = 1;
        int size = 0;
        for (int iLayer = 0; iLayer < numLayers; iLayer++)
        {
            size = getMipMappedSize(bytes_per_pixel, blockSize, result.mObj->mWidth, result.mObj->mHeight, i);
            if (is_compressed)
            {
                if (target == GL_TEXTURE_CUBE_MAP)
                    glCompressedTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + iLayer, i, dataFormat, width, height, 0, size, pixels.get() + offset );
                else
                {
                    glCompressedTexImage2D( result.mObj->mTarget , i, dataFormat, width, height, 0, size, pixels.get() + offset );
                }
            }
            else
            {
                if (target == GL_TEXTURE_CUBE_MAP)
                    glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + iLayer, i, dataFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, pixels.get() + offset );
                else
                    glTexImage2D( result.mObj->mTarget , i, dataFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, pixels.get() + offset );
            }
            offset += size;
        }
        width  >>= 1;
        height >>= 1;
    }
    if( numMipMaps > 1 ) {
        glTexParameteri( result.mObj->mTarget, GL_TEXTURE_MAX_LEVEL, numMipMaps - 1 );
        glTexParameteri( result.mObj->mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glTexParameteri( result.mObj->mTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    }
    else {
        glTexParameteri( result.mObj->mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( result.mObj->mTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    }
    if (is_cubemap)
    {
        glTexParameteri( result.mObj->mTarget, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
        glTexParameteri( result.mObj->mTarget, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
    }
    
    return result;
}
