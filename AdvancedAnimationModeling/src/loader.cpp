/**
* @file loader.cpp
* @author Diego Sanz , 540001618 , diego.sanz@digipen.edu
* @date 2020/09/20
* @copyright Copyright (C) 2020 DigiPen Institute of Technology .
*/
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "loader.h"
#include <tiny_gltf.h>
#include <iostream>
#include "node.h"
#include "scene_graph.h"
#include "resources.h"
#include <glad/glad.h>
#include "mesh_comp.h"

namespace cs460 {
typedef std::vector<int> indices;
typedef tinygltf::Model gltf_model;

bool load_model(gltf_model& model, const char* file_name)
{
	// Create a tinygltf loader
	tinygltf::TinyGLTF loader;
	std::string error, warning;

	// Try to load the file
	bool result = loader.LoadASCIIFromFile(&model, &error, &warning, file_name);

	// Notify the user if the program failed to load the file
	if (!result)
		std::cout << "Failed to load the gltf file: " << file_name << std::endl;

	// Notify the user of any errors or warnings
	if (warning.empty() == false)
		std::cout << "Loader warning: " << warning << std::endl;
	if (error.empty() == false)
		std::cout << "Loader Error: " << error << std::endl;

	return result;
}

void set_node_transform(node_rsc* node, const tinygltf::Node& data)
{
	// Transform given in matrix form
	if (data.matrix.size() != 0)
	{
		// Create the matrix
		const std::vector<double>& m = data.matrix;
		glm::mat4 trs(
			(float)m[0],  (float)m[1],  (float)m[2],  (float)m[3],
			(float)m[4],  (float)m[5],  (float)m[6],  (float)m[7],
			(float)m[8],  (float)m[9],  (float)m[10], (float)m[11],
			(float)m[12], (float)m[13], (float)m[14], (float)m[15]);

		node->m_local.set_transform(trs);
	}

	// Otherwise, check for TRS separately
	else
	{
		// Set translation
		if (data.translation.size() != 0)
		{
			glm::vec3 pos(
				(float)data.translation[0], 
				(float)data.translation[1], 
				(float)data.translation[2]);
			
			node->m_local.set_position(pos);
		}
		else
			node->m_local.set_position(glm::vec3(0.0f));

		// Set rotation
		if (data.rotation.size() != 0)
		{
			glm::quat rot(
				(float)data.rotation[3], 
				(float)data.rotation[0], 
				(float)data.rotation[1], 
				(float)data.rotation[2]);

			node->m_local.set_rotation(rot);
		}
		else
			node->m_local.set_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

		// Set scale
		if (data.scale.size() != 0)
		{
			glm::vec3 scale(
				(float)data.scale[0], 
				(float)data.scale[1], 
				(float)data.scale[2]);

			node->m_local.set_scale(scale);
		}
		else
			node->m_local.set_scale(glm::vec3(1.0f));
	}
}

void save_node(const gltf_model& model, const int node_idx, const unsigned int rsc_id, const indices& grandchild_ids)
{
	// Get the node data
	const tinygltf::Node& data = model.nodes[node_idx];
	
	// Create the node in the resource manager
	node_rsc node;
	node.m_name = data.name;
	node.m_mesh = data.mesh;
	node.m_skin = data.skin;

	if (data.skin >= 0)
	{
		int skin_root = model.skins[data.skin].skeleton;
		if (skin_root >= 0)
			node.m_skin_root = skin_root;
		else
			node.m_skin_root = node_idx;
	}

	node.m_childs = grandchild_ids;
	set_node_transform(&node, data);
	g_resources.record_node(rsc_id, node_idx, node);
}

void save_nodes_rec(const gltf_model& model, const indices& child_node_ids, const unsigned int rsc_id)
{
	// Get the number of childs
	size_t n_childs = child_node_ids.size();

	// Create each child
	for (size_t i = 0; i < n_childs; ++i)
	{
		// Get the child index
		int child_idx = child_node_ids[i];

		// Get the grandchildren indices
		const indices& grandchild_ids = model.nodes[child_idx].children;
		
		// Record the node in the resource manager
		save_node(model, child_idx, rsc_id, grandchild_ids);

		// Create the grandchildren
		save_nodes_rec(model, grandchild_ids, rsc_id);
	}
}

void save_nodes(const gltf_model& model, const unsigned int rsc_id)
{
	// Get the node indices of the first scene (a.k.a childs of the root node)
	const indices& node_ids = model.scenes[0].nodes;

	// Record the root nodes of the model
	g_resources.get_model_rsc(rsc_id).m_root_nodes = node_ids;

	// Create the nodes in the scene graph
	save_nodes_rec(model, node_ids, rsc_id);
}

void send_data_to_buffer(const gltf_model& model, const tinygltf::BufferView& buff_view, unsigned int buffer)
{
	// Get the buffer
	const tinygltf::Buffer& buff = model.buffers[buff_view.buffer];

	glBindBuffer(buff_view.target, buffer);
	glBufferData(buff_view.target, buff_view.byteLength, buff.data.data() + buff_view.byteOffset, GL_STATIC_DRAW);
}

void set_primitive_attribute(const gltf_model& model, primitive& prim, model_rsc& rsc, const int attrib_idx, const int acc_idx)
{
	// Get the accessor, 
	const tinygltf::Accessor& acc = model.accessors[acc_idx];

	// Get the bufferview
	const tinygltf::BufferView& buff_view = model.bufferViews[acc.bufferView];
	
	// Set the number of vertices
	if (attrib_idx == 0)
		prim.m_num_vertices = (int)acc.count;

	// Get the vbo handle of this buffer view
	unsigned int vbo;
	bool created = rsc.get_buffer(acc.bufferView, &vbo);

	// Check if the buffer is empty (not created yet)
	if (created == false)
		send_data_to_buffer(model, buff_view, vbo);

	int size = 0;
	if (attrib_idx == 0 || attrib_idx == 1 || attrib_idx == 4)
		size = 3;
	else if (attrib_idx == 2 || attrib_idx == 3)
		size = 2;
	else if (attrib_idx == 5 || attrib_idx == 6)
		size = 4;

	// Set attribute
	prim.set_attribute_pointer(vbo, attrib_idx, size, (int)acc.componentType, acc.ByteStride(buff_view), (int)acc.byteOffset);
}

void set_primitive_attributes(const gltf_model& model, primitive& prim, mesh& mesh, model_rsc& rsc, const tinygltf::Primitive& prim_data)
{
	int position_attrib_idx = 0;
	int normal_attrib_idx = 1;
	int diffuse_uv_idx = 2;
	int normal_map_uv_idx = 3;
	int tangent_idx = 4;
	int joints_idx = 5;
	int weights_idx = 6;

	// Set the position attribute
	auto it = prim_data.attributes.find("POSITION");
	auto end = prim_data.attributes.end();
	assert(it != end);
	set_primitive_attribute(model, prim, rsc, position_attrib_idx, it->second);

	// Update the min and max vertex of the mesh
	const std::vector<double>& min = model.accessors[it->second].minValues;
	const std::vector<double>& max = model.accessors[it->second].maxValues;
	if (min.size() == 3 && max.size() == 3)
		mesh.update_min_max_vertices(glm::vec3(min[0], min[1], min[2]), glm::vec3(max[0], max[1], max[2]));

	// Set the normal attribute
	it = prim_data.attributes.find("NORMAL");
	if (it != end)
		set_primitive_attribute(model, prim, rsc, normal_attrib_idx, it->second);
	else
		prim.m_no_normals = true;

	// Set the joints attribute
	it = prim_data.attributes.find("JOINTS_0");
	if (it != end)
		set_primitive_attribute(model, prim, rsc, joints_idx, it->second);

	// Set the weights attribute
	it = prim_data.attributes.find("WEIGHTS_0");
	if (it != end)
		set_primitive_attribute(model, prim, rsc, weights_idx, it->second);

	// Get the material of the primitive
	const tinygltf::Material& mat_data = model.materials[prim_data.material];
	
	// Check if the material has a diffuse texture
	if (mat_data.pbrMetallicRoughness.baseColorTexture.index >= 0)
	{
		// Get the text coord attribute id
		int uv_id = mat_data.pbrMetallicRoughness.baseColorTexture.texCoord;

		// Set the texture coordinates
		it = prim_data.attributes.find("TEXCOORD_" + std::to_string(uv_id));
		assert(it != end);
		set_primitive_attribute(model, prim, rsc, diffuse_uv_idx, it->second);
	}

	// Check if the material has a normal map
	if (mat_data.normalTexture.index >= 0)
	{
		// Set the normal map texture coordiantes
		int uv_id = mat_data.normalTexture.texCoord;
		it = prim_data.attributes.find("TEXCOORD_" + std::to_string(uv_id));
		assert(it != end);
		set_primitive_attribute(model, prim, rsc, normal_map_uv_idx, it->second);

		// Set the tangents
		it = prim_data.attributes.find("TANGENT");
		if (it != end)
		{
			set_primitive_attribute(model, prim, rsc, tangent_idx, it->second);
			prim.m_tangents = true;
		}
		else
			prim.m_tangents = false;
	}
	else
		prim.m_tangents = false;
}

void set_primitive_indices(const gltf_model& model, primitive& prim, model_rsc& rsc, const tinygltf::Primitive& prim_data)
{
	if (prim_data.indices < 0)
	{
		prim.m_no_ebo = true;
		return;
	}
	
	// Get the accessor
	const tinygltf::Accessor& acc = model.accessors[prim_data.indices];

	// Get the bufferview
	const tinygltf::BufferView& buff_view = model.bufferViews[acc.bufferView];

	// Get the vbo handle of this buffer view
	unsigned int ebo;
	bool created = rsc.get_buffer(acc.bufferView, &ebo);

	// Check if the buffer is empty (not created yet)
	if (created == false)
		send_data_to_buffer(model, buff_view, ebo);

	prim.set_indices(ebo, (int)acc.componentType, (int)acc.count, (int)acc.byteOffset);
}

void send_texture_data(const gltf_model& model, unsigned int tex_handle, int tex_idx)
{
	const tinygltf::Image& image = model.images[tex_idx];

	// Bind
	glBindTexture(GL_TEXTURE_2D, tex_handle);

	// Set texture params
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Get the image format
	unsigned int format = GL_RGBA;
	if (image.component == 1)
		format = GL_RED;
	else if (image.component == 2)
		format = GL_RG;
	else if (image.component == 3)
		format = GL_RGB;

	// Get the image number of bits
	unsigned int type = GL_UNSIGNED_BYTE;
	if (image.bits == 16)
		type = GL_UNSIGNED_SHORT;

	// Send the data to the GPU
	glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0,
		format, type, image.image.data());

