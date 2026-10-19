#if defined(TILES)

#include "r3d_gpu.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

#include "debug.h"
#include "path_info.h"

namespace r3d
{

namespace
{

auto read_file_bytes( const std::string &path ) -> std::vector<unsigned char>
{
    auto in = std::ifstream( path, std::ios::binary );
    if( !in.is_open() ) {
        return {};
    }
    return { std::istreambuf_iterator<char>( in ), std::istreambuf_iterator<char>() };
}

struct shader_format {
    SDL_GPUShaderFormat flag;
    const char *suffix;
};

// BN builds backend artifacts from src/shaders through its native shadercross pipeline.
constexpr shader_format shader_formats[] = {
    { .flag = SDL_GPU_SHADERFORMAT_SPIRV, .suffix = ".spv" },
    { .flag = SDL_GPU_SHADERFORMAT_DXIL, .suffix = ".dxil" },
    { .flag = SDL_GPU_SHADERFORMAT_MSL, .suffix = ".msl" },
};

} // namespace

gpu::~gpu()
{
    shutdown();
}

auto gpu::load_shader( const load_shader_options &options ) -> SDL_GPUShader *
{
    const auto basename = options.basename;
    auto stage = options.stage;
    auto samplers = options.samplers;
    auto uniform_buffers = options.uniform_buffers;

    const auto available = SDL_GPUShaderFormat( SDL_GetGPUShaderFormats( device_ ) );
    const shader_format *chosen = nullptr;
    for( const shader_format &f : shader_formats ) {
        if( ( available & f.flag ) != 0 ) {
            chosen = &f;
            break;
        }
    }
    if( !chosen ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: no shader format of the GPU device is shipped";
        return nullptr;
    }
    const auto path = std::string( PATH_INFO::datadir() + "shaders/" + basename + chosen->suffix );
    const auto bytes = std::vector<unsigned char>( read_file_bytes( path ) );
    if( bytes.empty() ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: cannot read shader " << path;
        return nullptr;
    }
    auto info = SDL_GPUShaderCreateInfo{};
    info.code_size = bytes.size();
    info.code = bytes.data();
    info.entrypoint = chosen->flag == SDL_GPU_SHADERFORMAT_MSL ? "main0" : "main";
    info.format = chosen->flag;
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniform_buffers;
    SDL_GPUShader *shader = SDL_CreateGPUShader( device_, &info );
    if( !shader ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: SDL_CreateGPUShader(" << path << ") failed: "
                                        << SDL_GetError();
    }
    return shader;
}

auto gpu::init( SDL_Renderer *renderer ) -> bool
{
    shutdown();
    renderer_ = renderer;
    device_ = renderer ? SDL_GetGPURendererDevice( renderer ) : nullptr;
    if( !device_ ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: SDL gpu renderer required; active renderer: "
                                        << ( renderer ? SDL_GetRendererName( renderer ) : "none" );
        renderer_ = nullptr;
        return false;
    }

    // Prefer a 32-bit float depth buffer, fall back to what the device supports.
    for( SDL_GPUTextureFormat fmt : {
             SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREFORMAT_D24_UNORM,
             SDL_GPU_TEXTUREFORMAT_D16_UNORM
         } ) {
        if( SDL_GPUTextureSupportsFormat( device_, fmt, SDL_GPU_TEXTURETYPE_2D,
                                          SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET ) ) {
            depth_format_ = fmt;
            break;
        }
    }
    if( depth_format_ == SDL_GPU_TEXTUREFORMAT_INVALID ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: no usable depth format";
        shutdown();
        return false;
    }

    SDL_GPUShader *vs = load_shader( { .basename = "r3d_block_vertex", .stage = SDL_GPU_SHADERSTAGE_VERTEX, .samplers = 0, .uniform_buffers = 1 } );
    SDL_GPUShader *fs = load_shader( { .basename = "r3d_block_fragment", .stage = SDL_GPU_SHADERSTAGE_FRAGMENT, .samplers = 1, .uniform_buffers = 0 } );
    SDL_GPUShader *silhouette_fs = load_shader( { .basename = "r3d_silhouette_fragment", .stage = SDL_GPU_SHADERSTAGE_FRAGMENT, .samplers = 1, .uniform_buffers = 0 } );
    if( !vs || !fs || !silhouette_fs ) {
        if( vs ) {
            SDL_ReleaseGPUShader( device_, vs );
        }
        if( fs ) {
            SDL_ReleaseGPUShader( device_, fs );
        }
        if( silhouette_fs ) {
            SDL_ReleaseGPUShader( device_, silhouette_fs );
        }
        shutdown();
        return false;
    }

    SDL_GPUVertexBufferDescription buffers[2] {};
    buffers[0].slot = 0;
    buffers[0].pitch = sizeof( vertex );
    buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    buffers[1].slot = 1;
    buffers[1].pitch = sizeof( uint32_t );
    buffers[1].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    SDL_GPUVertexAttribute attributes[3] {};
    attributes[0].location = 0;
    attributes[0].buffer_slot = 0;
    attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attributes[0].offset = 0;
    attributes[1].location = 1;
    attributes[1].buffer_slot = 0;
    attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attributes[1].offset = 3 * sizeof( float );
    attributes[2].location = 2;
    attributes[2].buffer_slot = 1;
    attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    attributes[2].offset = 0;

    auto color_target = SDL_GPUColorTargetDescription{};
    color_target.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

    auto pci = SDL_GPUGraphicsPipelineCreateInfo{};
    pci.vertex_shader = vs;
    pci.fragment_shader = fs;
    pci.vertex_input_state.vertex_buffer_descriptions = buffers;
    pci.vertex_input_state.num_vertex_buffers = 2;
    pci.vertex_input_state.vertex_attributes = attributes;
    pci.vertex_input_state.num_vertex_attributes = 3;
    pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    // Block faces are wound clockwise seen from outside (r3d_mesh.cpp), so their backs can go:
    // the cutaway then shows the floor and wall ends rather than the insides of what it cut.
    pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pci.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
    // Devices created for broad compatibility disable depth clamping; clipping is required.
    pci.rasterizer_state.enable_depth_clip = true;
    pci.depth_stencil_state.enable_depth_test = true;
    pci.depth_stencil_state.enable_depth_write = true;
    pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    pci.target_info.color_target_descriptions = &color_target;
    pci.target_info.num_color_targets = 1;
    pci.target_info.depth_stencil_format = depth_format_;
    pci.target_info.has_depth_stencil_target = true;
    block_pipeline_ = SDL_CreateGPUGraphicsPipeline( device_, &pci );
    // Billboards and flat sprites are seen from either side.
    pci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    entity_pipeline_ = SDL_CreateGPUGraphicsPipeline( device_, &pci );
    pci.fragment_shader = silhouette_fs;
    pci.depth_stencil_state.enable_depth_write = false;
    pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_GREATER;
    color_target.blend_state.enable_blend = true;
    color_target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color_target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color_target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    avatar_silhouette_pipeline_ = SDL_CreateGPUGraphicsPipeline( device_, &pci );
    SDL_ReleaseGPUShader( device_, vs );
    SDL_ReleaseGPUShader( device_, fs );
    SDL_ReleaseGPUShader( device_, silhouette_fs );
    if( !block_pipeline_ || !entity_pipeline_ || !avatar_silhouette_pipeline_ ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: SDL_CreateGPUGraphicsPipeline failed: " << SDL_GetError();
        shutdown();
        return false;
    }

    auto sci = SDL_GPUSamplerCreateInfo{};
    sci.min_filter = SDL_GPU_FILTER_NEAREST;
    sci.mag_filter = SDL_GPU_FILTER_NEAREST;
    sci.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_ = SDL_CreateGPUSampler( device_, &sci );
    if( !sampler_ ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: SDL_CreateGPUSampler failed: " << SDL_GetError();
        shutdown();
        return false;
    }
    return true;
}

auto gpu::release_staging() -> void
{
    if( staging_ && device_ ) {
        SDL_ReleaseGPUTransferBuffer( device_, staging_ );
    }
    staging_ = nullptr;
    staging_size_ = 0;
}

auto gpu::release_target() -> void
{
    if( wrapped_ ) {
        SDL_DestroyTexture( wrapped_ );
        wrapped_ = nullptr;
    }
    if( device_ ) {
        if( color_ ) {
            SDL_ReleaseGPUTexture( device_, color_ );
        }
        if( depth_ ) {
            SDL_ReleaseGPUTexture( device_, depth_ );
        }
    }
    color_ = nullptr;
    depth_ = nullptr;
    target_w_ = 0;
    target_h_ = 0;
}

auto gpu::abort_frame() -> void
{
    // A command buffer cannot be dropped, so finish whatever is open and submit it empty.
    if( pass_ ) {
        SDL_EndGPURenderPass( pass_ );
        pass_ = nullptr;
    }
    if( copy_pass_ ) {
        SDL_EndGPUCopyPass( copy_pass_ );
        copy_pass_ = nullptr;
    }
    if( cmd_ ) {
        SDL_SubmitGPUCommandBuffer( cmd_ );
        cmd_ = nullptr;
    }
    pending_.clear();
    pending_data_.clear();
}

auto gpu::shutdown() -> void
{
    if( device_ ) {
        abort_frame();
        // The renderer may still sample the wrapped target; make sure no GPU work references it.
        SDL_WaitForGPUIdle( device_ );
        release_target();
        release_staging();
        if( block_pipeline_ ) {
            SDL_ReleaseGPUGraphicsPipeline( device_, block_pipeline_ );
        }
        if( entity_pipeline_ ) {
            SDL_ReleaseGPUGraphicsPipeline( device_, entity_pipeline_ );
        }
        if( avatar_silhouette_pipeline_ ) {
            SDL_ReleaseGPUGraphicsPipeline( device_, avatar_silhouette_pipeline_ );
        }
        if( sampler_ ) {
            SDL_ReleaseGPUSampler( device_, sampler_ );
        }
    }
    block_pipeline_ = nullptr;
    entity_pipeline_ = nullptr;
    avatar_silhouette_pipeline_ = nullptr;
    sampler_ = nullptr;
    device_ = nullptr;
    renderer_ = nullptr;
    depth_format_ = SDL_GPU_TEXTUREFORMAT_INVALID;
}

auto gpu::create_sheet( SDL_Surface *rgba ) -> std::shared_ptr<SDL_Texture>
{
    if( !device_ || !rgba || rgba->w <= 0 || rgba->h <= 0 ) {
        return nullptr;
    }
    SDL_Surface *const pixels = SDL_ConvertSurface( rgba, SDL_PIXELFORMAT_RGBA32 );
    if( !pixels ) {
        return nullptr;
    }
    const auto w = static_cast<Uint32>( pixels->w );
    const auto h = static_cast<Uint32>( pixels->h );
    auto tci = SDL_GPUTextureCreateInfo{};
    tci.type = SDL_GPU_TEXTURETYPE_2D;
    tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width = w;
    tci.height = h;
    tci.layer_count_or_depth = 1;
    tci.num_levels = 1;
    SDL_GPUTexture *const texture = SDL_CreateGPUTexture( device_, &tci );

    auto tbi = SDL_GPUTransferBufferCreateInfo{};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size = w * h * 4;
    SDL_GPUTransferBuffer *const staging = texture ? SDL_CreateGPUTransferBuffer( device_,
                                           &tbi ) : nullptr;
    void *const mapped = staging ? SDL_MapGPUTransferBuffer( device_, staging, false ) : nullptr;
    SDL_GPUCommandBuffer *upload_cmd = nullptr;
    auto ok = static_cast<bool>( mapped != nullptr );
    if( ok ) {
        for( auto row = Uint32( 0 ); row < h; ++row ) {
            std::memcpy( static_cast<uint8_t *>( mapped ) + row * w * 4,
                         static_cast<const uint8_t *>( pixels->pixels ) + row * pixels->pitch, w * 4 );
        }
        SDL_UnmapGPUTransferBuffer( device_, staging );
        upload_cmd = SDL_AcquireGPUCommandBuffer( device_ );
        ok = upload_cmd != nullptr;
    }
    if( ok ) {
        SDL_GPUCopyPass *const copy = SDL_BeginGPUCopyPass( upload_cmd );
        auto src = SDL_GPUTextureTransferInfo{};
        src.transfer_buffer = staging;
        src.pixels_per_row = w;
        src.rows_per_layer = h;
        auto dst = SDL_GPUTextureRegion{};
        dst.texture = texture;
        dst.w = w;
        dst.h = h;
        dst.d = 1;
        SDL_UploadToGPUTexture( copy, &src, &dst, false );
        SDL_EndGPUCopyPass( copy );
        // Submitted now, so it runs before any later frame of ours samples the sheet.
        ok = SDL_SubmitGPUCommandBuffer( upload_cmd );
    }
    if( staging ) {
        SDL_ReleaseGPUTransferBuffer( device_, staging );
    }
    SDL_DestroySurface( pixels );

    SDL_Texture *wrapped = nullptr;
    if( ok ) {
        const auto props = SDL_PropertiesID( SDL_CreateProperties() );
        SDL_SetPointerProperty( props, SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_POINTER, texture );
        SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_RGBA32 );
        SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STATIC );
        SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, w );
        SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, h );
        wrapped = SDL_CreateTextureWithProperties( renderer_, props );
        SDL_DestroyProperties( props );
    }
    if( !wrapped ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: creating a sprite sheet failed: " << SDL_GetError();
        if( texture ) {
            SDL_ReleaseGPUTexture( device_, texture );
        }
        return nullptr;
    }
    SDL_GPUDevice *const device = device_;
    return std::shared_ptr<SDL_Texture>( wrapped, [device, texture]( SDL_Texture * t ) -> void {
        // The wrapper does not own the GPU texture.
        SDL_DestroyTexture( t );
        SDL_ReleaseGPUTexture( device, texture );
    } );
}

