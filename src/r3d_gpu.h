#pragma once
#ifndef CATA_SRC_R3D_GPU_H
#define CATA_SRC_R3D_GPU_H

#if defined(TILES)

#include <cstdint>
#include <memory>
#include <vector>

#include <SDL3/SDL.h>

#include "r3d_math.h"

namespace r3d
{

/// Per-vertex data of stream 0. Light colour lives in stream 1 as one RGBA8 value per vertex.
struct vertex {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
};
static_assert( sizeof( vertex ) == 20, "vertex layout is mirrored by the pipeline description" );

struct block_uniforms {
    mat4 view_proj;
    float offset[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    // xyz: world direction towards the camera; w: depth pull, in those units, of a vertex whose
    // light alpha is 1 (billboards; 0 for everything else). See r3d_block.vert.
    float pull[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    // Cutaway around the player (see r3d_block.frag): xy their world position, z the height
    // below which nothing is cut, w the half-width of the hole in tiles (0: off); cut2 xy the
    // view's forward direction on the ground, z the hole's half-length along it.
    float cut[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float cut2[4] = { 1.0f, 0.0f, 0.0f, 0.0f };

    // Distance fog (weather): rgb its colour, w its density (0: off). fog2 xy is the point in the
    // drawn frame the distance is measured from, on the ground (see fog_amount in r3d_hud_math.h);
    // fog2 z is 1 to draw translucent texels dithered instead of leaving them out (particles).
    float fog[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float fog2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    /// Draw unseen terrain as solid black in first person, rather than revealing beyond it.
    float flags[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
static_assert( sizeof( block_uniforms ) == 176, "block_uniforms is mirrored by r3d_block.vert" );

/// Owns the GPU objects of the 3D renderer: pipelines, sampler, and the offscreen colour+depth
/// target that gets composited into the SDL_Renderer scene. It shares the SDL_Renderer's own
/// SDL_GPUDevice (there is no second device), so every method must be called from the main
/// thread, and begin_frame() flushes the renderer's pending 2D work first.
class gpu
{
    public:
        gpu() = default;
        gpu( const gpu & ) = delete;
        auto operator=( const gpu & ) -> gpu & = delete;
        ~gpu();

        /// Creates shaders, pipelines and the sampler. Fails (false) without a GPU renderer.
        auto init( SDL_Renderer *renderer ) -> bool;
        /// Releases everything created by init and the target. Safe to call repeatedly.
        auto shutdown() -> void;
        auto ready() const -> bool {
            return device_ != nullptr && block_pipeline_ != nullptr;
        }
        auto renderer() const -> SDL_Renderer * {
            return renderer_;
        }

        /// (Re)creates the offscreen target when the size changes.
        auto ensure_target( int width, int height ) -> bool;
        auto target_width() const -> int {
            return target_w_;
        }
        auto target_height() const -> int {
            return target_h_;
        }

        /// A sprite sheet the 3D pass can sample, from an RGBA32 surface, uploaded on its own
        /// command buffer (the renderer only submits its uploads when it presents). Wrapped as
        /// an SDL_Texture of the renderer like the tileset sheets; empty on failure. The GPU
        /// texture is released with it, so it must go before shutdown().
        auto create_sheet( SDL_Surface *rgba ) -> std::shared_ptr<SDL_Texture>;
        auto create_buffer( SDL_GPUBufferUsageFlags usage,
                            uint32_t size ) -> SDL_GPUBuffer *; // *NOPAD*
        auto release_buffer( SDL_GPUBuffer *buffer ) -> void;

        /// Starts a frame: flushes the renderer and acquires a command buffer.
        auto begin_frame() -> bool;
        /// Queues a CPU->GPU copy; only valid between begin_frame and begin_pass. The data is
        /// copied right away; all of a frame's uploads go through one staging buffer at begin_pass.
        struct upload_options {
            SDL_GPUBuffer *dst;
            const void *data;
            uint32_t size;
            bool cycle = false;
        };

        auto upload( const upload_options &options ) -> bool;
        /// Performs the queued uploads and opens the render pass (clearing colour and depth).
        /// False means the frame is unusable and buffers uploaded this frame hold no valid data.
        auto begin_pass( float r, float g, float b ) -> bool;
        /// Binds the block pipeline.
        auto bind_block_pipeline() -> void;
        /// Binds the entity pipeline: same shaders, no face culling (billboards, flat items).
        auto bind_entity_pipeline() -> void;
        /// Draws the avatar's sprite alpha only where it is behind the existing depth buffer.
        auto bind_avatar_silhouette_pipeline() -> void;
        /// Draws indexed triangles from the given buffers with one sheet texture bound.
        /// @param sheet an SDL_Texture of the GPU renderer holding the tileset sprites.
        struct draw_block_options {
            SDL_GPUBuffer *positions;
            SDL_GPUBuffer *lights;
            SDL_GPUBuffer *indices;
            uint32_t first_index;
            uint32_t index_count;
            SDL_Texture *sheet;
            const block_uniforms &uniforms;
        };

        auto draw_block( const draw_block_options &options ) -> void;
        /// Ends the pass and submits. Returns the texture holding the result, or null on error.
        auto end_frame() -> SDL_Texture *; // *NOPAD*
        /// Abandons an unfinished frame (error path).
        auto abort_frame() -> void;

    private:
        struct load_shader_options {
            const char *basename;
            SDL_GPUShaderStage stage;
            uint32_t samplers;
            uint32_t uniform_buffers;
        };

        auto load_shader( const load_shader_options &options ) -> SDL_GPUShader *; // *NOPAD*
        auto release_target() -> void;
        auto flush_uploads() -> bool;
        auto release_staging() -> void;

        SDL_Renderer *renderer_ = nullptr;
        SDL_GPUDevice *device_ = nullptr;
        SDL_GPUGraphicsPipeline *block_pipeline_ = nullptr;
        // Same shaders without back-face culling, for billboards and flat sprites that are seen
        // from either side.
        SDL_GPUGraphicsPipeline *entity_pipeline_ = nullptr;
        SDL_GPUGraphicsPipeline *avatar_silhouette_pipeline_ = nullptr;
        SDL_GPUSampler *sampler_ = nullptr;

        SDL_GPUTexture *color_ = nullptr;
        SDL_GPUTexture *depth_ = nullptr;
        SDL_Texture *wrapped_ = nullptr;
        SDL_GPUTextureFormat depth_format_ = SDL_GPU_TEXTUREFORMAT_INVALID;
        int target_w_ = 0;
        int target_h_ = 0;

        SDL_GPUCommandBuffer *cmd_ = nullptr;
        SDL_GPUCopyPass *copy_pass_ = nullptr;
        SDL_GPURenderPass *pass_ = nullptr;
        struct pending_upload {
            SDL_GPUBuffer *dst = nullptr;
            uint32_t offset = 0; // into pending_data_
            uint32_t size = 0;
            bool cycle = false;
        };
        std::vector<pending_upload> pending_;
        std::vector<uint8_t> pending_data_;
        // Reused every frame (mapped with cycling, so the previous frame's copy is not waited on).
        SDL_GPUTransferBuffer *staging_ = nullptr;
        uint32_t staging_size_ = 0;
};

} // namespace r3d

#endif // defined(TILES)

#endif // CATA_SRC_R3D_GPU_H
