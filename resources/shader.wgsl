// In a new file 'resources/shader.wgsl'
// Move the content of the global `shaderSource` variable (and remove that variable from main.cpp)
struct VertexInput {
	@location(0) position: vec3f,
	//                        ^ This was a 2
	@location(1) color: vec3f,
};
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
// We add the declaration of 'uTime' to the shader prelude
struct MyUniforms {
	color: vec4f, // <-- this is now first!
	time: f32,
};

@group(0) @binding(0)
var<uniform> uMyUniforms: MyUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	let ratio = 640.0 / 480.0;
	let angle = uMyUniforms.time; // you can multiply it go rotate faster
	let alpha = cos(angle);
	let beta = sin(angle);
	var position = vec3f(
		in.position.x,
		alpha * in.position.y + beta * in.position.z,
		alpha * in.position.z - beta * in.position.y,
	);
	out.position = vec4f(position.x, position.y * ratio, position.z * 0.5 + 0.5, 1.0);
	//                                     This changes: ^^^^^^^^^^^^^^^^^^^^^^
	out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	// We multiply the scene's color with our global uniform (this is one
	// possible use of the color uniform, among many others).
	return vec4f(in.color, 1.0) * uMyUniforms.color;
}