auto gpu::ensure_target( int width, int height ) -> bool
{
    if( !device_ || width <= 0 || height <= 0 ) {
        return false;
    }
    if( color_ && target_w_ == width && target_h_ == height ) {
        return true;
    }
    if( color_ ) {
        SDL_WaitForGPUIdle( device_ );
    }
    release_target();

    auto tci = SDL_GPUTextureCreateInfo{};
    tci.type = SDL_GPU_TEXTURETYPE_2D;
    tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width = static_cast<Uint32>( width );
    tci.height = static_cast<Uint32>( height );
    tci.layer_count_or_depth = 1;
    tci.num_levels = 1;
    color_ = SDL_CreateGPUTexture( device_, &tci );
    tci.format = depth_format_;
    tci.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depth_ = SDL_CreateGPUTexture( device_, &tci );
    if( !color_ || !depth_ ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: creating the " << width << "x" << height
                                        << " target failed: " << SDL_GetError();
        release_target();
        return false;
    }

    const auto props = SDL_PropertiesID( SDL_CreateProperties() );
    SDL_SetPointerProperty( props, SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_POINTER, color_ );
    SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_RGBA32 );
    SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_TARGET );
    SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, width );
    SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, height );
    wrapped_ = SDL_CreateTextureWithProperties( renderer_, props );
    SDL_DestroyProperties( props );
    if( !wrapped_ ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: wrapping the target as SDL_Texture failed: "
                                        << SDL_GetError();
        release_target();
        return false;
    }
    SDL_SetTextureBlendMode( wrapped_, SDL_BLENDMODE_NONE );
    // Linear, so a supersampled target (see r3d::draw) is averaged down instead of aliasing.
    SDL_SetTextureScaleMode( wrapped_, SDL_SCALEMODE_LINEAR );
    target_w_ = width;
    target_h_ = height;
    return true;
}

