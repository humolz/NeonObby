#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "renderer/ModelLoader.h"

#include <iostream>
#include <unordered_map>
#include <cstring>
#include <cstdint>

namespace {

// Read a float accessor element by index
void readFloats(const cgltf_accessor* acc, size_t index, float* out, int count) {
    cgltf_accessor_read_float(acc, index, out, count);
}

// Read uint accessor element (for indices and joint indices)
uint32_t readUInt(const cgltf_accessor* acc, size_t index) {
    cgltf_uint val = 0;
    cgltf_accessor_read_uint(acc, index, &val, 1);
    return static_cast<uint32_t>(val);
}

void readUInt4(const cgltf_accessor* acc, size_t index, uint32_t* out) {
    cgltf_uint vals[4] = {0, 0, 0, 0};
    cgltf_accessor_read_uint(acc, index, vals, 4);
    out[0] = vals[0]; out[1] = vals[1]; out[2] = vals[2]; out[3] = vals[3];
}

} // namespace

std::unique_ptr<Model> ModelLoader::load(const std::string& path) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success) {
        std::cerr << "[ModelLoader] Failed to parse: " << path << " (code " << result << ")\n";
        return nullptr;
    }

    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success) {
        std::cerr << "[ModelLoader] Failed to load buffers: " << path << "\n";
        cgltf_free(data);
        return nullptr;
    }

    if (cgltf_validate(data) != cgltf_result_success) {
        std::cerr << "[ModelLoader] Validation failed: " << path << "\n";
        cgltf_free(data);
        return nullptr;
    }

    auto model = std::make_unique<Model>();

    // ---------- Build node→bone index map (using skin joints) ----------
    if (data->skins_count == 0) {
        std::cerr << "[ModelLoader] No skin found in " << path << "\n";
        cgltf_free(data);
        return nullptr;
    }

    const cgltf_skin* skin = &data->skins[0];
    size_t boneCount = skin->joints_count;
    model->bones.resize(boneCount);

    // Map cgltf_node* to bone index
    std::unordered_map<const cgltf_node*, int> nodeToBone;
    for (size_t i = 0; i < boneCount; i++) {
        nodeToBone[skin->joints[i]] = static_cast<int>(i);
    }

    // Fill bone data
    for (size_t i = 0; i < boneCount; i++) {
        const cgltf_node* node = skin->joints[i];
        Bone& bone = model->bones[i];

        bone.name = node->name ? node->name : ("bone_" + std::to_string(i));

        // Parent
        bone.parentIndex = -1;
        if (node->parent) {
            auto it = nodeToBone.find(node->parent);
            if (it != nodeToBone.end()) {
                bone.parentIndex = it->second;
            }
        }

        // Inverse bind matrix
        if (skin->inverse_bind_matrices) {
            float ibm[16];
            cgltf_accessor_read_float(skin->inverse_bind_matrices, i, ibm, 16);
            bone.inverseBindMatrix = glm::mat4(
                ibm[0], ibm[1], ibm[2], ibm[3],
                ibm[4], ibm[5], ibm[6], ibm[7],
                ibm[8], ibm[9], ibm[10], ibm[11],
                ibm[12], ibm[13], ibm[14], ibm[15]
            );
        }

        // Local TRS
        if (node->has_translation) {
            bone.localTranslation = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
        }
        if (node->has_rotation) {
            // glTF stores quat as (x, y, z, w), glm::quat constructor takes (w, x, y, z)
            bone.localRotation = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
        }
        if (node->has_scale) {
            bone.localScale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);
        }
    }

    // ---------- Load mesh geometry ----------
    if (data->meshes_count == 0) {
        std::cerr << "[ModelLoader] No meshes in " << path << "\n";
        cgltf_free(data);
        return nullptr;
    }

    const cgltf_mesh* mesh = &data->meshes[0];
    if (mesh->primitives_count == 0) {
        std::cerr << "[ModelLoader] Mesh has no primitives\n";
        cgltf_free(data);
        return nullptr;
    }

    std::vector<SkinnedVertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t indexOffset = 0;

    for (size_t pi = 0; pi < mesh->primitives_count; pi++) {
        const cgltf_primitive* prim = &mesh->primitives[pi];

        const cgltf_accessor* posAcc = nullptr;
        const cgltf_accessor* normAcc = nullptr;
        const cgltf_accessor* jointAcc = nullptr;
        const cgltf_accessor* weightAcc = nullptr;

        for (size_t ai = 0; ai < prim->attributes_count; ai++) {
            const cgltf_attribute* attr = &prim->attributes[ai];
            switch (attr->type) {
            case cgltf_attribute_type_position: posAcc = attr->data; break;
            case cgltf_attribute_type_normal:   normAcc = attr->data; break;
            case cgltf_attribute_type_joints:   if (!jointAcc) jointAcc = attr->data; break;
            case cgltf_attribute_type_weights:  if (!weightAcc) weightAcc = attr->data; break;
            default: break;
            }
        }

        if (!posAcc) {
            std::cerr << "[ModelLoader] Primitive missing position\n";
            continue;
        }

        size_t vertexCount = posAcc->count;
        size_t baseVertex = vertices.size();
        vertices.resize(baseVertex + vertexCount);

        for (size_t v = 0; v < vertexCount; v++) {
            SkinnedVertex& sv = vertices[baseVertex + v];

            float pos[3] = {0, 0, 0};
            cgltf_accessor_read_float(posAcc, v, pos, 3);
            sv.position = {pos[0], pos[1], pos[2]};

            if (normAcc) {
                float n[3] = {0, 1, 0};
                cgltf_accessor_read_float(normAcc, v, n, 3);
                sv.normal = {n[0], n[1], n[2]};
            }

            if (jointAcc) {
                uint32_t j[4] = {0, 0, 0, 0};
                readUInt4(jointAcc, v, j);
                sv.jointIndices = {(int)j[0], (int)j[1], (int)j[2], (int)j[3]};
            }

            if (weightAcc) {
                float w[4] = {0, 0, 0, 0};
                cgltf_accessor_read_float(weightAcc, v, w, 4);
                sv.jointWeights = {w[0], w[1], w[2], w[3]};
            } else {
                // Fully bound to bone 0 if no weights
                sv.jointWeights = {1.0f, 0.0f, 0.0f, 0.0f};
            }
        }

        // Indices
        if (prim->indices) {
            for (size_t i = 0; i < prim->indices->count; i++) {
                uint32_t idx = readUInt(prim->indices, i);
                indices.push_back(idx + indexOffset);
            }
        } else {
            // No indices: generate sequential
            for (size_t i = 0; i < vertexCount; i++) {
                indices.push_back(static_cast<uint32_t>(i + indexOffset));
            }
        }

        indexOffset += static_cast<uint32_t>(vertexCount);
    }

    model->mesh.upload(vertices, indices);

    // ---------- Load animations ----------
    model->clips.reserve(data->animations_count);
    for (size_t ai = 0; ai < data->animations_count; ai++) {
        const cgltf_animation* anim = &data->animations[ai];
        AnimationClip clip;
        clip.name = anim->name ? anim->name : ("anim_" + std::to_string(ai));

        for (size_t ci = 0; ci < anim->channels_count; ci++) {
            const cgltf_animation_channel* ch = &anim->channels[ci];
            const cgltf_animation_sampler* samp = ch->sampler;

            if (!ch->target_node || !samp || !samp->input || !samp->output) continue;

            auto it = nodeToBone.find(ch->target_node);
            if (it == nodeToBone.end()) continue; // not a joint

            AnimationChannel achan;
            achan.boneIndex = it->second;

            switch (ch->target_path) {
            case cgltf_animation_path_type_translation: achan.path = ChannelPath::Translation; break;
            case cgltf_animation_path_type_rotation:    achan.path = ChannelPath::Rotation; break;
            case cgltf_animation_path_type_scale:       achan.path = ChannelPath::Scale; break;
            default: continue;
            }

            // Read times
            achan.times.resize(samp->input->count);
            for (size_t k = 0; k < samp->input->count; k++) {
                cgltf_accessor_read_float(samp->input, k, &achan.times[k], 1);
                if (achan.times[k] > clip.duration) clip.duration = achan.times[k];
            }

            // Read values
            int components = (achan.path == ChannelPath::Rotation) ? 4 : 3;
            achan.values.resize(samp->output->count);
            for (size_t k = 0; k < samp->output->count; k++) {
                float v[4] = {0, 0, 0, 0};
                cgltf_accessor_read_float(samp->output, k, v, components);
                achan.values[k] = glm::vec4(v[0], v[1], v[2], v[3]);
            }

            clip.channels.push_back(std::move(achan));
        }

        model->clips.push_back(std::move(clip));
    }

    std::cout << "[ModelLoader] Loaded " << path << ": "
              << vertices.size() << " verts, "
              << indices.size() / 3 << " tris, "
              << boneCount << " bones, "
              << model->clips.size() << " animations\n";

    for (const auto& clip : model->clips) {
        std::cout << "  - " << clip.name << " (" << clip.duration << "s, "
                  << clip.channels.size() << " channels)\n";
    }

    cgltf_free(data);
    return model;
}
