#include "assets.h"
#include "math.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <functional>
#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::vector<unsigned char> read_binary_file(const std::filesystem::path& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return {};
    }
    file.seekg(0, std::ios::end);
    const auto size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> data(size);
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return data;
}

Mat4 mat4_from_ai(const aiMatrix4x4& in) {
    Mat4 out = {};
    out.m[0] = in.a1; out.m[4] = in.a2; out.m[8] = in.a3; out.m[12] = in.a4;
    out.m[1] = in.b1; out.m[5] = in.b2; out.m[9] = in.b3; out.m[13] = in.b4;
    out.m[2] = in.c1; out.m[6] = in.c2; out.m[10] = in.c3; out.m[14] = in.c4;
    out.m[3] = in.d1; out.m[7] = in.d2; out.m[11] = in.d3; out.m[15] = in.d4;
    return out;
}

std::array<float, 4> normalize_quat(const std::array<float, 4>& q) {
    const float len = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (len < 1e-6f) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    return {q[0] / len, q[1] / len, q[2] / len, q[3] / len};
}

std::array<float, 4> slerp_quat(const std::array<float, 4>& a, const std::array<float, 4>& b, float t) {
    std::array<float, 4> end = b;
    float dot = a[0] * end[0] + a[1] * end[1] + a[2] * end[2] + a[3] * end[3];
    if (dot < 0.0f) {
        dot = -dot;
        end = {-end[0], -end[1], -end[2], -end[3]};
    }

    if (dot > 0.9995f) {
        std::array<float, 4> out{
            a[0] + t * (end[0] - a[0]),
            a[1] + t * (end[1] - a[1]),
            a[2] + t * (end[2] - a[2]),
            a[3] + t * (end[3] - a[3])
        };
        return normalize_quat(out);
    }

    const float theta_0 = std::acos(clampf(dot, -1.0f, 1.0f));
    const float theta = theta_0 * t;
    const float sin_theta = std::sin(theta);
    const float sin_theta_0 = std::sin(theta_0);
    const float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
    const float s1 = sin_theta / sin_theta_0;
    return {
        s0 * a[0] + s1 * end[0],
        s0 * a[1] + s1 * end[1],
        s0 * a[2] + s1 * end[2],
        s0 * a[3] + s1 * end[3]
    };
}

Vec3 lerp_vec3(const Vec3& a, const Vec3& b, float t) {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

Vec3 sample_vec3_keys(const std::vector<AnimatedCharacter::KeyVec3>& keys, double time_seconds) {
    if (keys.empty()) {
        return {};
    }
    if (keys.size() == 1) {
        return keys.front().value;
    }

    if (time_seconds <= keys.front().time) {
        return keys.front().value;
    }
    if (time_seconds >= keys.back().time) {
        return keys.back().value;
    }

    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (time_seconds < keys[i + 1].time) {
            const double span = std::max(1e-6, keys[i + 1].time - keys[i].time);
            const float t = static_cast<float>((time_seconds - keys[i].time) / span);
            return lerp_vec3(keys[i].value, keys[i + 1].value, t);
        }
    }

    return keys.back().value;
}

std::array<float, 4> sample_quat_keys(const std::vector<AnimatedCharacter::KeyQuat>& keys, double time_seconds) {
    if (keys.empty()) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    if (keys.size() == 1) {
        return normalize_quat(keys.front().value);
    }

    if (time_seconds <= keys.front().time) {
        return normalize_quat(keys.front().value);
    }
    if (time_seconds >= keys.back().time) {
        return normalize_quat(keys.back().value);
    }

    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (time_seconds < keys[i + 1].time) {
            const double span = std::max(1e-6, keys[i + 1].time - keys[i].time);
            const float t = static_cast<float>((time_seconds - keys[i].time) / span);
            return normalize_quat(slerp_quat(keys[i].value, keys[i + 1].value, t));
        }
    }

    return normalize_quat(keys.back().value);
}

std::vector<std::filesystem::path> resource_roots() {
    const std::filesystem::path cwd = std::filesystem::current_path();
    return {
        cwd / "resources",
        cwd,  // Project root for mixamo folder
        cwd.parent_path() / "resources",
        cwd.parent_path(),  // One level up for mixamo folder
        cwd.parent_path().parent_path() / "project" / "resources"
    };
}