auto gpu::create_buffer( SDL_GPUBufferUsageFlags usage, uint32_t size ) -> SDL_GPUBuffer *
{
    auto info = SDL_GPUBufferCreateInfo{};
    info.usage = usage;
    info.size = size;
    return SDL_CreateGPUBuffer( device_, &info );
}

auto gpu::release_buffer( SDL_GPUBuffer *buffer ) -> void
{
    if( buffer && device_ ) {
        SDL_ReleaseGPUBuffer( device_, buffer );
    }
}

auto gpu::begin_frame() -> bool
{
    if( !ready() || !color_ || cmd_ ) {
        return false;
    }
    if( !SDL_FlushRenderer( renderer_ ) ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: SDL_FlushRenderer failed: " << SDL_GetError();
        return false;
    }
    cmd_ = SDL_AcquireGPUCommandBuffer( device_ );
    return cmd_ != nullptr;
}

auto gpu::upload( const upload_options &options ) -> bool
{
    auto dst = options.dst;
    const auto data = options.data;
    auto size = options.size;
    auto cycle = options.cycle;

    if( !cmd_ || pass_ || !dst || size == 0 ) {
        return false;
    }
    const auto offset = size_t( pending_data_.size() );
    const auto *bytes = static_cast<const uint8_t *>( data );
    pending_data_.insert( pending_data_.end(), bytes, bytes + size );
    pending_.push_back( { .dst = dst, .offset = static_cast<uint32_t>( offset ), .size = size, .cycle = cycle } );
    return true;
}