	glGenerateMipmap(GL_TEXTURE_2D);
}

unsigned int load_material(const gltf_model& model, int texture_idx, model_rsc& rsc)
{
	unsigned int handle;
	bool created = rsc.get_texture(texture_idx, &handle);
	if (created == false)
		send_texture_data(model, handle, texture_idx);
	return handle;
}

void set_material(const gltf_model& model, material& mat, model_rsc& rsc, const tinygltf::Material& mat_data)
{
	// Set the base color
	const std::vector<double>& b_color = mat_data.pbrMetallicRoughness.baseColorFactor;
	mat.m_base_color = glm::vec4(b_color[0], b_color[1], b_color[2], b_color[3]);

	// Get the image idx of the base color texture
	int tex_idx = mat_data.pbrMetallicRoughness.baseColorTexture.index;
	if (tex_idx >= 0)
	{
		int base_image_idx = model.textures[tex_idx].source;
		if (base_image_idx < 0)
			mat.m_diffuse = -1;
		else
			mat.m_diffuse = load_material(model, base_image_idx, rsc);
	}

	// Get the image index of the normal texture
	tex_idx = mat_data.normalTexture.index;

	if (tex_idx >= 0)
	{
		int normal_image_idx = model.textures[tex_idx].source;
		if (normal_image_idx < 0)
			mat.m_normal = -1;
		else
			mat.m_normal = load_material(model, normal_image_idx, rsc);
	}
}