std::filesystem::path find_resource(const std::string& relative_path) {
    for (const auto& root : resource_roots()) {
        const auto candidate = root / relative_path;
        if (std::filesystem::exists(candidate)) {
            std::cout << "Found resource: " << candidate << std::endl;
            return candidate;
        }
    }
    std::cout << "Resource not found: " << relative_path << std::endl;
    return {};
}

bool load_animated_character(const std::filesystem::path& model_path, AnimatedCharacter& model, std::string& error) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(model_path.string(), 
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);
    
    if (!scene) {
        error = "Failed to load model: " + std::string(importer.GetErrorString());
        return false;
    }

    if (!scene->HasMeshes()) {
        error = "Model has no meshes";
        return false;
    }

    std::unordered_map<std::string, int> texture_slot_by_key;

    auto add_texture_slot = [&](unsigned int material_index, aiTextureType texture_type) -> int {
        if (material_index >= scene->mNumMaterials) {
            return -1;
        }

        const aiMaterial* material = scene->mMaterials[material_index];
        if (material == nullptr || material->GetTextureCount(texture_type) == 0) {
            return -1;
        }

        aiString rel_path;
        if (material->GetTexture(texture_type, 0, &rel_path) != aiReturn_SUCCESS || rel_path.length == 0) {
            return -1;
        }

        const std::string rel = rel_path.C_Str();
        const std::string key = (rel[0] == '*')
            ? rel
            : (model_path.parent_path() / rel).lexically_normal().string();

        const auto existing = texture_slot_by_key.find(key);
        if (existing != texture_slot_by_key.end()) {
            return existing->second;
        }

        if (rel[0] == '*') {
            return -1;
        }

        CharacterMesh::Texture texture;
        texture.image_path = (model_path.parent_path() / rel).lexically_normal();

        model.mesh.textures.push_back(std::move(texture));
        const int slot = static_cast<int>(model.mesh.textures.size() - 1);
        texture_slot_by_key.emplace(key, slot);
        return slot;
    };

    std::vector<Vec3> all_positions;

    // Load mesh data
    for (unsigned int mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx) {
        aiMesh* ai_mesh = scene->mMeshes[mesh_idx];
        CharacterMesh::Primitive prim;

        // Load positions and tex coords
        for (unsigned int v = 0; v < ai_mesh->mNumVertices; ++v) {
            prim.positions.push_back({
                ai_mesh->mVertices[v].x,
                ai_mesh->mVertices[v].y,
                ai_mesh->mVertices[v].z
            });
            all_positions.push_back(prim.positions.back());

            if (ai_mesh->HasTextureCoords(0)) {
                prim.texcoords.push_back({
                    ai_mesh->mTextureCoords[0][v].x,
                    ai_mesh->mTextureCoords[0][v].y
                });
            } else {
                prim.texcoords.push_back({0.0f, 0.0f});
            }

            // Initialize bone data
            prim.bone_ids.push_back({-1, -1, -1, -1});
            prim.bone_weights.push_back({0.0f, 0.0f, 0.0f, 0.0f});
        }

        // Load indices
        for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
            aiFace& face = ai_mesh->mFaces[f];
            for (unsigned int i = 0; i < face.mNumIndices; ++i) {
                prim.indices.push_back(face.mIndices[i]);
            }
        }

        // Load bone data
        if (ai_mesh->HasBones()) {
            for (unsigned int b = 0; b < ai_mesh->mNumBones; ++b) {
                aiBone* bone = ai_mesh->mBones[b];
                
                // Add bone to lookup if not already present
                if (model.bone_lookup.find(bone->mName.C_Str()) == model.bone_lookup.end()) {
                    model.bone_lookup[bone->mName.C_Str()] = model.bones.size();
                    AnimatedCharacter::Bone new_bone;
                    new_bone.name = bone->mName.C_Str();
                    new_bone.offset = mat4_from_ai(bone->mOffsetMatrix);
                    model.bones.push_back(new_bone);
                }

                // Apply bone weights to vertices
                int bone_id = model.bone_lookup[bone->mName.C_Str()];
                for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                    unsigned int vert_id = bone->mWeights[w].mVertexId;
                    float weight = bone->mWeights[w].mWeight;

                    // Find first available slot
                    for (int slot = 0; slot < 4; ++slot) {
                        if (prim.bone_weights[vert_id][slot] < 0.001f) {
                            prim.bone_ids[vert_id][slot] = bone_id;
                            prim.bone_weights[vert_id][slot] = weight;
                            break;
                        }
                    }
                }
            }
        }

        if (ai_mesh->mMaterialIndex < scene->mNumMaterials) {
            int base_texture_slot = add_texture_slot(ai_mesh->mMaterialIndex, aiTextureType_DIFFUSE);
            prim.texture_id = base_texture_slot;
        }

        // Create vertex data (position + uv)
        for (size_t v = 0; v < prim.positions.size(); ++v) {
            prim.vertex_data.push_back(prim.positions[v].x);
            prim.vertex_data.push_back(prim.positions[v].y);
            prim.vertex_data.push_back(prim.positions[v].z);
            prim.vertex_data.push_back(prim.texcoords[v][0]);
            prim.vertex_data.push_back(prim.texcoords[v][1]);
        }

        model.mesh.primitives.push_back(prim);
    }

    // Load skeleton
    if (scene->mRootNode) {
        std::function<void(aiNode*, int)> process_node = [&](aiNode* node, int parent_idx) {
            model.node_lookup[node->mName.C_Str()] = model.nodes.size();
            
            AnimatedCharacter::Node char_node;
            char_node.name = node->mName.C_Str();
            char_node.local_transform = mat4_from_ai(node->mTransformation);
            
            int node_idx = model.nodes.size();
            model.nodes.push_back(char_node);

            // Link parent-child
            if (parent_idx >= 0) {
                model.nodes[parent_idx].children.push_back(node_idx);
            }

            // Update bone node references
            for (auto& bone : model.bones) {
                if (bone.name == node->mName.C_Str()) {
                    bone.node_index = node_idx;
                }
            }

            // Process children
            for (unsigned int c = 0; c < node->mNumChildren; ++c) {
                process_node(node->mChildren[c], node_idx);
            }
        };

        process_node(scene->mRootNode, -1);
    }

    // Load animations
    if (scene->HasAnimations()) {
        for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
            aiAnimation* ai_anim = scene->mAnimations[a];
            AnimatedCharacter::Clip clip;
            clip.name = ai_anim->mName.C_Str();
            clip.duration = ai_anim->mDuration;
            clip.ticks_per_second = ai_anim->mTicksPerSecond > 0 ? ai_anim->mTicksPerSecond : 25.0;

            for (unsigned int c = 0; c < ai_anim->mNumChannels; ++c) {
                aiNodeAnim* ai_channel = ai_anim->mChannels[c];
                AnimatedCharacter::Channel channel;
                channel.node_name = ai_channel->mNodeName.C_Str();

                // Position keys
                for (unsigned int k = 0; k < ai_channel->mNumPositionKeys; ++k) {
                    AnimatedCharacter::KeyVec3 key;
                    key.time = ai_channel->mPositionKeys[k].mTime;
                    key.value = {
                        ai_channel->mPositionKeys[k].mValue.x,
                        ai_channel->mPositionKeys[k].mValue.y,
                        ai_channel->mPositionKeys[k].mValue.z
                    };
                    channel.position_keys.push_back(key);
                }

                // Rotation keys
                for (unsigned int k = 0; k < ai_channel->mNumRotationKeys; ++k) {
                    AnimatedCharacter::KeyQuat key;
                    key.time = ai_channel->mRotationKeys[k].mTime;
                    key.value = {
                        ai_channel->mRotationKeys[k].mValue.x,
                        ai_channel->mRotationKeys[k].mValue.y,
                        ai_channel->mRotationKeys[k].mValue.z,
                        ai_channel->mRotationKeys[k].mValue.w
                    };
                    channel.rotation_keys.push_back(key);
                }

                // Scale keys
                for (unsigned int k = 0; k < ai_channel->mNumScalingKeys; ++k) {
                    AnimatedCharacter::KeyVec3 key;
                    key.time = ai_channel->mScalingKeys[k].mTime;
                    key.value = {
                        ai_channel->mScalingKeys[k].mValue.x,
                        ai_channel->mScalingKeys[k].mValue.y,
                        ai_channel->mScalingKeys[k].mValue.z
                    };
                    channel.scale_keys.push_back(key);
                }

                clip.channels.push_back(channel);
            }

            model.clips.push_back(clip);
        }
    }

    // Calculate inverse global matrix
    if (!model.nodes.empty()) {
        model.global_inverse = mat4_from_ai(scene->mRootNode->mTransformation.Inverse());
        model.node_transforms.resize(model.nodes.size(), identity_mat4());
    }

    if (!all_positions.empty()) {
        float min_x = all_positions.front().x;
        float min_y = all_positions.front().y;
        float min_z = all_positions.front().z;
        float max_x = all_positions.front().x;
        float max_z = all_positions.front().z;

        for (const auto& position : all_positions) {
            min_x = std::min(min_x, position.x);
            min_y = std::min(min_y, position.y);
            min_z = std::min(min_z, position.z);
            max_x = std::max(max_x, position.x);
            max_z = std::max(max_z, position.z);
        }

        model.bounds_center = {0.5f * (min_x + max_x), 0.0f, 0.5f * (min_z + max_z)};
        model.bounds_min_y = min_y;
        const float half_x = 0.5f * (max_x - min_x);
        const float half_z = 0.5f * (max_z - min_z);
        const float raw_radius = std::max(half_x, half_z);
        model.bounds_scale = (raw_radius > 1e-5f) ? (1.0f / raw_radius) : 1.0f;
        model.normalization = compose_trs(
            {-model.bounds_center[0], -model.bounds_min_y, -model.bounds_center[2]},
            {0.0f, 0.0f, 0.0f, 1.0f},
            {model.bounds_scale, model.bounds_scale, model.bounds_scale}
        );
    }

    std::cout << "Loaded character with " << model.mesh.primitives.size() << " primitives, "
              << model.bones.size() << " bones, " << model.clips.size() << " animations" << std::endl;

    return true;
}