auto gpu::flush_uploads() -> bool
{
    if( pending_.empty() ) {
        return true;
    }
    const auto total = static_cast<uint32_t>( pending_data_.size() );
    auto ok = static_cast<bool>( true );
    if( !staging_ || staging_size_ < total ) {
        release_staging();
        auto tbi = SDL_GPUTransferBufferCreateInfo{};
        tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbi.size = total + total / 2;
        staging_ = SDL_CreateGPUTransferBuffer( device_, &tbi );
        staging_size_ = staging_ ? tbi.size : 0;
        ok = staging_ != nullptr;
    }
    void *mapped = ok ? SDL_MapGPUTransferBuffer( device_, staging_, true ) : nullptr;
    if( mapped ) {
        std::memcpy( mapped, pending_data_.data(), total );
        SDL_UnmapGPUTransferBuffer( device_, staging_ );
        copy_pass_ = SDL_BeginGPUCopyPass( cmd_ );
    }
    if( !mapped || !copy_pass_ ) {
        DebugLog( DL::Error, DC::Main ) << "r3d: staging upload failed: " << SDL_GetError();
        ok = false;
    } else {
        for( const pending_upload &u : pending_ ) {
            auto src = SDL_GPUTransferBufferLocation{};
            src.transfer_buffer = staging_;
            src.offset = u.offset;
            auto region = SDL_GPUBufferRegion{};
            region.buffer = u.dst;
            region.size = u.size;
            SDL_UploadToGPUBuffer( copy_pass_, &src, &region, u.cycle );
        }
        SDL_EndGPUCopyPass( copy_pass_ );
        copy_pass_ = nullptr;
    }
    pending_.clear();
    pending_data_.clear();
    return ok;
}

