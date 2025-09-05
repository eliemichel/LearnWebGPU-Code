// {Begin block 'file: resources/shader.wgsl' (in root '037 - Loading from file - Next')}
// In a new file 'resources/shader.wgsl'
// Move the content of the global `shaderSource` variable (and remove that variable from main.cpp)
// {Begin block 'Shader source' (in root '033 - Multiple Attributes - Option A - Next')}
// {Begin block 'Shader prelude' (in root '054 - Transformation matrices - Next')}
// {Begin block 'Define VertexInput struct' (in root '050 - A simple example - Next')}
struct VertexInput {
	@location(0) position: vec3f,
	//                        ^ This was a 2
	@location(1) color: vec3f,
};
// {End block 'Define VertexInput struct' (in root '050 - A simple example - Next')}
// {Begin block 'Define VertexOutput struct' (in root '033 - Multiple Attributes - Option A - Next')}
/**
 * A structure with fields labeled with builtins and locations can also be used
 * as *output* of the vertex shader, which is also the input of the fragment
 * shader.
 */
struct VertexOutput {
	@builtin(position) position: vec4f,
	// The location here does not refer to a vertex attribute, it just means
	// that this field must be handled by the rasterizer.
	// (It can also refer to another field of another struct that would be used
	// as input to the fragment shader.)
	@location(0) color: vec3f,
};
// {End block 'Define VertexOutput struct' (in root '033 - Multiple Attributes - Option A - Next')}
// We add the declaration of 'uTime' to the shader prelude
// {Begin block 'Declare uniforms' (in root '043 - More uniforms - Next')}
struct MyUniforms {
	color: vec4f, // <-- this is now first!
	time: f32,
};

@group(0) @binding(0)
var<uniform> uMyUniforms: MyUniforms;
// {End block 'Declare uniforms' (in root '043 - More uniforms - Next')}
// Anywhere in the global scope (e.g. just before defining vs_main)
const pi = 3.14159265359;
// {End block 'Shader prelude' (in root '054 - Transformation matrices - Next')}

@vertex
// {Begin block 'Vertex shader' (in root '050 - A simple example - Next')}
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	let ratio = 640.0 / 480.0;
	// {Begin block 'Set vertex out position' (in root '055 - Projection matrices - Next')}
	var position = in.position;
	// {Begin block 'Transform vertex position' (in root '054 - Transformation matrices - Next')}
	// Scale the object
	let S = transpose(mat4x4f(
		0.3,  0.0, 0.0, 0.0,
		0.0,  0.3, 0.0, 0.0,
		0.0,  0.0, 0.3, 0.0,
		0.0,  0.0, 0.0, 1.0,
	));
	
	// Translate the object
	let T = transpose(mat4x4f(
		1.0,  0.0, 0.0, 0.5,
		0.0,  1.0, 0.0, 0.0,
		0.0,  0.0, 1.0, 0.0,
		0.0,  0.0, 0.0, 1.0,
	));
	
	// {Begin block 'Define R1 and R2 as above BUT as mat4x4' (in root '054 - Transformation matrices - Next')}
	// Rotate the model in the XY plane
	let angle1 = uMyUniforms.time;
	let c1 = cos(angle1);
	let s1 = sin(angle1);
	let R1 = transpose(mat4x4f(
		 c1,  s1, 0.0, 0.0,
		-s1,  c1, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0,  0.0, 0.0, 1.0,
	));
	
	// Tilt the view point in the YZ plane
	// by three 8th of turn (1 turn = 2 pi)
	let angle2 = 3.0 * pi / 4.0;
	let c2 = cos(angle2);
	let s2 = sin(angle2);
	let R2 = transpose(mat4x4f(
		1.0, 0.0, 0.0, 0.0,
		0.0,  c2,  s2, 0.0,
		0.0, -s2,  c2, 0.0,
		0.0,  0.0, 0.0, 1.0,
	));
	// {End block 'Define R1 and R2 as above BUT as mat4x4' (in root '054 - Transformation matrices - Next')}
	
	let homogeneous_position = vec4f(position, 1.0);
	position = (R2 * R1 * T * S * homogeneous_position).xyz;
	// {End block 'Transform vertex position' (in root '054 - Transformation matrices - Next')}
	
	// We move the view point so that all Z coordinates are > 0
	// (this did not make a difference with the orthographic projection
	// but does now.)
	let focalPoint = vec3f(0.0, 0.0, -2.0);
	position = position - focalPoint;
	
	// We divide by the Z coord
	position.x /= position.z;
	position.y /= position.z;
	
	// Apply the orthographic matrix for remapping Z and handling the ratio
	// near and far must be positive
	let near = 0.0;
	let far = 100.0;
	let scale = 1.0;
	// {Begin block 'Define projection matrix P' (in root '055 - Projection matrices - Next')}
	let P = transpose(mat4x4f(
		1.0 / scale,      0.0,           0.0,                  0.0,
		    0.0,     ratio / scale,      0.0,                  0.0,
		    0.0,          0.0,      1.0 / (far - near), -near / (far - near),
		    0.0,          0.0,           0.0,                  1.0,
	));
	// {End block 'Define projection matrix P' (in root '055 - Projection matrices - Next')}
	out.position = P * vec4f(position, 1.0);
	// {End block 'Set vertex out position' (in root '055 - Projection matrices - Next')}
	out.color = in.color;
	return out;
}
// {End block 'Vertex shader' (in root '050 - A simple example - Next')}

@fragment
// {Begin block 'Fragment shader' (in root '043 - More uniforms - Next')}
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	// We multiply the scene's color with our global uniform (this is one
	// possible use of the color uniform, among many others).
	return vec4f(in.color, 1.0) * uMyUniforms.color;
}
// {End block 'Fragment shader' (in root '043 - More uniforms - Next')}
// {End block 'Shader source' (in root '033 - Multiple Attributes - Option A - Next')}
// {End block 'file: resources/shader.wgsl' (in root '037 - Loading from file - Next')}