void set_primitive_material(const gltf_model& model, primitive& prim, model_rsc& rsc, const tinygltf::Primitive& prim_data)
{
	int mat_idx = prim_data.material;

	assert(mat_idx >= 0);

	// Check if the material has been created
	bool created;
	material& mat = rsc.get_material(mat_idx, &created);

	// Set the material for the first time if necessary
	if (created == false)
		set_material(model, mat, rsc, model.materials[mat_idx]);

	// Set the material index
	prim.m_material = mat_idx;
}

void create_primitives(const gltf_model& model, const tinygltf::Mesh& mesh_data, mesh& mesh, model_rsc& rsc)
{
	// Get the number of primitives
	size_t n_primitives = mesh_data.primitives.size();

	// Create each primitive
	for (size_t i = 0; i < n_primitives; ++i)
	{
		// Create the primitive
		primitive& prim = mesh.new_primitive();

		// Get the primitive data
		const tinygltf::Primitive& prim_data = mesh_data.primitives[i];

		// Set the rendering mode
		prim.m_render_mode = prim_data.mode;

		// Set the material of the primitive
		set_primitive_material(model, prim, rsc, prim_data);

		// Set the attributes
		set_primitive_attributes(model, prim, mesh, rsc, prim_data);

		// Set the indices
		set_primitive_indices(model, prim, rsc, prim_data);
	}
}