bool load_animation_clip(const std::filesystem::path& clip_path, AnimatedCharacter::Clip& clip, std::string& error) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(clip_path.string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
    if (scene == nullptr || scene->mNumAnimations == 0) {
        error = "Failed to import animation clip: " + clip_path.string();
        const char* assimp_error = importer.GetErrorString();
        if (assimp_error != nullptr && assimp_error[0] != '\0') {
            error += " (" + std::string(assimp_error) + ")";
        }
        return false;
    }

    const aiAnimation* anim = scene->mAnimations[0];
    clip = AnimatedCharacter::Clip{};
    clip.name = anim->mName.C_Str();
    if (clip.name.empty()) {
        clip.name = clip_path.stem().string();
    }
    clip.duration = static_cast<double>(anim->mDuration);
    clip.ticks_per_second = anim->mTicksPerSecond > 0.0 ? anim->mTicksPerSecond : 25.0;

    for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
        const aiNodeAnim* channel = anim->mChannels[c];
        if (channel == nullptr) {
            continue;
        }

        AnimatedCharacter::Channel out_channel;
        out_channel.node_name = channel->mNodeName.C_Str();

        for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k) {
            AnimatedCharacter::KeyVec3 key;
            key.time = channel->mPositionKeys[k].mTime;
            key.value = {
                channel->mPositionKeys[k].mValue.x,
                channel->mPositionKeys[k].mValue.y,
                channel->mPositionKeys[k].mValue.z
            };
            out_channel.position_keys.push_back(key);
        }

        for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k) {
            AnimatedCharacter::KeyQuat key;
            key.time = channel->mRotationKeys[k].mTime;
            key.value = {
                channel->mRotationKeys[k].mValue.x,
                channel->mRotationKeys[k].mValue.y,
                channel->mRotationKeys[k].mValue.z,
                channel->mRotationKeys[k].mValue.w
            };
            out_channel.rotation_keys.push_back(key);
        }

        for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k) {
            AnimatedCharacter::KeyVec3 key;
            key.time = channel->mScalingKeys[k].mTime;
            key.value = {
                channel->mScalingKeys[k].mValue.x,
                channel->mScalingKeys[k].mValue.y,
                channel->mScalingKeys[k].mValue.z
            };
            out_channel.scale_keys.push_back(key);
        }

        clip.channels.push_back(std::move(out_channel));
    }

    return true;
}