auto gpu::begin_pass( float r, float g, float b ) -> bool
{
    if( !cmd_ || pass_ || !flush_uploads() ) {
        return false;
    }
    auto cti = SDL_GPUColorTargetInfo{};
    cti.texture = color_;
    cti.clear_color = SDL_FColor{ .r = r, .g = g, .b = b, .a = 1.0f };
    cti.load_op = SDL_GPU_LOADOP_CLEAR;
    cti.store_op = SDL_GPU_STOREOP_STORE;
    auto dti = SDL_GPUDepthStencilTargetInfo{};
    dti.texture = depth_;
    dti.clear_depth = 1.0f;
    dti.load_op = SDL_GPU_LOADOP_CLEAR;
    dti.store_op = SDL_GPU_STOREOP_DONT_CARE;
    dti.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    dti.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    pass_ = SDL_BeginGPURenderPass( cmd_, &cti, 1, &dti );
    return pass_ != nullptr;
}

auto gpu::bind_block_pipeline() -> void
{
    if( pass_ ) {
        SDL_BindGPUGraphicsPipeline( pass_, block_pipeline_ );
    }
}

auto gpu::bind_entity_pipeline() -> void
{
    if( pass_ && entity_pipeline_ ) {
        SDL_BindGPUGraphicsPipeline( pass_, entity_pipeline_ );
    }
}

auto gpu::bind_avatar_silhouette_pipeline() -> void
{
    if( pass_ && avatar_silhouette_pipeline_ ) {
        SDL_BindGPUGraphicsPipeline( pass_, avatar_silhouette_pipeline_ );
    }
}

auto gpu::draw_block( const draw_block_options &options ) -> void
{
    auto positions = options.positions;
    auto lights = options.lights;
    auto indices = options.indices;
    auto first_index = options.first_index;
    auto index_count = options.index_count;
    auto sheet = options.sheet;
    const auto &uniforms = options.uniforms;

    if( !pass_ || !positions || !lights || !indices || index_count == 0 || !sheet ) {
        return;
    }
    const auto props = SDL_PropertiesID( SDL_GetTextureProperties( sheet ) );
    auto *gpu_sheet = static_cast<SDL_GPUTexture *>(
                          SDL_GetPointerProperty( props, SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, nullptr ) );
    if( !gpu_sheet ) {
        return;
    }
    const SDL_GPUBufferBinding vertex_bindings[2] = { { .buffer = positions, .offset = 0 }, { .buffer = lights, .offset = 0 } };
    SDL_BindGPUVertexBuffers( pass_, 0, vertex_bindings, 2 );
    const auto index_binding = SDL_GPUBufferBinding{ .buffer = indices, .offset = 0 };
    SDL_BindGPUIndexBuffer( pass_, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT );
    const auto sampler_binding = SDL_GPUTextureSamplerBinding{ .texture = gpu_sheet, .sampler = sampler_ };
    SDL_BindGPUFragmentSamplers( pass_, 0, &sampler_binding, 1 );
    SDL_PushGPUVertexUniformData( cmd_, 0, &uniforms, sizeof( uniforms ) );
    SDL_DrawGPUIndexedPrimitives( pass_, index_count, 1, first_index, 0, 0 );
}

auto gpu::end_frame() -> SDL_Texture *
{
    if( !cmd_ ) {
        return nullptr;
    }
    if( pass_ ) {
        SDL_EndGPURenderPass( pass_ );
        pass_ = nullptr;
    }
    if( copy_pass_ ) {
        SDL_EndGPUCopyPass( copy_pass_ );
        copy_pass_ = nullptr;
    }
    const auto ok = static_cast<bool>( SDL_SubmitGPUCommandBuffer( cmd_ ) );
    cmd_ = nullptr;
    pending_.clear();
    pending_data_.clear();
    return ok ? wrapped_ : nullptr;
}

} // namespace r3d

#endif // defined(TILES)