void create_mesh(const gltf_model& model, unsigned int mesh_id, model_rsc& rsc)
{
	const tinygltf::Mesh& mesh_data = model.meshes[mesh_id];

	// Create a mesh
	mesh& mesh = rsc.new_mesh();
	mesh.m_name = mesh_data.name;

	create_primitives(model, mesh_data, mesh, rsc);
}

void create_meshes(const gltf_model& model, model_rsc& rsc)
{
	// Get the number of meshes
	size_t n_meshes = model.meshes.size();

	// Create each mesh
	for (size_t i = 0; i < n_meshes; ++i)
		create_mesh(model, (int)i, rsc);
}

void create_skin(const gltf_model& model, unsigned int skin_id, model_rsc& rsc)
{
	const tinygltf::Skin& skin_data = model.skins[skin_id];

	// Create a mesh
	skin& skin = rsc.new_skin();
	skin.m_name = skin_data.name;
	skin.m_joints = skin_data.joints;

	// Get the accessor of the joint matrices
	const tinygltf::Accessor& acc = model.accessors[skin_data.inverseBindMatrices];

	// Get the buffer view of the joint matrices
	const tinygltf::BufferView& buff_view = model.bufferViews[acc.bufferView];

	// Get the buffer data
	const unsigned char* buff_data = model.buffers[buff_view.buffer].data.data();

	// Point to the first matrix
	const unsigned char* walker = buff_data + buff_view.byteOffset + acc.byteOffset;

	// Get the number of matrices
	int n_matrices = (int)acc.count;

	// Store all the matrices
	int stride = acc.ByteStride(buff_view);
	for (int i = 0; i < n_matrices; ++i, walker += stride)
	{
		const glm::mat4* mtx = reinterpret_cast<const glm::mat4*>(walker);
		skin.m_inv_bind_mtxs.push_back(*mtx);
	}
}

void create_skins(const gltf_model& model, model_rsc& rsc)
{
	// Get the number of skins
	size_t n_skins = model.skins.size();

	// Create each skin
	for (size_t i = 0; i < n_skins; ++i)
		create_skin(model, (int)i, rsc);
}