void update_animated_character(AnimatedCharacter& model, int clip_index, float time_seconds) {
    if (model.mesh.primitives.empty()) {
        return;
    }

    if (clip_index < 0 || clip_index >= static_cast<int>(model.clips.size()) || model.bones.empty()) {
        for (auto& prim : model.mesh.primitives) {
            prim.vertex_data.clear();
            prim.vertex_data.reserve(prim.positions.size() * 5);
            for (size_t i = 0; i < prim.positions.size(); ++i) {
                const Vec3& p = prim.positions[i];
                const float x = (p.x - model.bounds_center[0]) * model.bounds_scale;
                const float y = (p.y - model.bounds_min_y) * model.bounds_scale;
                const float z = (p.z - model.bounds_center[2]) * model.bounds_scale;
                const float u = (i < prim.texcoords.size()) ? prim.texcoords[i][0] : 0.0f;
                const float v = (i < prim.texcoords.size()) ? (1.0f - prim.texcoords[i][1]) : 0.0f;
                prim.vertex_data.push_back(x);
                prim.vertex_data.push_back(y);
                prim.vertex_data.push_back(z);
                prim.vertex_data.push_back(u);
                prim.vertex_data.push_back(v);
            }
        }
        return;
    }

    const auto& clip = model.clips[clip_index];
    double clip_time = time_seconds;
    if (clip.duration > 0.0) {
        clip_time = std::fmod(time_seconds * clip.ticks_per_second, clip.duration);
    }

    // Build channel lookup
    std::unordered_map<std::string, const AnimatedCharacter::Channel*> channel_lookup;
    for (const auto& channel : clip.channels) {
        channel_lookup[channel.node_name] = &channel;
    }

    // Calculate global transforms using proper tree recursion
    std::vector<Mat4> global_transforms(model.nodes.size(), identity_mat4());

    std::function<void(int, const Mat4&)> recurse = [&](int node_index, const Mat4& parent_transform) {
        if (node_index < 0 || node_index >= static_cast<int>(model.nodes.size())) {
            return;
        }

        const auto& node = model.nodes[node_index];
        Mat4 local = node.local_transform;

        // Sample animation channel for this node
        const auto channel_it = channel_lookup.find(node.name);
        if (channel_it != channel_lookup.end() && channel_it->second != nullptr) {
            const auto& channel = *channel_it->second;
            
            Vec3 translation = channel.position_keys.empty()
                ? Vec3{0.0f, 0.0f, 0.0f}
                : sample_vec3_keys(channel.position_keys, clip_time);
            
            Vec3 scale = channel.scale_keys.empty()
                ? Vec3{1.0f, 1.0f, 1.0f}
                : sample_vec3_keys(channel.scale_keys, clip_time);
            
            std::array<float, 4> rotation = channel.rotation_keys.empty()
                ? std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}
                : sample_quat_keys(channel.rotation_keys, clip_time);

            local = compose_trs({translation.x, translation.y, translation.z}, rotation, {scale.x, scale.y, scale.z});
        }

        global_transforms[node_index] = mul_mat4(parent_transform, local);

        // Recurse to children
        for (int child_index : node.children) {
            recurse(child_index, global_transforms[node_index]);
        }
    };

    if (!model.nodes.empty()) {
        recurse(0, identity_mat4());
    }

    // Calculate bone transforms
    model.bone_transforms.clear();
    model.bone_transforms.resize(model.bones.size(), identity_mat4());

    for (size_t i = 0; i < model.bones.size(); ++i) {
        const auto& bone = model.bones[i];
        if (bone.node_index >= 0 && bone.node_index < static_cast<int>(global_transforms.size())) {
            model.bone_transforms[i] = mul_mat4(model.global_inverse, mul_mat4(global_transforms[bone.node_index], bone.offset));
        }
    }

    // Apply bone transforms to vertices
    for (auto& prim : model.mesh.primitives) {
        prim.vertex_data.clear();
        prim.vertex_data.reserve(prim.positions.size() * 5);

        for (size_t i = 0; i < prim.positions.size(); ++i) {
            Vec3 skinned{};
            bool has_influence = false;

            const auto& ids = prim.bone_ids[i];
            const auto& weights = prim.bone_weights[i];
            for (int j = 0; j < 4; ++j) {
                if (ids[j] < 0 || weights[j] <= 0.0f || ids[j] >= static_cast<int>(model.bone_transforms.size())) {
                    continue;
                }
                const Vec3 transformed = transform_point(model.bone_transforms[ids[j]], prim.positions[i]);
                skinned = skinned + transformed * weights[j];
                has_influence = true;
            }

            if (!has_influence) {
                skinned = prim.positions[i];
            }

            const float x = (skinned.x - model.bounds_center[0]) * model.bounds_scale;
            const float y = (skinned.y - model.bounds_min_y) * model.bounds_scale;
            const float z = (skinned.z - model.bounds_center[2]) * model.bounds_scale;
            const float u = (i < prim.texcoords.size()) ? prim.texcoords[i][0] : 0.0f;
            const float v = (i < prim.texcoords.size()) ? (1.0f - prim.texcoords[i][1]) : 0.0f;

            prim.vertex_data.push_back(x);
            prim.vertex_data.push_back(y);
            prim.vertex_data.push_back(z);
            prim.vertex_data.push_back(u);
            prim.vertex_data.push_back(v);
        }

        if (prim.vbo != 0 && !prim.vertex_data.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, prim.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, prim.vertex_data.size() * sizeof(float), prim.vertex_data.data());
        }
    }
}

bool upload_character_textures(CharacterMesh& mesh, std::string& error) {
    for (auto& texture : mesh.textures) {
        int width = texture.width;
        int height = texture.height;
        unsigned char* data = nullptr;
        if (!texture.embedded_rgba.empty()) {
            data = texture.embedded_rgba.data();
        } else if (std::filesystem::exists(texture.image_path)) {
            int channels = 0;
            data = stbi_load(texture.image_path.string().c_str(), &width, &height, &channels, 4);
        } else {
            std::cout << "Texture not found: " << texture.image_path << std::endl;
            continue;
        }

        if (data == nullptr || width <= 0 || height <= 0) {
            continue;
        }

        GLuint tex_id;
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        texture.gl_id = tex_id;
        texture.width = width;
        texture.height = height;
        if (texture.embedded_rgba.empty()) {
            stbi_image_free(data);
        }
    }

    return true;
}

void free_character_textures(CharacterMesh& mesh) {
    for (auto& texture : mesh.textures) {
        if (texture.gl_id != 0) {
            glDeleteTextures(1, &texture.gl_id);
            texture.gl_id = 0;
        }
    }
}

void setup_character_mesh_gpu(CharacterMesh& mesh) {
    for (auto& prim : mesh.primitives) {
        glGenVertexArrays(1, &prim.vao);
        glGenBuffers(1, &prim.vbo);
        glGenBuffers(1, &prim.ebo);
        glGenBuffers(1, &prim.bone_id_vbo);
        glGenBuffers(1, &prim.bone_weight_vbo);

        glBindVertexArray(prim.vao);
        glBindBuffer(GL_ARRAY_BUFFER, prim.vbo);
        glBufferData(GL_ARRAY_BUFFER, prim.vertex_data.size() * sizeof(float), prim.vertex_data.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, prim.indices.size() * sizeof(unsigned int), prim.indices.data(), GL_STATIC_DRAW);

        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // TexCoord
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, prim.bone_id_vbo);
        glBufferData(GL_ARRAY_BUFFER, prim.bone_ids.size() * sizeof(std::array<int, 4>), prim.bone_ids.data(), GL_STATIC_DRAW);
        glVertexAttribIPointer(2, 4, GL_INT, sizeof(std::array<int, 4>), (void*)0);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, prim.bone_weight_vbo);
        glBufferData(GL_ARRAY_BUFFER, prim.bone_weights.size() * sizeof(std::array<float, 4>), prim.bone_weights.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(std::array<float, 4>), (void*)0);
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void free_character_mesh_gpu(CharacterMesh& mesh) {
    for (auto& prim : mesh.primitives) {
        if (prim.vao != 0) {
            glDeleteVertexArrays(1, &prim.vao);
        }
        if (prim.vbo != 0) {
            glDeleteBuffers(1, &prim.vbo);
        }
        if (prim.ebo != 0) {
            glDeleteBuffers(1, &prim.ebo);
        }
        if (prim.bone_id_vbo != 0) {
            glDeleteBuffers(1, &prim.bone_id_vbo);
        }
        if (prim.bone_weight_vbo != 0) {
            glDeleteBuffers(1, &prim.bone_weight_vbo);
        }
    }
}