void create_sampler(const gltf_model& model, animation& anim, const tinygltf::Animation& anim_data, int sampler_id)
{
	// Create the sampler
	const tinygltf::AnimationSampler& sampler = anim_data.samplers[sampler_id];
	animation::sampler s;

	// Interpolation mode
	if (sampler.interpolation == "LINEAR")
		s.m_lerp_mode = animation::sampler::lerp_mode::linear;
	else if (sampler.interpolation == "STEP")
		s.m_lerp_mode = animation::sampler::lerp_mode::step;
	else if (sampler.interpolation == "CUBICSPLINE")
		s.m_lerp_mode = animation::sampler::lerp_mode::cubic;

	// Input:
	
	// Get the accessor
	const tinygltf::Accessor& acc1 = model.accessors[sampler.input];

	// Get the buffer view
	const tinygltf::BufferView& buff_view1 = model.bufferViews[acc1.bufferView];

	// Get the buffer data
	const unsigned char* buff_data = model.buffers[buff_view1.buffer].data.data();
	
	// Point to the first input
	const unsigned char* walker = buff_data + buff_view1.byteOffset + acc1.byteOffset;

	// Load the input data
	int n_input = (int)acc1.count;
	int stride = acc1.ByteStride(buff_view1);
	for (int i = 0; i < n_input; ++i, walker += stride)
	{
		const float* input = reinterpret_cast<const float*>(walker);
		s.m_input.push_back(*input);
	}

	// Output:

	// Get the accessor
	const tinygltf::Accessor& acc2 = model.accessors[sampler.output];

	// Get the buffer view
	const tinygltf::BufferView& buff_view2 = model.bufferViews[acc2.bufferView];

	// Get the buffer data
	buff_data = model.buffers[buff_view2.buffer].data.data();

	// Point to the first input
	walker = buff_data + buff_view2.byteOffset + acc2.byteOffset;

	// Load the input data
	int n_output = (int)acc2.count;
	stride = acc2.ByteStride(buff_view2);
	int n_comps = acc2.type;
	if (n_comps == TINYGLTF_TYPE_SCALAR)
		n_comps = 1;
	for (int i = 0; i < n_output; ++i, walker += stride)
	{
		for (int j = 0; j < n_comps; ++j)
		{
			const float* out = reinterpret_cast<const float*>(walker + j * sizeof(float));
			s.m_output.push_back(*out);
		}
	}

	// Set the max time of the animation
	float max_time = s.m_input[n_input - 1];
	if (max_time > anim.m_max_time)
		anim.m_max_time = max_time;

	anim.m_samplers.push_back(s);
}

void create_animation(const gltf_model& model, unsigned int anim_id, model_rsc& rsc)
{
	const tinygltf::Animation& anim_data = model.animations[anim_id];

	// Create the animation
	animation& anim = rsc.new_anim();
	anim.m_name = std::to_string(anim_id) + " " + anim_data.name;

	// Create the samplers
	size_t n_samplers = anim_data.samplers.size();
	for (size_t i = 0; i < n_samplers; ++i)
		create_sampler(model, anim, anim_data, (int)i);

	// Create the channels
	size_t n_channels = anim_data.channels.size();
	for (size_t i = 0; i < n_channels; ++i)
	{
		const tinygltf::AnimationChannel& ch_data = anim_data.channels[i];
		animation::channel ch;
		
		// Set the target node
		ch.m_node = ch_data.target_node;
		
		// Set the path type
		if (ch_data.target_path == "translation")
			ch.m_path_type = animation::channel::path_type::translation;
		else if (ch_data.target_path == "rotation")
			ch.m_path_type = animation::channel::path_type::rotation;
		else if (ch_data.target_path == "scale")
			ch.m_path_type = animation::channel::path_type::scale;
		else if (ch_data.target_path == "weights")
			ch.m_path_type = animation::channel::path_type::weights;

		// Set the sampler
		ch.m_sampler = ch_data.sampler;
		anim.m_chanels.push_back(ch);
	}
}

void create_animations(const gltf_model& model, model_rsc& rsc)
{
	size_t n_anims = model.animations.size();
	for (size_t i = 0; i < n_anims; ++i)
		create_animation(model, (int)i, rsc);
}

int create_resources(const gltf_model& model, const char* model_name)
{
	// Create the new model in the resource manager
	int rsc_id = g_resources.new_model(model_name);

	// Get the resources of the new model
	model_rsc& rsc = g_resources.get_model_rsc(rsc_id);

	create_meshes(model, rsc);
	create_skins(model, rsc);
	create_animations(model, rsc);

	return rsc_id;
}

void import_model(const gltf_model& model, const char* model_name)
{
	// Create the resources (meshes, textures...)
	int model_rsc_id = create_resources(model, model_name);

	// Save node info in the resource manager
	save_nodes(model, model_rsc_id);
}

void import_gltf_file(const char* file_name)
{
	// Check if the model has already been loaded
	if (g_resources.model_registered(file_name))
	{
		std::cout << "model: " << file_name << " is already loaded" << std::endl;
		return;
	}

	// Load the gltf file into the model
	tinygltf::Model model;
	bool success = load_model(model, file_name);
	
	// Check if the program failed to load the file
	if (!success)
		return;

	import_model(model, file_name);
}
}